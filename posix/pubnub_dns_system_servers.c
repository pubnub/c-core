#include "pubnub_internal.h"
#include "core/pubnub_assert.h"
#include "core/pubnub_log.h"
#include "core/pubnub_dns_servers.h"
#include "lib/pubnub_parse_ipv4_addr.h"
#if PUBNUB_USE_IPV6
#include "lib/pubnub_parse_ipv6_addr.h"
#endif

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

int pubnub_dns_read_system_servers_ipv4(struct pubnub_ipv4_address* o_ipv4, size_t n)
{
    FILE*    fp;
    char     buffer[255];
    unsigned i     = 0;
    bool     found = false;

    PUBNUB_ASSERT_OPT(n > 0);
    PUBNUB_ASSERT_OPT(o_ipv4 != NULL);

    fp = fopen("/etc/resolv.conf", "r");
    if (NULL == fp) {
        PUBNUB_LOG_ERROR(
            "Can't open file:'/etc/resolv.conf' for reading!-errno:%d\n", errno);
        return -1;
    }
    while ((i < n) && !feof(fp)) {
        /* Reads new line */
        fgets(buffer, sizeof buffer, fp);
        if (strncmp(buffer, "nameserver", 10) == 0) {
            char* addr_start = buffer + 10;
            while (*addr_start == ' ' || *addr_start == '\t') {
                addr_start++;
            }
            char* end = addr_start;
            while (*end && *end != ' ' && *end != '\n' && *end != '\r' && *end != '\t') {
                end++;
            }
            *end = '\0';

            if (pubnub_parse_ipv4_addr(addr_start, &o_ipv4[i]) != 0) {
                PUBNUB_LOG_ERROR(
                    "pubnub_dns_read_system_servers_ipv4():"
                    "- ipv4 'numbers-and-dots' notation string(%s)"
                    "read from file:'/etc/resolv.conf' is not valid!\n",
                    buffer);
            }
            else {
                found = true;
                ++i;
            }
        }
    }
    if (fclose(fp) != 0) {
        PUBNUB_LOG_ERROR("Error closing file: '/etc/resolv.conf'! errno:%d\n",
                         errno);
        return -1;
    }
    if (!found) {
        PUBNUB_LOG_ERROR(
            "Couldn't find system servers ipv4 in file:'/etc/resolv.conf'!");
        return -1;
    }

    return i;
}

#if PUBNUB_USE_IPV6
int pubnub_dns_read_system_servers_ipv6(struct pubnub_ipv6_address* o_ipv6, size_t n)
{
    FILE*    fp;
    char     buffer[255];
    unsigned dns_count          = 0;
    bool     has_ipv4_addresses = false;

    PUBNUB_ASSERT_OPT(n > 0);
    PUBNUB_ASSERT_OPT(o_ipv6 != NULL);

    fp = fopen("/etc/resolv.conf", "r");
    if (NULL == fp) {
        PUBNUB_LOG_ERROR(
            "Can't open file:'/etc/resolv.conf' for reading!-errno:%d\n", errno);
        return -1;
    }
    while ((dns_count < n) && !feof(fp)) {
        /* Reads new line */
        fgets(buffer, sizeof buffer, fp);
        if (strncmp(buffer, "nameserver", 10) == 0) {
            bool     is_ipv4_address = false;
            uint8_t* dns_bytes       = NULL;
            char*    addr_start      = buffer + 10;
            while (*addr_start == ' ' || *addr_start == '\t') {
                addr_start++;
            }

            char* end = addr_start;
            while (*end && *end != ' ' && *end != '\n' && *end != '\r' && *end != '\t') {
                end++;
            }
            *end = '\0';

            /* Try parsing as IPv6 first */
            if (pubnub_parse_ipv6_addr(addr_start, &o_ipv6[dns_count]) == 0) {
                dns_bytes = o_ipv6[dns_count].ipv6;
                ++dns_count;
            }
            else {
                struct pubnub_ipv4_address ipv4_addr;
                if (pubnub_parse_ipv4_addr(addr_start, &ipv4_addr) == 0) {
                    has_ipv4_addresses = true;
                    is_ipv4_address    = true;
                    memset(o_ipv6[dns_count].ipv6, 0, 10);
                    o_ipv6[dns_count].ipv6[10] = 0xff;
                    o_ipv6[dns_count].ipv6[11] = 0xff;
                    memcpy(&o_ipv6[dns_count].ipv6[12], ipv4_addr.ipv4, 4);
                    dns_bytes = o_ipv6[dns_count].ipv6;
                    ++dns_count;
                }
                else {
                    PUBNUB_LOG_WARNING("pubnub_dns_read_system_servers_ipv6(): "
                                       "Invalid nameserver address '%s' in "
                                       "/etc/resolv.conf, skipping.\n",
                                       addr_start);
                }
            }

            if (NULL != dns_bytes) {
                char str[INET6_ADDRSTRLEN];
                PUBNUB_LOG_TRACE(
                    "Added %s DNS (%s).\n",
                    is_ipv4_address ? "IPv4-mapped IPv6" : "IPv6",
                    inet_ntop(AF_INET6, dns_bytes, str, INET6_ADDRSTRLEN));
            }
        }
    }
    if (fclose(fp) != 0) {
        PUBNUB_LOG_ERROR("Error closing file: '/etc/resolv.conf'! errno:%d\n",
                         errno);
        return -1;
    }
    if (0 == dns_count) {
        PUBNUB_LOG_WARNING(
            "Couldn't find any DNS servers in file:'/etc/resolv.conf'!");
        return 0;
    }

    PUBNUB_LOG_DEBUG("Discovered %u %s DNS server(s)\n",
                     dns_count,
                     has_ipv4_addresses ? "IPv4-mapped IPv6/IPv6" : "IPv6");

    return dns_count;
}
#endif /* PUBNUB_USE_IPV6 */
