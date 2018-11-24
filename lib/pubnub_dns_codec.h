/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_DNS_HANDLER
#define      INC_PUBNUB_DNS_HANDLER

#include "core/pubnub_dns_servers.h"

#include <stdlib.h>

/** Prepares DNS query request for @p host(name) in @p buf whose maximum available length is
    @p buf_size in octets.
    If function succeedes, @p to_send 'carries' the length of prepared message.
    If function reports en error, @p to_send 'keeps' the length of successfully prepared segment
    before error occurred.
    
    @retval 0 success, -1 on error
 */
int pubnub_prepare_dns_request(uint8_t* buf, size_t buf_size, char const* host, int *to_send);

/** Picks valid resolved domain name address from the response from DNS server.
    @p buf points to the beginning of that response and @p msg_size is its length in octets.
    Upon success resolved address is placed in the corresponing structure pointed by
    @p resolved_addr

    @retval 0 success, -1 on error
 */
int pubnub_pick_resolved_address(uint8_t const* buf,
                                 size_t msg_size,
                                 struct pubnub_ipv4_address* resolved_addr);

#endif /* defined INC_PUBNUB_DNS_HANDLER */
