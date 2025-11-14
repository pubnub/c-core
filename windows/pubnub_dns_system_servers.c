/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"

#include "core/pubnub_dns_servers.h"
#include "core/pubnub_assert.h"
#include "core/pubnub_log.h"

#include <winsock2.h>
#include <iphlpapi.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <string.h>
#include <stdlib.h>

#pragma comment(lib, "IPHLPAPI.lib")
#pragma comment(lib, "ws2_32.lib")

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

/* Maximum retry attempts for GetAdaptersAddresses buffer allocation */
#define MAX_ADAPTER_ADDRESS_RETRIES 3

/* Public DNS servers to use for GetBestInterface routing test */
#define PUBLIC_DNS_TEST_IP_1 "8.8.8.8"        /* Google Public DNS */
#define PUBLIC_DNS_TEST_IP_2 "1.1.1.1"        /* Cloudflare DNS */

/* Timeout for DNS server connectivity check (milliseconds) */
#define DNS_CONNECTIVITY_TIMEOUT_MS 2000

/** Helper structure to track adapters with their metrics for sorting */
struct adapter_with_metric {
    IP_ADAPTER_ADDRESSES* adapter;
    ULONG metric;
    bool is_best_route;
};

/** Check whether provided address is valid (not loopback, not APIPA, not zero).
 *
 *  @param host_addr Address in host byte order.
 *  @return @c true if proper non-internal address has been provided,
 *  @c false otherwise.
 */
static bool pubnub_ipv4_address_valid(DWORD host_addr)
{
    if (host_addr == 0 ||
        ((host_addr >> 24) & 0xFF) == 127 ||
        (((host_addr >> 24) & 0xFF) == 169 && ((host_addr >> 16) & 0xFF) == 254)) {
        return false;
    }

    return true;
}

/** Comparison function for qsort to sort adapters by priority.
 *  Priority order:
 *  1. Adapters on the best route (lowest priority number)
 *  2. Lower interface metric
 *
 *  @param a Left-hand @c adapter_with_metric which will be used in comparison.
 *  @param b Right-hand @c adapter_with_metric which will be used in comparison.
 *  @return @b -1 when the left-hand adapter has higher priority than the
 *  right-hand; @b 0 when both adapters have the same priority; @b 1 when the
 *  right-hand adapter has higher priority than the left-hand.
 */
static int compare_adapters_by_priority(const void* a, const void* b)
{
    const struct adapter_with_metric* aa = (const struct adapter_with_metric*)a;
    const struct adapter_with_metric* bb = (const struct adapter_with_metric*)b;

    /* Best route adapters come first */
    if (aa->is_best_route && !bb->is_best_route) return -1;
    if (!aa->is_best_route && bb->is_best_route) return 1;

    /* Then sort by metric (lower is better) */
    if (aa->metric < bb->metric) return -1;
    if (aa->metric > bb->metric) return 1;

    return 0;
}

/** Test if a DNS server is reachable by attempting a TCP connection to port 53.
 *  This is used to detect stale VPN connections where the adapter appears UP
 *  but the DNS server is actually unreachable.
 *
 *  @param dns_addr_net DNS server address in network byte order.
 *  @return @c true if DNS server is reachable, @c false otherwise.
 */
static bool pubnub_dns_server_reachable(DWORD dns_addr_net)
{
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) return false;

    /* Set socket to non-blocking mode for timeout control */
    u_long mode = 1;
    if (ioctlsocket(sock, FIONBIO, &mode) != 0) {
        closesocket(sock);
        return false;
    }

    struct sockaddr_in dns_addr;
    memset(&dns_addr, 0, sizeof(dns_addr));
    dns_addr.sin_family = AF_INET;
    dns_addr.sin_port = htons(53);
    dns_addr.sin_addr.s_addr = dns_addr_net;

    int result = connect(sock, (struct sockaddr*)&dns_addr, sizeof(dns_addr));

    if (result == SOCKET_ERROR) {
        if (WSAGetLastError() != WSAEWOULDBLOCK) {
            closesocket(sock);
            return false;
        }

        /* Wait for socket's in-progress connection complete with timeout. */
        struct timeval timeout;
        timeout.tv_sec = DNS_CONNECTIVITY_TIMEOUT_MS / 1000;
        timeout.tv_usec = (DNS_CONNECTIVITY_TIMEOUT_MS % 1000) * 1000;

        fd_set writefds, exceptfds;
        FD_ZERO(&writefds);
        FD_ZERO(&exceptfds);
        FD_SET(sock, &writefds);
        FD_SET(sock, &exceptfds);

        result = select(0, NULL, &writefds, &exceptfds, &timeout);

        if (result == SOCKET_ERROR || result == 0 ||
            FD_ISSET(sock, &exceptfds) || !FD_ISSET(sock, &writefds)) {
            closesocket(sock);
            return false;
        }

        /* Ensure that connect didn't end up with an error. */
        int so_error = 0;
        int len = sizeof(so_error);
        if (getsockopt(sock, SOL_SOCKET, SO_ERROR, (char*)&so_error, &len) != 0 ||
            so_error != 0) {
            closesocket(sock);
            return false;
        }
    }

    closesocket(sock);
    return true;
}

/** Copy IPv4 address from network byte order DWORD to array, checking for
 *  duplicates.
 *
 *  @param array Pointer to the array of @c pubnub_ipv4_address structures with
 *  previously discovered DNS server addresses.
 *  @param count Current number of entries in @b array.
 *  @param n_addr DNS server address in network byte order.
 *  @param out Target entry in @b array where @b n_addr should be copied (if not
 *  present yet in @b array).
 *  @return @c true if the provided address in network byte order was acceptable
 *  and copied to the list of discovered DNS server addresses, @c false otherwise.
 */
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

    /* Filter out invalid addresses */
    if (!pubnub_ipv4_address_valid(host_addr)) return false;

    /* Check for duplicates */
    for (size_t i = 0; i < count; i++) {
        if (memcmp(array[i].ipv4, temp_ip, 4) == 0) return false;
    }

    out[0] = temp_ip[0];
    out[1] = temp_ip[1];
    out[2] = temp_ip[2];
    out[3] = temp_ip[3];

    return true;
}

/** Get adapter addresses with retry logic to handle race conditions.
 *
 *  @param aa Pointer to the list of @c IP_ADAPTER_ADDRESSES structs.
 *  @return @c true if @p aa has been populated with active adapter,
 *  @c false otherwise.
 */
static bool pubnub_adapter_addresses_get(IP_ADAPTER_ADDRESSES** aa)
{
    if (NULL == aa) return false;

    /* Free existing allocation if present */
    if (*aa) {
        FREE(*aa);
        *aa = NULL;
    }

    ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_INCLUDE_GATEWAYS;
    ULONG buflen = 15000; /* Start with reasonable size to avoid extra call */
    DWORD ret;
    int retries = 0;

    /* Retry loop to handle race conditions where adapters change between calls */
    do {
        IP_ADAPTER_ADDRESSES* buffer = (IP_ADAPTER_ADDRESSES*)MALLOC(buflen);
        if (NULL == buffer) {
            PUBNUB_LOG_ERROR("OOM allocating %lu bytes for GetAdaptersAddresses\n",
                             (unsigned long)buflen);
            return false;
        }

        ret = GetAdaptersAddresses(AF_INET, flags, NULL, buffer, &buflen);

        if (ret == NO_ERROR) {
            *aa = buffer;
            return true;
        }

        FREE(buffer);

        if (ret == ERROR_BUFFER_OVERFLOW) {
            if (++retries >= MAX_ADAPTER_ADDRESS_RETRIES) {
                PUBNUB_LOG_ERROR("GetAdaptersAddresses buffer overflow after "
                                 "%d retries\n", retries);
                return false;
            }
            /* `GetAdaptersAddresses` call failed but updated `buflen` value. */
            continue;
        }

        PUBNUB_LOG_ERROR("GetAdaptersAddresses failed: %lu\n", (unsigned long)ret);
        return false;

    } while (ret == ERROR_BUFFER_OVERFLOW && retries < MAX_ADAPTER_ADDRESS_RETRIES);

    return false;
}

/** Check if an adapter is suitable for DNS queries.
 *  An adapter is suitable if it is:
 *  - Operationally UP, AND
 *  - Not loopback or tunnel, AND
 *  - IPv4 enabled, AND
 *  - Has a valid gateway OR physical adapters with valid unicast address.
 *
 *  @param aa Pointer to the adapter structure which should be checked.
 *  @return @c true if provided adapter fulfill requirements ("alive"),
 *  @c false otherwise.
 */
static bool pubnub_adapter_is_suitable(IP_ADAPTER_ADDRESSES* aa)
{
    if (NULL == aa) return false;

    if (aa->OperStatus != IfOperStatusUp ||
        aa->IfType == IF_TYPE_SOFTWARE_LOOPBACK ||
        aa->IfType == IF_TYPE_TUNNEL ||
        !(aa->Flags & IP_ADAPTER_IPV4_ENABLED)) {
        return false;
    }

    /* Check for valid gateway. */
    for (IP_ADAPTER_GATEWAY_ADDRESS* gw = aa->FirstGatewayAddress; gw != NULL; gw = gw->Next) {
        if (gw->Address.lpSockaddr && gw->Address.lpSockaddr->sa_family == AF_INET) {
            struct sockaddr_in* sin = (struct sockaddr_in*)gw->Address.lpSockaddr;
            if (pubnub_ipv4_address_valid(ntohl(sin->sin_addr.S_un.S_addr)))
                return true;
        }
    }

    /* Fallback: Accept physical Ethernet or WiFi adapters with valid unicast address
     * This handles WSL2, Hyper-V, Docker scenarios where gateway info may be missing
     * but the adapter is still functional for local DNS.
     */
    if (aa->IfType == IF_TYPE_ETHERNET_CSMACD ||
        aa->IfType == IF_TYPE_IEEE80211 ||
        aa->IfType == 71) {

        for (IP_ADAPTER_UNICAST_ADDRESS* ua = aa->FirstUnicastAddress; ua != NULL; ua = ua->Next) {
            if (ua->Address.lpSockaddr && ua->Address.lpSockaddr->sa_family == AF_INET) {
                struct sockaddr_in* sin = (struct sockaddr_in*)ua->Address.lpSockaddr;
                if (pubnub_ipv4_address_valid(ntohl(sin->sin_addr.S_un.S_addr)))
                    return true;
            }
        }
    }

    return false;
}

/** Get the best interface for reaching the public Internet (for DNS queries).
 *  Detect the index of the adapter that is used by Windows for request routing.
 *
 *  @return Non-zero index of the adapter that can route to the public Internet,
 *  zero otherwise.
 */
static DWORD pubnub_get_best_interface_index(void)
{
    DWORD best_if_index = 0;
    struct in_addr dest_addr;

    if (inet_pton(AF_INET, PUBLIC_DNS_TEST_IP_1, &dest_addr) == 1) {
        if (GetBestInterface(dest_addr.S_un.S_addr, &best_if_index) == NO_ERROR && best_if_index != 0) {
            return best_if_index;
        }
    }

    if (inet_pton(AF_INET, PUBLIC_DNS_TEST_IP_2, &dest_addr) == 1) {
        if (GetBestInterface(dest_addr.S_un.S_addr, &best_if_index) == NO_ERROR && best_if_index != 0) {
            return best_if_index;
        }
    }

    return 0;
}

/** Check if DNS server is reachable from the adapter.
* A DNS server is reachable if:
* 1. It's on the same subnet as one of the adapter's unicast addresses, OR
* 2. The adapter has a valid gateway (can route to it)
*
* @param dns_addr_net DNS address in network byte order.
* @param aa Pointer to the adapter that should be tested for subnet unicast
* match with provided DNS server address.
* @return @c true if the adapter can route the request to the provided DNS
* server address, @c false otherwise.
*/
static bool pubnub_dns_reachable_from_adapter(DWORD dns_addr_net, IP_ADAPTER_ADDRESSES* aa)
{
    DWORD dns_addr = ntohl(dns_addr_net);

    /* Check if DNS is on the same subnet as any unicast address */
    for (IP_ADAPTER_UNICAST_ADDRESS* ua = aa->FirstUnicastAddress;
        ua != NULL;
        ua = ua->Next) {
        if (ua->Address.lpSockaddr && ua->Address.lpSockaddr->sa_family == AF_INET) {
            struct sockaddr_in* sin = (struct sockaddr_in*)ua->Address.lpSockaddr;
            DWORD if_addr = ntohl(sin->sin_addr.S_un.S_addr);

            /* Get subnet mask from prefix length */
            ULONG prefix_len = ua->OnLinkPrefixLength;
            if (prefix_len > 0 && prefix_len <= 32) {
                DWORD mask = (prefix_len == 32) ? 0xFFFFFFFF : ~((1UL << (32 - prefix_len)) - 1);

                /* Check if DNS is on same subnet */
                if ((dns_addr & mask) == (if_addr & mask)) {
                    return true;
                }
            }
        }
    }

    /* Check if adapter has a valid gateway (can route off-subnet) */
    for (IP_ADAPTER_GATEWAY_ADDRESS* gw = aa->FirstGatewayAddress; gw != NULL; gw = gw->Next) {
        if (gw->Address.lpSockaddr && gw->Address.lpSockaddr->sa_family == AF_INET) {
            struct sockaddr_in* sin = (struct sockaddr_in*)gw->Address.lpSockaddr;
            if (pubnub_ipv4_address_valid(ntohl(sin->sin_addr.S_un.S_addr)))
                return true;
        }
    }

    return false;
}

/** Retrieve DNS servers from network adapters, prioritized by routing metrics.
*
 *  @param o_ipv4 Pointer to the array of @c pubnub_ipv4_address structures with
 *  previously discovered DNS server addresses.
 *  @param n Maximum number of discovered DNS server addresses.
 *  @return Number of discovered system DNS servers, @c zero otherwise.
 */
int pubnub_dns_read_system_servers_ipv4(
    struct pubnub_ipv4_address* o_ipv4,
    size_t n)
{
    if (!o_ipv4 || n == 0) return 0;

    IP_ADAPTER_ADDRESSES* addrs = NULL;
    if (!pubnub_adapter_addresses_get(&addrs)) return 0;

    /* Get the best interface index for routing to public Internet. */
    DWORD best_if_index = pubnub_get_best_interface_index();

    /* Count suitable adapters. */
    size_t adapter_count = 0;
    for (IP_ADAPTER_ADDRESSES* aa = addrs; aa != NULL; aa = aa->Next) {
        if (pubnub_adapter_is_suitable(aa)) adapter_count++;
    }

    if (adapter_count == 0) {
        FREE(addrs);
        return 0;
    }

    /* Build array of adapters with their metrics for sorting. */
    struct adapter_with_metric* adapter_list =
        (struct adapter_with_metric*)MALLOC(adapter_count * sizeof(struct adapter_with_metric));

    if (!adapter_list) {
        FREE(addrs);
        return 0;
    }

    size_t idx = 0;
    for (IP_ADAPTER_ADDRESSES* aa = addrs; aa != NULL; aa = aa->Next) {
        if (pubnub_adapter_is_suitable(aa)) {
            adapter_list[idx].adapter = aa;
            adapter_list[idx].metric = aa->Ipv4Metric;
            adapter_list[idx].is_best_route =
                (best_if_index != 0 && aa->IfIndex == best_if_index);
            idx++;
        }
    }

    /* Sort adapters by priority. */
    qsort(adapter_list, adapter_count, sizeof(struct adapter_with_metric),
          compare_adapters_by_priority);

    /* Extract DNS servers from adapters in priority order. */
    unsigned j = 0;
    for (size_t i = 0; i < adapter_count && j < n; i++) {
        IP_ADAPTER_ADDRESSES* aa = adapter_list[i].adapter;

        PUBNUB_LOG_TRACE("Processing adapter: Index=%lu, Metric=%lu, IsBestRoute=%d\n",
                         (unsigned long)aa->IfIndex,
                         (unsigned long)adapter_list[i].metric,
                         adapter_list[i].is_best_route);

        for (IP_ADAPTER_DNS_SERVER_ADDRESS* ds = aa->FirstDnsServerAddress; ds && j < n; ds = ds->Next) {
            if (!ds->Address.lpSockaddr || ds->Address.lpSockaddr->sa_family != AF_INET)
                continue;

            const struct sockaddr_in* sin = (const struct sockaddr_in*)ds->Address.lpSockaddr;
            DWORD net_addr = sin->sin_addr.S_un.S_addr;
            if (net_addr == 0) continue;

            /* Verifying adapter can be used to reach DNS server. */
            if (!pubnub_dns_reachable_from_adapter(net_addr, aa)) {
                PUBNUB_LOG_DEBUG("Skipping DNS server - not reachable from "
                                 "adapter\n");
                continue;
            }

            /* Verify DNS server is actually reachable via TCP connection test.
             * This detects stale VPN connections where the adapter appears UP but
             * the DNS server is unreachable. This is critical because GetBestInterface
             * may return a stale VPN adapter if Windows routing table hasn't been updated.
             * We test ALL adapters to catch this scenario.
             */
            if (!pubnub_dns_server_reachable(net_addr)) {
                PUBNUB_LOG_WARNING("Skipping DNS server - TCP connection test "
                                   "failed (possibly stale VPN)\n");
                continue;
            }

            /* Add to output if not duplicate */
            if (pubnub_copy_ipv4_bytes_from_be_dword(o_ipv4, j, net_addr, o_ipv4[j].ipv4)) {
                PUBNUB_LOG_TRACE("Added DNS server from adapter index %lu\n",
                                 (unsigned long)aa->IfIndex);
                ++j;
            }
        }
    }

    FREE(adapter_list);
    FREE(addrs);

    return (int)j;
}
