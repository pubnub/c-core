/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"

#include "core/pubnub_dns_servers.h"
#include "core/pubnub_assert.h"
#include "core/pubnub_log.h"

#include <winsock2.h>
#include <iphlpapi.h>
#include <windows.h>
#include <windns.h>
#include <string.h>

#pragma comment(lib, "IPHLPAPI.lib")
#pragma comment(lib, "dnsapi.lib")

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

/** Copy to the local address endianness from the network address endianness. */
static bool pubnub_copy_ipv4_bytes_from_be_dword(
    const struct pubnub_ipv4_address* array,
    const size_t count,
    DWORD n_addr,
    unsigned char out[4])
{
    DWORD host_addr = ntohl(n_addr);
    unsigned char temp_ip[4];

    temp_ip[0] = (unsigned char)((host_addr >> 24) & 0xFF);
    temp_ip[1] = (unsigned char)((host_addr >> 16) & 0xFF);
    temp_ip[2] = (unsigned char)((host_addr >> 8) & 0xFF);
    temp_ip[3] = (unsigned char)(host_addr & 0xFF);

    /* Filter out loopback and APIPA addresses from `DnsQueryConfig`. */
    if (127 == temp_ip[0] || (169 == temp_ip[0] && 254 == temp_ip[1]))
        return false;

    for (size_t i = 0; i < count; i++) {
        if (memcmp(array[i].ipv4, temp_ip, 4) == 0) return false;
    }

    out[0] = temp_ip[0];
    out[1] = temp_ip[1];
    out[2] = temp_ip[2];
    out[3] = temp_ip[3];

    return true;
}

/** Check whether the DNS address is retrieved for the active network adapter
    or not.
 */
static bool pubnub_dns_from_active_adapter(DWORD n_addr, IP_ADAPTER_ADDRESSES* addrs)
{
    if (!addrs) return true;
    bool found = false;

    for (IP_ADAPTER_ADDRESSES* aa = addrs; aa; aa = aa->Next) {
        if (aa->OperStatus != IfOperStatusUp) continue;

        /** Search for "online" adapters with gateway or valid unicast address.
         *  Note: FirstGatewayAddress can be NULL even for working adapters
         *  on some Windows configurations (WSL, Hyper-V, certain VPNs), so we
         *  also check if the adapter has a valid unicast IP address as a fallback.
         */
        bool has_gateway = false;
        for (IP_ADAPTER_GATEWAY_ADDRESS* gw = aa->FirstGatewayAddress; gw != NULL; gw = gw->Next) {
            if (NULL != gw->Address.lpSockaddr) {
                has_gateway = true;
                break;
            }
        }
        
        /* Fallback: if no gateway, check if adapter has valid unicast address */
        bool has_valid_unicast = false;
        if (!has_gateway) {
            for (IP_ADAPTER_UNICAST_ADDRESS* ua = aa->FirstUnicastAddress; ua != NULL; ua = ua->Next) {
                if (ua->Address.lpSockaddr && 
                    ua->Address.lpSockaddr->sa_family == AF_INET) {
                    struct sockaddr_in* sin = (struct sockaddr_in*)ua->Address.lpSockaddr;
                    DWORD addr = ntohl(sin->sin_addr.S_un.S_addr);
                    /* Valid if not 0.0.0.0, not loopback (127.x), not APIPA (169.254.x.x) */
                    if (addr != 0 && 
                        ((addr >> 24) & 0xFF) != 127 &&
                        !(((addr >> 24) & 0xFF) == 169 && ((addr >> 16) & 0xFF) == 254)) {
                        has_valid_unicast = true;
                        break;
                    }
                }
            }
        }
        
        if (!has_gateway && !has_valid_unicast) continue;

        for (IP_ADAPTER_DNS_SERVER_ADDRESS* ds = aa->FirstDnsServerAddress; ds; ds = ds->Next) {
            if (!ds->Address.lpSockaddr ||
                ds->Address.lpSockaddr->sa_family != AF_INET) {
                continue;
            }

            const struct sockaddr_in* sin = (const struct sockaddr_in*)ds->Address.lpSockaddr;
            DWORD net_addr = sin->sin_addr.S_un.S_addr;
            if (net_addr == 0) continue;

            if (net_addr == n_addr) {
                found = true;
                break;
            }
        }
        if (found) break;
    }

    return found;
}

/** Retrieve DNS servers from "online" network adapters. */
int pubnub_dns_get_via_adapters(struct pubnub_ipv4_address* o_ipv4, size_t n)
{
    IP_ADAPTER_ADDRESSES* addrs;
    unsigned char temp_ip[4];
    DWORD net_addr;
    unsigned j = 0;
    ULONG buflen;
    DWORD ret;

    /* Get required buffer size */
    ret = GetAdaptersAddresses(
        AF_INET,
        GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST,
        NULL, NULL, &buflen);

    if (ret != ERROR_BUFFER_OVERFLOW || buflen == 0) {
        PUBNUB_LOG_DEBUG("GetAdaptersAddresses can't retrieve any adapters: %lu\n", (unsigned long)ret);
        return (int)j;
    }

    addrs = (IP_ADAPTER_ADDRESSES*)MALLOC(buflen);
    if (!addrs) {
        PUBNUB_LOG_ERROR("OOM allocating %lu for GetAdaptersAddresses\n", (unsigned long)buflen);
        return (int)j;
    }

    /* Get adapter information */
    ret = GetAdaptersAddresses(
        AF_INET,
        GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST,
        NULL, addrs, &buflen);
    if (ret != NO_ERROR) {
        PUBNUB_LOG_ERROR("GetAdaptersAddresses failed: %lu\n", (unsigned long)ret);
        FREE(addrs);
        return (int)j;
    }

    /* Enumerate adapters and collect unique DNS servers */
    for (IP_ADAPTER_ADDRESSES* aa = addrs; aa && j < n; aa = aa->Next) {
        if (aa->OperStatus != IfOperStatusUp ||
            aa->IfType == IF_TYPE_SOFTWARE_LOOPBACK ||
            aa->IfType == IF_TYPE_TUNNEL ||
            !(aa->Flags & IP_ADAPTER_IPV4_ENABLED)) {
            continue;
        }

        /** Search for "online" adapters with gateway or valid unicast address.
         *  Note: FirstGatewayAddress can be NULL even for working adapters
         *  on some Windows configurations (WSL, Hyper-V, certain VPNs), so we
         *  also check if the adapter has a valid unicast IP address as a fallback.
         */
        bool has_gateway = false;
        for (IP_ADAPTER_GATEWAY_ADDRESS* gw = aa->FirstGatewayAddress; gw != NULL; gw = gw->Next) {
            if (NULL != gw->Address.lpSockaddr) {
                has_gateway = true;
                break;
            }
        }
        
        /* Fallback: if no gateway, check if adapter has valid unicast address */
        bool has_valid_unicast = false;
        if (!has_gateway) {
            for (IP_ADAPTER_UNICAST_ADDRESS* ua = aa->FirstUnicastAddress; ua != NULL; ua = ua->Next) {
                if (ua->Address.lpSockaddr && 
                    ua->Address.lpSockaddr->sa_family == AF_INET) {
                    struct sockaddr_in* sin = (struct sockaddr_in*)ua->Address.lpSockaddr;
                    DWORD addr = ntohl(sin->sin_addr.S_un.S_addr);
                    /* Valid if not 0.0.0.0, not loopback (127.x), not APIPA (169.254.x.x) */
                    if (addr != 0 && 
                        ((addr >> 24) & 0xFF) != 127 &&
                        !(((addr >> 24) & 0xFF) == 169 && ((addr >> 16) & 0xFF) == 254)) {
                        has_valid_unicast = true;
                        break;
                    }
                }
            }
        }
        
        if (!has_gateway && !has_valid_unicast) continue;

        for (IP_ADAPTER_DNS_SERVER_ADDRESS* ds = aa->FirstDnsServerAddress; ds && j < n; ds = ds->Next) {
            if (!ds->Address.lpSockaddr ||
                ds->Address.lpSockaddr->sa_family != AF_INET) {
                continue;
            }

            const struct sockaddr_in* sin = (const struct sockaddr_in*)ds->Address.lpSockaddr;
            net_addr = sin->sin_addr.S_un.S_addr;
            if (net_addr == 0) continue;

            if (pubnub_copy_ipv4_bytes_from_be_dword(o_ipv4, j, net_addr, o_ipv4[j].ipv4))
                ++j;
        }
    }

    FREE(addrs);
    return (int)j;
}

int pubnub_dns_read_system_servers_ipv4(struct pubnub_ipv4_address* o_ipv4, size_t n)
{
    if (!o_ipv4 || n == 0) return 0;

    DWORD buflen = 0;
    DNS_STATUS status = DnsQueryConfig(
        DnsConfigDnsServerList,
        0, NULL, NULL, NULL, &buflen);

    if (status == ERROR_SUCCESS && buflen >= sizeof(IP4_ARRAY)) {
        unsigned j = 0;
        PIP4_ARRAY ip4_list = (PIP4_ARRAY)LocalAlloc(LMEM_FIXED, buflen);
        if (ip4_list) {
            status = DnsQueryConfig(
                DnsConfigDnsServerList,
                0, NULL, NULL, ip4_list, &buflen);

            if (status == ERROR_SUCCESS) {
                IP_ADAPTER_ADDRESSES* active_adapters = NULL;
                ULONG adapter_buflen = 0;

                if (GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST,
                    NULL, NULL, &adapter_buflen) == ERROR_BUFFER_OVERFLOW) {
                    active_adapters = (IP_ADAPTER_ADDRESSES*)MALLOC(adapter_buflen);
                    if (active_adapters) {
                        DWORD ret = GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST,
                                                         NULL, active_adapters, &adapter_buflen);
                        if (NO_ERROR != ret) {
                            PUBNUB_LOG_WARNING("GetAdaptersAddresses failed: %lu - can't validate DNS servers\n",
                                               (unsigned long)ret);
                            FREE(active_adapters);
                            active_adapters = NULL;
                        }
                    }
                }

                for (DWORD i = 0; i < ip4_list->AddrCount && j < n; ++i) {
                    if (ip4_list->AddrArray[i] == 0 ||
                        !pubnub_dns_from_active_adapter(ip4_list->AddrArray[i], active_adapters)) {
                        continue;
                    }

                    if (pubnub_copy_ipv4_bytes_from_be_dword(o_ipv4, j, ip4_list->AddrArray[i], o_ipv4[j].ipv4))
                        ++j;
                }
                if (active_adapters) FREE(active_adapters);
            }

            LocalFree(ip4_list);
        }

        if (j > 0) return (int)j;
    }

    return pubnub_dns_get_via_adapters(o_ipv4, n);
}