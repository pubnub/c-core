/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "core/pubnub_assert.h"
#include "core/pubnub_dns_servers.h"

#include <stdio.h>
#include <string.h>

int pubnub_parse_ipv4_addr(char const* addr, struct pubnub_ipv4_address* p)
{
    int a[4];
    int rslt;

    PUBNUB_ASSERT_OPT(addr != NULL);
    PUBNUB_ASSERT_OPT(p != NULL);

    if (strchr(addr, '.') == NULL) {
        return -1;
    }
    rslt = sscanf(addr, "%d.%d.%d.%d", a, a + 1, a + 2, a + 3);
    if (rslt != 4) {
        return -1;
    }
    for (rslt--; rslt >= 0; --rslt) {
        if ((a[rslt] < 0) || (a[rslt] > 255)) {
            return -1;
        }
        p->ipv4[rslt] = a[rslt];
    }
    return 0;
}
