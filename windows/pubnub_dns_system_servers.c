/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"

#include "core/pubnub_dns_servers.h"
#include "core/pubnub_assert.h"
#include "core/pubnub_log.h"

#include <winsock2.h>
#include <windows.h>
#include <iphlpapi.h>
#include <string.h>

#pragma comment(lib, "IPHLPAPI.lib")

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x)   HeapFree(GetProcessHeap(), 0, (x))

/* Check if an IPv4 address already exists in the array */
static int ipv4_already_exists(const struct pubnub_ipv4_address* array, size_t count, const unsigned char new_ip[4]) {
    size_t i;
    for (i = 0; i < count; i++) {
        if (memcmp(array[i].ipv4, new_ip, 4) == 0) {
            return 1; /* Found duplicate */
        }
    }
    return 0; /* Not found */
}

int pubnub_dns_read_system_servers_ipv4(struct pubnub_ipv4_address* o_ipv4, size_t n)
{
    ULONG buflen;
    DWORD ret;
    IP_ADAPTER_ADDRESSES* addrs;
    unsigned j;
    IP_ADAPTER_ADDRESSES* aa;
    IP_ADAPTER_DNS_SERVER_ADDRESS* ds;
    const struct sockaddr_in* sin;
    DWORD net_addr;
    unsigned char temp_ip[4];

    if (!o_ipv4 || n == 0) {
        return 0;
    }

    buflen = 0;
    j = 0;

    /* Get required buffer size */
    ret = GetAdaptersAddresses(
        AF_INET,
        GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST, /* keep DNS servers */
        NULL, NULL, &buflen
    );

    if (ret != ERROR_BUFFER_OVERFLOW || buflen == 0) {
        PUBNUB_LOG_ERROR("GetAdaptersAddresses preflight failed: %lu\n", (unsigned long)ret);
        return -1;
    }

    addrs = (IP_ADAPTER_ADDRESSES*)MALLOC(buflen);
    if (!addrs) {
        PUBNUB_LOG_ERROR("OOM allocating %lu for GetAdaptersAddresses\n", (unsigned long)buflen);
        return -1;
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
        return -1;
    }

    /* Enumerate adapters and collect unique DNS servers */
    for (aa = addrs; aa && j < n; aa = aa->Next) {
        for (ds = aa->FirstDnsServerAddress; ds && j < n; ds = ds->Next) {
            if (!ds->Address.lpSockaddr || ds->Address.lpSockaddr->sa_family != AF_INET) {
                continue;
            }
            
            sin = (const struct sockaddr_in*)ds->Address.lpSockaddr;
            net_addr = sin->sin_addr.S_un.S_addr;
            if (net_addr == 0) {
                continue; /* skip 0.0.0.0 */
            }
            
            /* Convert from network order to host order, then extract bytes */
            {
                DWORD host_addr = ntohl(net_addr);
                temp_ip[0] = (unsigned char)((host_addr >> 24) & 0xFF);
                temp_ip[1] = (unsigned char)((host_addr >> 16) & 0xFF);
                temp_ip[2] = (unsigned char)((host_addr >>  8) & 0xFF);
                temp_ip[3] = (unsigned char)( host_addr        & 0xFF);
            }
            
            if (!ipv4_already_exists(o_ipv4, j, temp_ip)) {
                o_ipv4[j].ipv4[0] = temp_ip[0];
                o_ipv4[j].ipv4[1] = temp_ip[1];
                o_ipv4[j].ipv4[2] = temp_ip[2];
                o_ipv4[j].ipv4[3] = temp_ip[3];
                ++j;
            }
        }
    }

    FREE(addrs);
    return (int)j;
}