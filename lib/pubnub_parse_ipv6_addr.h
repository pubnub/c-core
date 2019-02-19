/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_PARSE_IPV6_ADDR
#define INC_PUBNUB_PARSE_IPV6_ADDR
#include "core/pubnub_dns_servers.h"

/** Parses Ipv6 address string @p addr and, in case it succeeds, resolved value stores at @p p.
    @retval -1 on error
    @retval 0 on success
  */
int pubnub_parse_ipv6_addr(char const* addr, struct pubnub_ipv6_address* p);

#endif
