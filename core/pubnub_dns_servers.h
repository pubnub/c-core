/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_DNS_SERVERS
#define INC_PUBNUB_DNS_SERVERS

#include <stdint.h>

/** IPv4 Address, in binary format.
 */
struct pubnub_ipv4_address {
    /** The four octets of the IPv4 address */
    uint8_t ipv4[4];
};

#if PUBNUB_USE_IPV6
/** IPv6 Address, in binary format.
 */
struct pubnub_ipv6_address {
    /** The 8 double octets(big endian) of the IPv6 address */
    uint8_t ipv6[16];
};

/* primary, secondary(ipv4, ipv6) and default dns server */
#define PUBNUB_MAX_DNS_SERVERS_MASK 0x10 
#else
/* primary, secondary(ipv4) and default dns server */
#define PUBNUB_MAX_DNS_SERVERS_MASK 0x04
#endif /* PUBNUB_USE_IPV6 */

#if PUBNUB_SET_DNS_SERVERS
#include <stdlib.h>
#include "lib/pb_extern.h"

/** Sets the primary DNS server IPv4 address to use when
    resolving the Pubnub origin, in binary form(network order).
    Applies to all subsequent DNS queries, if successful.
    (Note: All DNS servers are initially set to zeros)

    @param ipv4 The IPv4 address of the server to use. Set all
                0 to not use this DNS server.
    @retval 0 OK
    @retval -1 error: Pubnub DNS module not used and can't set
               the primary Ipv4 DNS server
  */
PUBNUB_EXTERN int pubnub_dns_set_primary_server_ipv4(struct pubnub_ipv4_address ipv4);

/** Sets the secondary DNS server IPv4 address to use
    when resolving the Pubnub origin, in binary form(network order).

    Applies to all subsequent DNS queries, if successful and if
    using secondary server is supported. Secondary server, if
    used at all, is used if a query to the primary server fails.
    (Note: All DNS servers are initially set to zeros)

    @param ipv4 The IPv4 address of the server to use. Set all
           0 to not use this DNS server.
    @retval 0 OK
    @retval -1 error: Pubnub DNS module not used and can't set
               the secondary Ipv4 DNS server
  */
PUBNUB_EXTERN int pubnub_dns_set_secondary_server_ipv4(struct pubnub_ipv4_address ipv4);

/** Sets the primary DNS server IPv4 address from the corresponding
    'numbers-and-dots' notation string to use when resolving the Pubnub origin.
    Applies to all subsequent DNS queries, if successful.
    (Note: All DNS servers are initially set to zeros)

    @param ipv4_str The IPv4 address string of the server to use. Set all
                    zeros("0.0.0.0") to not use this DNS server.
    @retval 0 OK
    @retval -1 error: Pubnub DNS module not used, or can't set
               the primary Ipv4 DNS server
  */
PUBNUB_EXTERN int pubnub_dns_set_primary_server_ipv4_str(char const* ipv4_str);

/** Sets the secondary DNS server IPv4 address from the corresponding
    'numbers-and-dots' notation string to use when resolving the Pubnub origin.

    Applies to all subsequent DNS queries, if successful and if
    using secondary server is supported. Secondary server, if
    used at all, is used if a query to the primary server fails.
    (Note: All DNS servers are initially set to zeros)

    @param ipv4_str The IPv4 address of the server to use. Set all
           0 to not use this DNS server.
    @retval 0 OK
    @retval -1 error: Pubnub DNS module not used, or can't set the
               secondary Ipv4 DNS server
  */
PUBNUB_EXTERN int pubnub_dns_set_secondary_server_ipv4_str(char const* ipv4_str);

/** Reads the currently set primary DNS server's IPv4 address,
    in binary form(network order).

    @param[out] o_ipv4 The IPv4 address of the server used.
    @retval 0 OK
    @retval -1 error: Pubnub DNS module not used and can't read
            the primary Ipv4 DNS server
  */
PUBNUB_EXTERN int pubnub_get_dns_primary_server_ipv4(struct pubnub_ipv4_address* o_ipv4);

/** Reads the currently set secondary DNS server's IPv4 address,
    in binary form(network order).

    @param[out] o_ipv4 The IPv4 address of the server used.
    @retval 0 OK
    @retval -1 error: Pubnub DNS module not used and can't read
            the secondary Ipv4 DNS server
  */
PUBNUB_EXTERN int pubnub_get_dns_secondary_server_ipv4(struct pubnub_ipv4_address* o_ipv4);

/** Reads the DNS servers in the system configuration. Will read
    at most @p n servers, even if more are configured. Keep in
    mind that modern systems have complex configurations and
    often this will yield just one DNS server which listens on the
    loopback IP address, while the "real" DNS configuration is
    not available through standard means.

    On POSIX systems, this will read from `/etc/resolv.conf`,
    looking for `nameserver` lines. On Windows, this will use OS
    functions to get the info.

    @param[out] o_ipv4 The array where to put the system DNS servers.
                       allocated by the caller for @p n elements.

    @param[in] n The number of elements allocated for the @p o_ipv4
    @retval -1: error, can't read DNS server configuration
    @retval otherwise: number of DNS servers read
  */
PUBNUB_EXTERN int pubnub_dns_read_system_servers_ipv4(struct pubnub_ipv4_address* o_ipv4,
                                        size_t                      n);
#if PUBNUB_USE_IPV6
/** Sets the primary DNS server IPv6 address to use when
    resolving the Pubnub origin, in binary form(network order).
    Applies to all subsequent DNS queries, if successful.
    (Note: All DNS servers are initially set to zeros)

    @param ipv6 The IPv6 address of the server to use. Set all
                0 to not use this DNS server.
    @retval 0 OK
    @retval -1 error: Pubnub DNS module not used and can't set
               the primary Ipv6 DNS server
  */
PUBNUB_EXTERN int pubnub_dns_set_primary_server_ipv6(struct pubnub_ipv6_address ipv6);

/** Sets the secondary DNS server IPv6 address to use
    when resolving the Pubnub origin, in binary form(network order).

    Applies to all subsequent DNS queries, if successful and if
    using secondary server is supported. Secondary server, if
    used at all, is used if a query to the primary server fails.
    (Note: All DNS servers are initially set to zeros)

    @param ipv6 The IPv6 address of the server to use. Set all
           0 to not use this DNS server.
    @retval 0 OK
    @retval -1 error: Pubnub DNS module not used and can't set
            the secondary Ipv6 DNS server
  */
PUBNUB_EXTERN int pubnub_dns_set_secondary_server_ipv6(struct pubnub_ipv6_address ipv6);

/** Sets the primary DNS server IPv6 address from the corresponding
    'numbers-and-colons' notation string to use when resolving the Pubnub origin.
    Applies to all subsequent DNS queries, if successful.
    (Note: All DNS servers are initially set to zeros) 

    @param ipv6_str The IPv6 address string of the server to use. Set all
                    zeros("::0", or "0::") to not use this DNS server.
    @retval 0 OK
    @retval -1 error: Pubnub DNS module not used, or can't set
               the primary Ipv6 DNS server
  */
PUBNUB_EXTERN int pubnub_dns_set_primary_server_ipv6_str(char const* ipv6_str);

/** Sets the secondary DNS server IPv6 address from the corresponding
    'numbers-and-colons' notation string to use when resolving the Pubnub origin.

    Applies to all subsequent DNS queries, if successful and if
    using secondary server is supported. Secondary server, if
    used at all, is used if a query to the primary server fails.
    (Note: All DNS servers are initially set to zeros) 

    @param ipv6_str The IPv6 address string of the server to use. Set all
                    zeros("::0", or "0::") to not use this DNS server.
    @retval 0 OK
    @retval -1 error: Pubnub DNS module not used and can't set the
            secondary Ipv6 DNS server
  */
PUBNUB_EXTERN int pubnub_dns_set_secondary_server_ipv6_str(char const* ipv6_str);

/** Reads the currently set primary DNS server's IPv6 address,
    in binary form(network order).

    @param[out] o_ipv6 The IPv6 address of the server used.
    @retval 0 OK
    @retval -1 error: Pubnub DNS module not used and can't read
            the primary Ipv6 DNS server
  */
PUBNUB_EXTERN int pubnub_get_dns_primary_server_ipv6(struct pubnub_ipv6_address* o_ipv6);

/** Reads the currently set secondary DNS server's IPv6 address,
    in binary form(network order).

    @param[out] o_ipv6 The IPv6 address of the server used.
    @retval 0 OK
    @retval -1 error: Pubnub DNS module not used and can't read
            the secondary Ipv6 DNS server
  */
PUBNUB_EXTERN int pubnub_get_dns_secondary_server_ipv6(struct pubnub_ipv6_address* o_ipv6);
#endif /* PUBNUB_USE_IPV6 */

#else

#define pubnub_dns_set_primary_server_ipv4(ipv4) -1
#define pubnub_dns_set_secondary_server_ipv4(ipv4) -1
#define pubnub_dns_set_primary_server_ipv4_str(ipv4_str) -1
#define pubnub_dns_set_secondary_server_ipv4_str(ipv4_str) -1
#define pubnub_get_dns_primary_server_ipv4(o_ipv4) -1
#define pubnub_get_dns_secondary_server_ipv4(o_ipv4) -1
#define pubnub_dns_read_system_servers_ipv4(o_ipv4, n) -1
#if PUBNUB_USE_IPV6
#define pubnub_dns_set_primary_server_ipv6(ipv6) -1
#define pubnub_dns_set_secondary_server_ipv6(ipv6) -1
#define pubnub_dns_set_primary_server_ipv6_str(ipv6_str) -1
#define pubnub_dns_set_secondary_server_ipv6_str(ipv6_str) -1
#define pubnub_get_dns_primary_server_ipv6(o_ipv6) -1
#define pubnub_get_dns_secondary_server_ipv6(o_ipv6) -1
#endif /* PUBNUB_USE_IPV6 */
#endif /* PUBNUB_SET_DNS_SERVERS */
#endif /* !defined INC_PUBNUB_DNS_SERVERS */
