/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PBPAL_ANDS_SOCKETS
#define      INC_PBPAL_ANDS_SOCKETS


#include <arpa/inet.h> //inet_addr , inet_ntoa , ntohs etc


/**
 * Perform a DNS query by sending a packet to the DNS server @p dest.
 */
int send_dns_query(int skt, struct sockaddr const *dest, unsigned char *host);

/** Read a DNS response from @p dns_server. */
int read_response(int skt, struct sockaddr *dns_server, unsigned char const *host, struct sockaddr_in *dest);



#endif /* defined INC_PBPAL_ANDS_SOCKETS */
