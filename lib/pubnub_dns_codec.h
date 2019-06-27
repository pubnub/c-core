/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_DNS_HANDLER
#define      INC_PUBNUB_DNS_HANDLER

#include "pubnub_internal.h"
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

#if PUBNUB_USE_MULTIPLE_ADDRESSES
#define PBDNS_OPTIONAL_PARAMS_DECLARATIONS , struct pubnub_multi_addresses* spare_addresses \
                                           , struct pubnub_options const* options
#define PBDNS_OPTIONAL_PARAMS , spare_addresses, options
#else
#define PBDNS_OPTIONAL_PARAMS_DECLARATIONS
#define PBDNS_OPTIONAL_PARAMS
#endif


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

/** Picks valid resolved(Ipv4, or Ipv6) domain name addresses from the response from DNS server.
    @p buf points to the beginning of that response and @p msg_size is its length in octets.
    Upon success one resolved address is placed in the corresponing structure pointed by
    @p resolved_addr_ipv4 and/or @p resolved_addr_ipv6(if structure pointer is not NULL and
    the address of a given kind is found). Optionaly(but by default), rest of them are saved
    in ip_address arrays within @p pb context as auxiliary ones.

    @retval 0 success, -1 on error
 */
int pbdns_pick_resolved_addresses(uint8_t const* buf,
                                  size_t msg_size,
                                  struct pubnub_ipv4_address* resolved_addr_ipv4
                                  IPV6_ADDR_ARGUMENT_DECLARATION
                                  PBDNS_OPTIONAL_PARAMS_DECLARATIONS);


#endif /* defined INC_PUBNUB_DNS_HANDLER */
