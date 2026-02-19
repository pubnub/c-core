/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PBPAL_ANDS_SOCKETS
#define INC_PBPAL_ANDS_SOCKETS

#include "lib/pubnub_dns_codec.h"

struct sockaddr;
struct dns_queries_tracking;

/**
 * Perform a DNS query by sending a packet to the DNS server @p dest.
 */
int send_dns_query(pubnub_t*                    pb,
                   pb_socket_t                  skt,
                   struct sockaddr const*       dest,
                   char const*                  host,
                   struct dns_queries_tracking* tracking);

/** Reads response from DNS server @p dest, putting it into @p resolved addr. */
int read_dns_response(pubnub_t*        pb,
                      pb_socket_t      skt,
                      struct sockaddr* dest,
                      struct dns_queries_tracking* tracking
                          PBDNS_OPTIONAL_PARAMS_DECLARATIONS);


#endif /* defined INC_PBPAL_ANDS_SOCKETS */
