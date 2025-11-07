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
static bool copy_ipv4_bytes_from_be_dword(
    const struct pubnub_ipv4_address* array,
    const size_t count,
    DWORD n_addr,
    unsigned char out[4])
{
    DWORD h_addr = ntohl(n_addr);
    unsigned char temp_ip[4];
    bool is_unique = false;

    temp_ip[0] = (unsigned char)((h_addr >> 24) & 0xFF);
    temp_ip[1] = (unsigned char)((h_addr >> 16) & 0xFF);
    temp_ip[2] = (unsigned char)((h_addr >> 8) & 0xFF);
    temp_ip[3] = (unsigned char)(h_addr & 0xFF);

    for (size_t i = 0; i < count; i++) {
        if (memcmp(array[i].ipv4, temp_ip, 4) == 0) return false;
    }

    out[0] = temp_ip[0];
    out[1] = temp_ip[1];
    out[2] = temp_ip[2];
    out[3] = temp_ip[3];

    return true;
}

int fallback_get_dns_via_adapters(struct pubnub_ipv4_address* o_ipv4, size_t n)
{
    ULONG buflen;
    DWORD ret;
    IP_ADAPTER_ADDRESSES* addrs;
    DWORD net_addr;
    unsigned j = 0;
    unsigned char temp_ip[4];

    /* Get required buffer size */
    ret = GetAdaptersAddresses(
        AF_INET,
        GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST, /* keep DNS servers */
        NULL, NULL, &buflen
    );

    if (ret != ERROR_BUFFER_OVERFLOW || buflen == 0) {
        PUBNUB_LOG_ERROR("GetAdaptersAddresses preflight failed: %lu\n", (unsigned long)ret);
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
        GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST, /* keep DNS servers */
        NULL, addrs, &buflen
    );
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

        for (IP_ADAPTER_DNS_SERVER_ADDRESS* ds = aa->FirstDnsServerAddress; ds && j < n; ds = ds->Next) {
            if (!ds->Address.lpSockaddr ||
                ds->Address.lpSockaddr->sa_family != AF_INET) {
                continue;
            }

            const struct sockaddr_in* sin = (const struct sockaddr_in*)ds->Address.lpSockaddr;
            net_addr = sin->sin_addr.S_un.S_addr;
            if (net_addr == 0) continue; /* skip 0.0.0.0 */

            if (copy_ipv4_bytes_from_be_dword(o_ipv4, j, net_addr, o_ipv4[j].ipv4))
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
                for (DWORD i = 0; i < ip4_list->AddrCount && j < n; ++i) {
                    if (ip4_list->AddrArray[i] == 0) continue;

                    if (copy_ipv4_bytes_from_be_dword(o_ipv4, j, ip4_list->AddrArray[i], o_ipv4[j].ipv4))
                        ++j;
                }
            }

            LocalFree(ip4_list);
        }

        if (j > 0) return (int)j;
    }

    return fallback_get_dns_via_adapters(o_ipv4, n);
}