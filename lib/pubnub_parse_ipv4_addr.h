#if !defined INC_PUBNUB_PARSE_IPV4_ADDR
#define INC_PUBNUB_PARSE_IPV4_ADDR
#include "core/pubnub_dns_servers.h"

int pubnub_parse_ipv4_addr(char const* addr, struct pubnub_ipv4_address* p);

#endif
