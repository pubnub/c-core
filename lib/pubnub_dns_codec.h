/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_DNS_HANDLER
#define      INC_PUBNUB_DNS_HANDLER

#include "core/pubnub_dns_servers.h"

#include <stdlib.h>

/** Question/query types */
enum DNSqueryType {
    /** Address - IPv4 */
    dnsA = 1,
    /** Name server */
    dnsNS = 2,
    /** Canonical name */
    dnsCNAME = 5,
    /** Start of authority */
    dnsSOA = 6,
    /** Pointer (to another location in the name space ) */
    dnsPTR = 12,
    /** Mail exchange (responsible for handling e-mail sent to the
     * domain */
    dnsMX = 15,
    /** IPv6 address - 128 bit */
    dnsAAAA = 28,
    /** Service locator */
    dnsSRV = 33,
    /** All cached records */
    dnsANY = 255
};

#if PUBNUB_USE_IPV6
#define IPV6_ADDR_ARGUMENT_DECLARATION , struct pubnub_ipv6_address* resolved_addr_ipv6
#define IPV6_ADDR_ARGUMENT , resolved_addr_ipv6
#else
#define IPV6_ADDR_ARGUMENT_DECLARATION
#define IPV6_ADDR_ARGUMENT
#endif /* PUBNUB_USE_IPV6 */

/** Prepares DNS @p query_type query request for @p host(name) in @p buf whose maximum available
    length is @p buf_size in octets.
    If function succeedes, @p to_send 'carries' the length of prepared message.
    If function reports en error, @p to_send 'keeps' the length of successfully prepared segment
    before error occurred.
    
    @retval 0 success, -1 on error
 */
int pbdns_prepare_dns_request(uint8_t* buf,
                              size_t buf_size,
                              char const* host,
                              int *to_send,
                              enum DNSqueryType query_type);

/** Picks valid resolved(Ipv4, or Ipv6) domain name address from the response from DNS server.
    @p buf points to the beginning of that response and @p msg_size is its length in octets.
    Upon success resolved address is placed in the corresponing structure pointed by
    @p resolved_addr_ipv4 or @p resolved_addr_ipv6(whichever was found first and its structure
    pointer was not NULL).

    @retval 0 success, -1 on error
 */
int pbdns_pick_resolved_address(uint8_t const* buf,
                                size_t msg_size,
                                struct pubnub_ipv4_address* resolved_addr_ipv4
                                IPV6_ADDR_ARGUMENT_DECLARATION);


#endif /* defined INC_PUBNUB_DNS_HANDLER */
