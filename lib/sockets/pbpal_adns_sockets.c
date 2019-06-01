/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"

#include "lib/sockets/pbpal_adns_sockets.h"
#include "core/pubnub_assert.h"
#include "core/pubnub_log.h"

#if !defined(_WIN32)
#include <arpa/inet.h>
#define CAST
#else
#include <ws2tcpip.h>
#define CAST (int*)
#endif

#include <stdint.h>
#if PUBNUB_USE_MULTIPLE_ADDRESSES
#include <time.h>
#endif

#define DNS_PORT 53

#if PUBNUB_LOG_LEVEL >= PUBNUB_LOG_LEVEL_TRACE
#define TRACE_SOCKADDR(str, addr, sockaddr_size)                               \
    do {                                                                       \
        char M_h_[50];                                                         \
        char M_s_[20];                                                         \
        getnameinfo(addr,                                                      \
                    sockaddr_size,                                             \
                    M_h_,                                                      \
                    sizeof M_h_,                                               \
                    M_s_,                                                      \
                    sizeof M_s_,                                               \
                    NI_NUMERICHOST | NI_NUMERICSERV);                          \
        PUBNUB_LOG_TRACE(str "%s-port:%s\n", M_h_, M_s_);                      \
    } while (0)
#else
#define TRACE_SOCKADDR(str, addr)
#endif


int send_dns_query(pb_socket_t            skt,
                   struct sockaddr const* dest,
                   char const*            host,
                   enum DNSqueryType      query_type)
{
    uint8_t buf[4096];
    int     to_send;
    int     sent_to;
    size_t  sockaddr_size;

    switch (dest->sa_family) {
    case AF_INET:
        sockaddr_size = sizeof(struct sockaddr_in);
        ((struct sockaddr_in*)dest)->sin_port = htons(DNS_PORT);
        break;
#if PUBNUB_USE_IPV6
    case AF_INET6:
        sockaddr_size = sizeof(struct sockaddr_in6);
        ((struct sockaddr_in6*)dest)->sin6_port = htons(DNS_PORT);
        break;
#endif /* PUBNUB_USE_IPV6 */
    default:
        PUBNUB_LOG_ERROR("send_dns_query(socket=%d): invalid address family "
                         "dest->sa_family =%uh\n",
                         skt,
                         dest->sa_family);
        return -1;
    }
    if (-1 == pbdns_prepare_dns_request(buf, sizeof buf, host, &to_send, query_type)) {
        PUBNUB_LOG_ERROR("Couldn't prepare dns request! : #prepared bytes=%d\n",
                         to_send);
        return -1;
    }
    TRACE_SOCKADDR("Sending DNS query to: ", dest, sockaddr_size);
    sent_to = sendto(skt, (char*)buf, to_send, 0, dest, sockaddr_size);
    if (sent_to <= 0) {
        return socket_would_block() ? +1 : -1;
    }
    else if (to_send != sent_to) {
        PUBNUB_LOG_ERROR("sendto() sent %d out of %d bytes!\n", sent_to, to_send);
        return -1;
    }
    return 0;
}

#if PUBNUB_USE_IPV6
#define P_ADDR_IPV6_ARGUMENT , &addr_ipv6
#else
#define P_ADDR_IPV6_ARGUMENT
#endif


int read_dns_response(pb_socket_t skt,
                      struct sockaddr* dest,
                      struct sockaddr* resolved_addr
                      PBDNS_OPTIONAL_PARAMS_DECLARATIONS)
{
    uint8_t                    buf[8192];
    int                        msg_size;
    unsigned                   sockaddr_size;
    struct pubnub_ipv4_address addr_ipv4 = {{0}};
#if PUBNUB_USE_IPV6
    struct pubnub_ipv6_address addr_ipv6 = {{0}};
#endif
    PUBNUB_ASSERT(SOCKET_INVALID != skt);

    switch (dest->sa_family) {
    case AF_INET:
        sockaddr_size = sizeof(struct sockaddr_in);
        ((struct sockaddr_in*)dest)->sin_port = htons(DNS_PORT);
        break;
#if PUBNUB_USE_IPV6
    case AF_INET6:
        sockaddr_size = sizeof(struct sockaddr_in6);
        ((struct sockaddr_in6*)dest)->sin6_port = htons(DNS_PORT);
        break;
#endif /* PUBNUB_USE_IPV6 */
    default:
        PUBNUB_LOG_ERROR("read_dns_response(socket=%d): invalid address family "
                         "dest->sa_family =%uh\n",
                         skt,
                         dest->sa_family);
        return -1;
    }
    msg_size = recvfrom(skt, (char*)buf, sizeof buf, 0, dest, CAST & sockaddr_size);
    if (msg_size <= 0) {
        return socket_would_block() ? +1 : -1;
    }
#if PUBNUB_USE_MULTIPLE_ADDRESSES
    time(&spare_addresses->time_of_the_last_dns_query);
#endif
    if (pbdns_pick_resolved_addresses(buf,
                                      (size_t)msg_size,
                                      &addr_ipv4
                                      P_ADDR_IPV6_ARGUMENT
                                      PBDNS_OPTIONAL_PARAMS) != 0) {
        return -1;
    }
    if (addr_ipv4.ipv4[0] != 0) {
        memcpy(&((struct sockaddr_in*)resolved_addr)->sin_addr.s_addr,
               addr_ipv4.ipv4,
               sizeof addr_ipv4.ipv4);
        resolved_addr->sa_family = AF_INET;
    }
#if PUBNUB_USE_IPV6
    else {
        memcpy(((struct sockaddr_in6*)resolved_addr)->sin6_addr.s6_addr,
               addr_ipv6.ipv6,
               sizeof addr_ipv6.ipv6);
        resolved_addr->sa_family = AF_INET6;
    }
#endif /* PUBNUB_USE_IPV6 */
    return 0;
}


#if 0
#include <stdio.h>
#if !defined(_WIN32)
#include <fcntl.h>
#endif

/* When running this test example PUBNUB_USE_MULTIPLE_ADDRESSES should be defined and
   set to zero in the corresponding make file
*/
int main()
{
    struct sockaddr_in  dest;
    struct sockaddr_in6 dest6;
    struct sockaddr_storage resolved_addr;
    
#if defined(_WIN32)
    WSADATA wsaData;
    int iResult;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }
#endif
    puts("===========================ADNS-AF_INET========================");

    pb_socket_t skt = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (SOCKET_INVALID == skt) {
        PUBNUB_LOG_ERROR("Error: Couldnt't get Ipv4 socket.\n");
        return -1;
    }
    
#if !defined(_WIN32)
    int flags = fcntl(skt, F_GETFL, 0);
    fcntl(skt, F_SETFL, flags | O_NONBLOCK);
#endif
    dest.sin_family      = AF_INET;
    dest.sin_port        = htons(53);
    inet_pton(AF_INET, "208.67.222.222", &(dest.sin_addr.s_addr));

    if (-1 == send_dns_query(skt, (struct sockaddr*)&dest, "facebook.com", dnsANY)) {
        PUBNUB_LOG_ERROR("Error: Couldn't send datagram(Ipv4).\n");
        return -1;
    }

#if !defined(_WIN32)
    fd_set         read_set;
    int            rslt;
    struct timeval timev = { 0, 300000 };

    FD_ZERO(&read_set);
    FD_SET(skt, &read_set);
    rslt = select(skt + 1, &read_set, NULL, NULL, &timev);
    if (-1 == rslt) {
        puts("select() Error!\n");
        return -1;
    }
    else if (rslt > 0) {
        printf("skt=%d, rslt=%d, timev.tv_sec=%ld, timev.tv_usec=%ld\n",
               skt,
               rslt,
               timev.tv_sec,
               timev.tv_usec);
#endif
        read_dns_response(skt, (struct sockaddr*)&dest, (struct sockaddr*)&resolved_addr);
#if !defined(_WIN32)
    }
    else {
        puts("no select() event(Ipv4).");
    }
#endif
    socket_close(skt);

    puts("=========================ADNS-AF_INET6========================");

    skt   = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (SOCKET_INVALID == skt) {
        PUBNUB_LOG_ERROR("Error: Didn't get Ipv6 socket.\n");
        return -1;
    }
#if !defined(_WIN32)
    flags = fcntl(skt, F_GETFL, 0);
    fcntl(skt, F_SETFL, flags | O_NONBLOCK);
    timev.tv_sec  = 0;
    timev.tv_usec = 300000;
#endif
    dest6.sin6_family = AF_INET6;
    dest6.sin6_port   = htons(53);
    inet_pton(AF_INET6, "2001:470:20::2", dest6.sin6_addr.s6_addr);

    if (-1 == send_dns_query(skt, (struct sockaddr*)&dest6, "facebook.com", dnsANY)) {
        PUBNUB_LOG_ERROR("Error: Couldn't send datagram(Ipv6).\n");
        
        return -1;
    }

#if !defined(_WIN32)
    FD_ZERO(&read_set);
    FD_SET(skt, &read_set);
    rslt = select(skt + 1, &read_set, NULL, NULL, &timev);
    if (-1 == rslt) {
        puts("select() Error!\n");
        return -1;
    }
    else if (rslt > 0) {
        printf("skt=%d, rslt=%d, timev.tv_sec=%ld, timev.tv_usec=%ld\n",
               skt,
               rslt,
               timev.tv_sec,
               timev.tv_usec);
#endif
        read_dns_response(skt, (struct sockaddr*)&dest6, (struct sockaddr*)&resolved_addr);
#if !defined(_WIN32)
    }
    else {
        puts("no select() event(Ipv6).");
    }
#endif
    socket_close(skt);

    return 0;
}
#endif
