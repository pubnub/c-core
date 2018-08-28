#include "pubnub_internal.h"
#include "core/pubnub_assert.h"
#include "core/pubnub_log.h"
#include "core/pubnub_dns_servers.h"
#include "lib/pubnub_parse_ipv4_addr.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

int pubnub_dns_read_system_servers_ipv4(struct pubnub_ipv4_address* o_ipv4, size_t n)
{
    FILE* fp;
    char  buffer[255];
    unsigned  i = 0;
    bool  found = false;

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
            if (pubnub_parse_ipv4_addr(buffer + 10, &o_ipv4[i]) != 0) {
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
