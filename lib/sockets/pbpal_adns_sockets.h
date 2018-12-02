/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PBPAL_ANDS_SOCKETS
#define      INC_PBPAL_ANDS_SOCKETS


struct sockaddr;
struct sockaddr_in;

/**
 * Perform a DNS query by sending a packet to the DNS server @p dest.
 */
int send_dns_query(int skt, struct sockaddr const *dest, char const*host);

/** Read a DNS response from DNS server @p dest, putting it into @p resolved addr. */
int read_dns_response(int skt, struct sockaddr *dest, struct sockaddr_in *resolved_addr);



#endif /* defined INC_PBPAL_ANDS_SOCKETS */
