#include "pubnub_internal.h"
#include "core/pubnub_assert.h"
#include "core/pubnub_log.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


int pubnub_dns_read_system_servers_ipv4(struct pubnub_ipv4_address* o_ipv4, size_t n)
{
    PUBNUB_UNUSED(o_ipv4);
    PUBNUB_UNUSED(n);
    return -1;
}
