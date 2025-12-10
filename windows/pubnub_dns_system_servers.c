/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
/**
 * @file pubnub_dns_system_servers.c
 * @brief Windows DNS server discovery using GetAdaptersAddresses.
 *
 * This implementation is designed for Windows 8+ and provides:
 * - Thread-safe DNS server discovery
 * - Proper handling of stale VPN connections via DNS validation
 * - IPv4 and IPv6 support with GetBestRoute2
 * - Adapter prioritization based on routing metrics
 *
 * Minimum Windows version: Windows 8 (NT 6.2)
 */

#if PUBNUB_USE_SSL
#include "openssl/pubnub_internal.h"
#else
#include "pubnub_internal.h"
#endif
#include "core/pubnub_dns_servers.h"
#include "core/pubnub_assert.h"
#include "core/pubnub_log.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <windows.h>
#include <string.h>
#include <stdlib.h>

#pragma comment(lib, "IPHLPAPI.lib")
#pragma comment(lib, "ws2_32.lib")

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

/** Maximum retry attempts for GetAdaptersAddresses */
#define MAX_ADAPTER_ADDRESS_RETRIES 3
/** Maximum number of DNS servers per adapter */
#define MAX_DNS_SERVERS_PER_ADAPTER 8

/** DNS server validation timeout in milliseconds.
 *  DNS servers will be validated only if timeout is larget than @b 0.
 */
#ifndef PUBNUB_DNS_SERVERS_VALIDATION_TIMEOUT
#define PUBNUB_DNS_SERVERS_VALIDATION_TIMEOUT 0
#endif /* PUBNUB_DNS_SERVERS_VALIDATION_TIMEOUT */

/** Adapter info for sorting with verified DNS servers. */
struct adapter_info {
    IP_ADAPTER_ADDRESSES*   adapter;
    ULONG                   metric;
    bool                    is_best_route;
    size_t                  dns_count;
    struct sockaddr_storage dns_servers[MAX_DNS_SERVERS_PER_ADAPTER];
};

/** Check if @c IPv4 address is valid (not loopback, APIPA, or zero).
 *
 *  @param addr IPv4 address to check (4 bytes, network order).
 *  @return @c true if valid, @c false otherwise.
 */
static bool is_valid_ipv4(const uint8_t addr[4])
{
    if (addr[0] == 0 || addr[0] == 127 || (addr[0] == 169 && addr[1] == 254))
        return false;

    return true;
}

#if PUBNUB_USE_IPV6
/** Check if @c IPv6 address is valid (not loopback, or zero).
 *
 *  @param addr IPv6 address to check (16 bytes, network order).
 *  @return @c true if valid, @c false otherwise.
 */
static bool is_valid_ipv6(const uint8_t addr[16])
{
    bool loopback = addr[15] == 1;
    bool all_zero = true;

    for (int i = 0; i < 16 && all_zero; i++) {
        if (addr[i] != 0)
            all_zero = false;
    }

    for (int i = 0; i < 15 && loopback; i++) {
        if (addr[i] != 0)
            loopback = false;
    }

    if (all_zero || loopback || (addr[0] == 0xfe && (addr[1] & 0xc0) == 0x80))
        return false;

    return true;
}
#endif /* PUBNUB_USE_IPV6 */

/** Check address is valid (not loopback, APIPA, or zero) for specified family.
 *
 *  @param family @c AF_INET or @c AF_INET6 family.
 *  @param addr Address to check (4 bytes for @c IPv4, 16 bytes for @c IPv6,
 *              network order).
 *  @return @c true if valid, @c false otherwise.
 */
static bool is_valid_address(ULONG family, const uint8_t* addr)
{
    if (family == AF_INET)
        return is_valid_ipv4(addr);
#if PUBNUB_USE_IPV6
    if (family == AF_INET6)
        return is_valid_ipv6(addr);
#endif /* PUBNUB_USE_IPV6 */
    return false;
}

/** Check if adapter is suitable for DNS queries (unified for IPv4/IPv6).
 *
 *  @param family Address family (@c AF_INET or @c AF_INET6).
 *  @param aa Adapter to check.
 *  @return @c true if suitable.
 */
static bool is_adapter_suitable(ULONG family, IP_ADAPTER_ADDRESSES* aa)
{
    if (!aa || aa->OperStatus != IfOperStatusUp
        || aa->IfType == IF_TYPE_SOFTWARE_LOOPBACK)
        return false;

    if (AF_INET == family && !(aa->Flags & IP_ADAPTER_IPV4_ENABLED))
        return false;

#if PUBNUB_USE_IPV6
    if (AF_INET6 == family && !(aa->Flags & IP_ADAPTER_IPV6_ENABLED))
        return false;
#endif /* PUBNUB_USE_IPV6 */

    /* For IPv4: check if it has valid gateway */
    if (AF_INET == family) {
        for (IP_ADAPTER_GATEWAY_ADDRESS* gw = aa->FirstGatewayAddress; gw != NULL;
             gw = gw->Next) {
            if (gw->Address.lpSockaddr
                && gw->Address.lpSockaddr->sa_family == family) {
                struct sockaddr_in* sin =
                    (struct sockaddr_in*)gw->Address.lpSockaddr;
                const uint8_t* bytes = (uint8_t*)&sin->sin_addr.s_addr;
                if (is_valid_address(AF_INET, bytes))
                    return true;
            }
        }

        /* Accept physical Ethernet or Wi-Fi adapters with valid unicast address.
         * This handles WSL2, Hyper-V, Docker scenarios where gateway info may
         * be missing but the adapter is still functional for local DNS.
         *
         * Skip adapter in other case without unicast address verification.
         */
        if (aa->IfType != IF_TYPE_ETHERNET_CSMACD
            && aa->IfType != IF_TYPE_IEEE80211 && aa->IfType != 71)
            return false;
    }

    /* Must have at least one valid unicast address */
    /* For IPv4: required if no gateway, for IPv6: always required */
    for (IP_ADAPTER_UNICAST_ADDRESS* ua = aa->FirstUnicastAddress; ua != NULL;
         ua                             = ua->Next) {
        if (!ua->Address.lpSockaddr || ua->Address.lpSockaddr->sa_family != family)
            continue;

        uint8_t* bytes = NULL;
        if (AF_INET == family) {
            struct sockaddr_in* sin = (struct sockaddr_in*)ua->Address.lpSockaddr;
            bytes = (uint8_t*)&sin->sin_addr.s_addr;
        }
#if PUBNUB_USE_IPV6
        else if (AF_INET6 == family) {
            struct sockaddr_in6* sin6 = (struct sockaddr_in6*)ua->Address.lpSockaddr;
            bytes = (uint8_t*)&sin6->sin6_addr;
        }
#endif /* PUBNUB_USE_IPV6 */

        if (is_valid_address(family, bytes))
            return true;
    }

    return false;
}

/** Retrieve adapter addresses for specified address family.
 *
 *  @param family Address family (@c AF_INET or @c AF_INET6).
 *  @return Pointer to adapter addresses on success, @c NULL on error.
 *          Caller must @c FREE the returned pointer.
 */
static IP_ADAPTER_ADDRESSES* get_adapter_addresses(ULONG family)
{
    IP_ADAPTER_ADDRESSES* addrs = NULL;
    ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST
                  | GAA_FLAG_INCLUDE_GATEWAYS;
    ULONG buflen  = 15000;
    int   retries = 0;
    DWORD ret;

    do {
        addrs = (IP_ADAPTER_ADDRESSES*)MALLOC(buflen);
        if (!addrs) {
            PUBNUB_LOG_ERROR(
                "Failed to allocate %lu bytes for adapter info (family=%lu)\n",
                (unsigned long)buflen,
                family);
            return NULL;
        }

        ret = GetAdaptersAddresses(family, flags, NULL, addrs, &buflen);

        if (ret == NO_ERROR)
            break;

        FREE(addrs);
        addrs = NULL;

        if (ret != ERROR_BUFFER_OVERFLOW
            || ++retries >= MAX_ADAPTER_ADDRESS_RETRIES) {
            PUBNUB_LOG_ERROR("GetAdaptersAddresses(family=%lu) failed: %lu\n",
                             (unsigned long)family,
                             (unsigned long)ret);
            return NULL;
        }
    } while (ret == ERROR_BUFFER_OVERFLOW);

    return addrs;
}


#if PUBNUB_DNS_SERVERS_VALIDATION_TIMEOUT
/** Validate DNS server by sending a UDP probe packet.
 *
 *  @param addr_family Address family (@c AF_INET or @c AF_INET6).
 *  @param dns_addr DNS server address (4 bytes for @c IPv4, 16 bytes for
 *         @c IPv6, network order).
 *  @param if_index Interface index to bind query to (0 for any).
 *  @return @c true if DNS server responds successfully.
 */
static bool validate_dns_server_udp(int           addr_family,
                                    const uint8_t dns_addr[],
                                    DWORD         if_index)
{
    /* Minimal DNS query packet */
    const uint8_t query[12] = { 0x00, 0x01, 0x01, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

    SOCKET sock = socket(addr_family, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        PUBNUB_LOG_ERROR("Failed to create validation socket: %d\n",
                         WSAGetLastError());
        return false;
    }

    /* Set receive timeout */
    DWORD timeout_ms = PUBNUB_DNS_SERVERS_VALIDATION_TIMEOUT;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout_ms, sizeof(timeout_ms))
        != 0) {
        PUBNUB_LOG_WARNING("Failed to set socket timeout: %d\n", WSAGetLastError());
    }

    /* Prepare DNS server address */
    struct sockaddr_storage dns_server;
    ZeroMemory(&dns_server, sizeof(dns_server));
    int addr_len;
    if (addr_family == AF_INET) {
        struct sockaddr_in* sin = (struct sockaddr_in*)&dns_server;
        sin->sin_family         = AF_INET;
        sin->sin_port           = htons(53);
        memcpy(&sin->sin_addr.s_addr, dns_addr, 4);
        addr_len = sizeof(struct sockaddr_in);
        PUBNUB_LOG_TRACE("Validating DNS server: %u.%u.%u.%u\n",
                         dns_addr[0],
                         dns_addr[1],
                         dns_addr[2],
                         dns_addr[3]);
    }
#if PUBNUB_USE_IPV6
    else if (addr_family == AF_INET6) {
        struct sockaddr_in6* sin6 = (struct sockaddr_in6*)&dns_server;
        sin6->sin6_family         = AF_INET6;
        sin6->sin6_port           = htons(53);
        memcpy(sin6->sin6_addr.s6_addr, dns_addr, 16);
        addr_len = sizeof(struct sockaddr_in6);
        PUBNUB_LOG_TRACE(
            "Validating DNS server: %02x%02x:%02x%02x:%02x%02x:%02x%02x:"
            "%02x%02x:%02x%02x:%02x%02x:%02x%02x\n",
            dns_addr[0],
            dns_addr[1],
            dns_addr[2],
            dns_addr[3],
            dns_addr[4],
            dns_addr[5],
            dns_addr[6],
            dns_addr[7],
            dns_addr[8],
            dns_addr[9],
            dns_addr[10],
            dns_addr[11],
            dns_addr[12],
            dns_addr[13],
            dns_addr[14],
            dns_addr[15]);
    }
#endif
    else {
        PUBNUB_LOG_ERROR("Invalid address family: %d\n", addr_family);
        socket_close(sock);
        return false;
    }

    /* Send minimal DNS query to probe server */
    int sent = sendto(
        sock, (char*)query, sizeof(query), 0, (struct sockaddr*)&dns_server, addr_len);

    if (sent <= 0) {
        PUBNUB_LOG_DEBUG("DNS validation sendto() failed: %d\n", WSAGetLastError());
        socket_close(sock);
        return false;
    }

    /* Try to receive any response (even error response means server is alive) */
    uint8_t response[512];
    int recvd = recvfrom(sock, (char*)response, sizeof(response), 0, NULL, NULL);

    if (recvd > 0) {
        PUBNUB_LOG_TRACE("DNS server responded (%d bytes) - validation OK\n", recvd);
        socket_close(sock);
        return true;
    }

    int err = WSAGetLastError();
    if (err == WSAETIMEDOUT) {
        PUBNUB_LOG_DEBUG("DNS validation timeout - server unreachable\n");
    }
    else {
        PUBNUB_LOG_DEBUG("DNS validation recvfrom() failed: %d\n", err);
    }

    socket_close(sock);
    return false;
}
#endif /* PUBNUB_DNS_SERVERS_VALIDATION_TIMEOUT */

/** Get best interface index for public Internet routing using GetBestRoute2.
 *
 *  @param family Address family (@c AF_INET or @c AF_INET6).
 *  @return Interface index, or @c 0 if index cannot determine.
 */
static DWORD get_best_interface_index(ADDRESS_FAMILY family)
{
    SOCKADDR_INET dest_addr;
    SOCKADDR_INET source_addr;
    ZeroMemory(&dest_addr, sizeof(dest_addr));
    ZeroMemory(&source_addr, sizeof(source_addr));
    dest_addr.si_family = family;

    if (family == AF_INET) {
        if (inet_pton(AF_INET, "8.8.8.8", &dest_addr.Ipv4.sin_addr) != 1)
            return 0;
    }
#if PUBNUB_USE_IPV6
    else if (family == AF_INET6) {
        if (inet_pton(AF_INET6, "2001:4860:4860::8888", &dest_addr.Ipv6.sin6_addr)
            != 1)
            return 0;
    }
#endif
    else {
        return 0;
    }

    MIB_IPFORWARD_ROW2 route;
    DWORD ret = GetBestRoute2(NULL, 0, NULL, &dest_addr, 0, &route, &source_addr);

    if (ret != NO_ERROR) {
        PUBNUB_LOG_DEBUG("GetBestRoute2(family=%lu) failed: %lu\n",
                         family,
                         (unsigned long)ret);
        return 0;
    }

    NET_LUID    luid = route.InterfaceLuid;
    NET_IFINDEX if_index;
    ret = ConvertInterfaceLuidToIndex(&luid, &if_index);

    if (ret != NO_ERROR) {
        PUBNUB_LOG_DEBUG(
            "ConvertInterfaceLuidToIndex(family=%lu) failed: %lu\n",
            family,
            (unsigned long)ret);
        return 0;
    }

    PUBNUB_LOG_TRACE("Best interface index for %s: %lu\n",
                     family == AF_INET ? "IPv4" : "IPv6",
                     (unsigned long)if_index);

    return (DWORD)if_index;
}


/** Comparison function for qsort - prioritize adapters. */
static int compare_adapter_priority(const void* a, const void* b)
{
    const struct adapter_info* aa = (const struct adapter_info*)a;
    const struct adapter_info* bb = (const struct adapter_info*)b;

    /* Best route adapters first */
    if (aa->is_best_route && !bb->is_best_route)
        return -1;
    if (!aa->is_best_route && bb->is_best_route)
        return 1;

    /* Then by metric (lower is better) */
    if (aa->metric < bb->metric)
        return -1;
    if (aa->metric > bb->metric)
        return 1;

    return 0;
}

/** Process provided list of adapter addresses to find useful DNS server
 *  addresses.
 *
 *  @param family Adapter address family (@c AF_INET or @c AF_INET6).
 *  @param addrs List of @c family adapter addresses.
 *  @param n Maximum number of required DNS server addresses.
 *  @param[out] count Number of verified adapters with DNS servers.
 *  @return List of @c adapter_info with suitable adapters.
 */
struct adapter_info* pubnub_dns_read_system_servers(ULONG family,
                                                    IP_ADAPTER_ADDRESSES* addrs,
                                                    const size_t          n,
                                                    size_t*               count)
{
    if (!addrs)
        return NULL;

    size_t adapter_count = 0;
    for (IP_ADAPTER_ADDRESSES* aa = addrs; aa != NULL; aa = aa->Next) {
        adapter_count++;
    }

    if (adapter_count == 0) {
        PUBNUB_LOG_WARNING("No suitable adapters found (family=%lu)\n", family);
        *count = 0;
        return NULL;
    }

    struct adapter_info* adapter_list =
        (struct adapter_info*)MALLOC(adapter_count * sizeof(struct adapter_info));
    if (!adapter_list) {
        *count = 0;
        return NULL;
    }

    DWORD  best_if_index = get_best_interface_index(family);
    size_t idx           = 0;
    for (IP_ADAPTER_ADDRESSES* aa = addrs; aa != NULL; aa = aa->Next) {
        if (is_adapter_suitable(family, aa)) {
            adapter_list[idx].adapter = aa;
            adapter_list[idx].metric =
                AF_INET == family ? aa->Ipv4Metric : aa->Ipv6Metric;
            adapter_list[idx].is_best_route =
                (best_if_index != 0 && aa->IfIndex == best_if_index);
            adapter_list[idx].dns_count = 0;
            idx++;
        }
    }

    adapter_count = idx;
    if (adapter_count == 0) {
        PUBNUB_LOG_WARNING("No suitable adapters found (family=%lu)\n", family);
        FREE(adapter_list);
        *count = 0;
        return NULL;
    }

    qsort(adapter_list,
          adapter_count,
          sizeof(struct adapter_info),
          compare_adapter_priority);

    size_t total_dns_count = 0;
    for (size_t i = 0; i < adapter_count && total_dns_count < n; i++) {
        IP_ADAPTER_ADDRESSES* aa = adapter_list[i].adapter;

        PUBNUB_LOG_TRACE(
            "Processing adapter idx=%lu metric=%lu best=%d (family=%lu)\n",
            (unsigned long)aa->IfIndex,
            (unsigned long)adapter_list[i].metric,
            adapter_list[i].is_best_route,
            family);

        for (IP_ADAPTER_DNS_SERVER_ADDRESS* ds = aa->FirstDnsServerAddress;
             ds && adapter_list[i].dns_count < MAX_DNS_SERVERS_PER_ADAPTER
             && total_dns_count < n;
             ds = ds->Next) {

            if (!ds->Address.lpSockaddr)
                continue;

            ULONG dns_family = ds->Address.lpSockaddr->sa_family;

            /* IPv4 can't handle IPv6 addresses and should skip such. */
            if (AF_INET == family && dns_family != family)
                continue;

            uint8_t* dns_bytes = NULL;

            if (AF_INET == dns_family) {
                struct sockaddr_in* sin =
                    (struct sockaddr_in*)ds->Address.lpSockaddr;
                dns_bytes = (uint8_t*)&sin->sin_addr.s_addr;
            }
#if PUBNUB_USE_IPV6
            else if (AF_INET6 == dns_family) {
                struct sockaddr_in6* sin6 =
                    (struct sockaddr_in6*)ds->Address.lpSockaddr;
                dns_bytes = (uint8_t*)&sin6->sin6_addr;
            }
#endif /* PUBNUB_USE_IPV6 */

            if (!dns_bytes || !is_valid_address(dns_family, dns_bytes))
                continue;

            /* Check for duplicates across all already verified DNS servers */
            bool is_duplicate = false;
            for (size_t j = 0; j <= i; j++) {
                for (size_t k = 0; k < adapter_list[j].dns_count; k++) {
                    struct sockaddr_storage* stored =
                        &adapter_list[j].dns_servers[k];
                    if (AF_INET == dns_family && stored->ss_family == AF_INET) {
                        struct sockaddr_in* stored_sin = (struct sockaddr_in*)stored;
                        if (memcmp(&stored_sin->sin_addr.s_addr, dns_bytes, 4) == 0) {
                            is_duplicate = true;
                            break;
                        }
                    }
#if PUBNUB_USE_IPV6
                    else if (AF_INET6 == dns_family
                             && stored->ss_family == AF_INET6) {
                        struct sockaddr_in6* stored_sin6 =
                            (struct sockaddr_in6*)stored;
                        if (memcmp(&stored_sin6->sin6_addr, dns_bytes, 16) == 0) {
                            is_duplicate = true;
                            break;
                        }
                    }
#endif
                }
                if (is_duplicate)
                    break;
            }

            /* Skipping already added DNS server. */
            if (is_duplicate)
                continue;

#if PUBNUB_DNS_SERVERS_VALIDATION_TIMEOUT
            if (!validate_dns_server_udp(dns_family, dns_bytes, aa->IfIndex)) {
                PUBNUB_LOG_WARNING(
                    "Skipping DNS - validation failed (family=%lu)\n", family);
                continue;
            }
#endif /* PUBNUB_DNS_SERVERS_VALIDATION_TIMEOUT */

            /* Store the verified DNS server */
            struct sockaddr_storage* verified =
                &adapter_list[i].dns_servers[adapter_list[i].dns_count];
            ZeroMemory(verified, sizeof(struct sockaddr_storage));

            if (AF_INET == dns_family) {
                struct sockaddr_in* sin_out = (struct sockaddr_in*)verified;
                sin_out->sin_family         = AF_INET;
                sin_out->sin_port           = htons(53);
                memcpy(&sin_out->sin_addr.s_addr, dns_bytes, 4);
            }
#if PUBNUB_USE_IPV6
            else if (AF_INET6 == dns_family) {
                struct sockaddr_in6* sin6_out = (struct sockaddr_in6*)verified;
                sin6_out->sin6_family         = AF_INET6;
                sin6_out->sin6_port           = htons(53);
                memcpy(&sin6_out->sin6_addr, dns_bytes, 16);
            }
#endif /* PUBNUB_USE_IPV6 */

            adapter_list[i].dns_count++;
            total_dns_count++;
        }
    }

    *count = adapter_count;
    return adapter_list;
}


/**
 * Read system DNS servers for IPv4.
 *
 * Thread-safe implementation using GetAdaptersAddresses with optional
 * DNS validation to filter out stale VPN servers.
 *
 * @param o_ipv4 Output array (must be allocated by caller).
 * @param n Maximum number of DNS servers to return.
 * @return Number of DNS servers found, or -1 on error.
 */
int pubnub_dns_read_system_servers_ipv4(struct pubnub_ipv4_address* o_ipv4, size_t n)
{
    PUBNUB_ASSERT_OPT(o_ipv4 != NULL);
    PUBNUB_ASSERT_OPT(n > 0);

    if (!o_ipv4 || n == 0)
        return -1;

    IP_ADAPTER_ADDRESSES* addrs = get_adapter_addresses(AF_INET);
    if (!addrs)
        return -1;

    size_t               count = 0;
    struct adapter_info* adapter_list =
        pubnub_dns_read_system_servers(AF_INET, addrs, n, &count);
    if (!adapter_list || 0 == count) {
        FREE(addrs);
        if (adapter_list)
            FREE(adapter_list);
        return -1;
    }

    unsigned dns_count = 0;
    for (size_t i = 0; i < count; i++) {
        if (!adapter_list[i].dns_count)
            continue;
        for (size_t d = 0; d < adapter_list[i].dns_count; d++) {
            struct sockaddr_in* sin =
                (struct sockaddr_in*)&adapter_list[i].dns_servers[d];
            uint8_t* dns_bytes = (uint8_t*)&sin->sin_addr.s_addr;
            memcpy(o_ipv4[dns_count].ipv4, dns_bytes, 4);

            PUBNUB_LOG_TRACE("Added IPv4 DNS: %u.%u.%u.%u\n",
                             dns_bytes[0],
                             dns_bytes[1],
                             dns_bytes[2],
                             dns_bytes[3]);
            dns_count++;
        }
    }

    FREE(adapter_list);
    FREE(addrs);

    if (dns_count == 0) {
        PUBNUB_LOG_ERROR("No valid DNS servers found\n");
        return -1;
    }

    PUBNUB_LOG_DEBUG("Discovered %u DNS server(s)\n", dns_count);
    return (int)dns_count;
}


#if PUBNUB_USE_IPV6
int pubnub_dns_read_system_servers_ipv6(struct pubnub_ipv6_address* o_ipv6, size_t n)
{
    PUBNUB_ASSERT_OPT(o_ipv6 != NULL);
    PUBNUB_ASSERT_OPT(n > 0);

    if (!o_ipv6 || n == 0)
        return -1;

    IP_ADAPTER_ADDRESSES* addrs = get_adapter_addresses(AF_INET6);
    if (!addrs)
        return -1;

    size_t               count = 0;
    struct adapter_info* adapter_list =
        pubnub_dns_read_system_servers(AF_INET6, addrs, n, &count);
    if (!adapter_list || 0 == count) {
        FREE(addrs);
        if (adapter_list)
            FREE(adapter_list);
        return -1;
    }

    bool     has_ipv4_addresses = false;
    unsigned dns_count          = 0;
    for (size_t i = 0; i < count; i++) {
        if (!adapter_list[i].dns_count)
            continue;
        for (size_t d = 0; d < adapter_list[i].dns_count; d++) {
            struct sockaddr_storage* stored = &adapter_list[i].dns_servers[d];
            uint8_t                  dns_bytes_buffer[16];
            uint8_t*                 dns_bytes = NULL;

            if (AF_INET == stored->ss_family) {
                struct sockaddr_in* sin = (struct sockaddr_in*)stored;
                memset(dns_bytes_buffer, 0, 10);
                dns_bytes_buffer[10] = 0xff;
                dns_bytes_buffer[11] = 0xff;
                memcpy(&dns_bytes_buffer[12], &sin->sin_addr.s_addr, 4);
                dns_bytes          = dns_bytes_buffer;
                has_ipv4_addresses = true;
            }
            else {
                struct sockaddr_in6* sin6 = (struct sockaddr_in6*)stored;
                dns_bytes                 = (uint8_t*)&sin6->sin6_addr;
            }
            memcpy(o_ipv6[dns_count].ipv6, dns_bytes, 16);

            char str[INET6_ADDRSTRLEN];
            PUBNUB_LOG_TRACE(
                "Added %s DNS (%s).\n",
                AF_INET == stored->ss_family ? "IPv4-mapped IPv6" : "IPv6",
                inet_ntop(AF_INET6, dns_bytes, str, INET6_ADDRSTRLEN));
            dns_count++;
        }
    }

    FREE(adapter_list);
    FREE(addrs);

    if (dns_count == 0) {
        PUBNUB_LOG_WARNING("No IPv6 DNS servers found\n");
        return 0;
    }

    PUBNUB_LOG_DEBUG("Discovered %u %s DNS server(s)\n",
                     dns_count,
                     has_ipv4_addresses ? "IPv4-mapped IPv6/IPv6" : "IPv6");
    return (int)dns_count;
}
#endif /* PUBNUB_USE_IPV6 */
