/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "lib/sockets/pbpal_adns_sockets.h"

#include "pubnub_internal.h"
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


int send_dns_query(pb_socket_t                  skt,
                   struct sockaddr const*       dest,
                   char const*                  host,
                   struct dns_queries_tracking* tracking)
{
    uint8_t buf[4096];
    int     to_send;
    int     sent_to;
    size_t  sockaddr_size;
    bool    any_blocked = false;

    if (!tracking) {
        PUBNUB_LOG_ERROR("send_dns_query: tracking required\n");
        return -1;
    }

    switch (dest->sa_family) {
    case AF_INET:
        sockaddr_size                         = sizeof(struct sockaddr_in);
        ((struct sockaddr_in*)dest)->sin_port = htons(DNS_PORT);
        break;
#if PUBNUB_USE_IPV6
    case AF_INET6:
        sockaddr_size                           = sizeof(struct sockaddr_in6);
        ((struct sockaddr_in6*)dest)->sin6_port = htons(DNS_PORT);
        break;
#endif /* PUBNUB_USE_IPV6 */
    default:
        PUBNUB_LOG_ERROR("send_dns_query(socket=%d): invalid address family "
                         "dest->sa_family=%u\n",
                         skt,
                         dest->sa_family);
        return -1;
    }

    if (!tracking->sent_a) {
        if (pbdns_prepare_dns_request(buf, sizeof buf, host, &to_send, dnsA) == 0) {
            TRACE_SOCKADDR("Sending DNS A query to: ", dest, sockaddr_size);
            sent_to = sendto(skt, (char*)buf, to_send, 0, dest, sockaddr_size);
            if (sent_to > 0 && to_send == sent_to)
                tracking->sent_a = true;
            else if (sent_to <= 0 && socket_would_block())
                any_blocked = true;
            else
                PUBNUB_LOG_WARNING("Failed to send A query: %d\n", sent_to);
        }
    }

#if PUBNUB_USE_IPV6
    if (!tracking->sent_aaaa) {
        if (pbdns_prepare_dns_request(buf, sizeof buf, host, &to_send, dnsAAAA)
            == 0) {
            TRACE_SOCKADDR("Sending DNS AAAA query to: ", dest, sockaddr_size);
            sent_to = sendto(skt, (char*)buf, to_send, 0, dest, sockaddr_size);
            if (sent_to > 0 && to_send == sent_to)
                tracking->sent_aaaa = true;
            else if (sent_to <= 0 && socket_would_block())
                any_blocked = true;
            else
                PUBNUB_LOG_WARNING("Failed to send AAAA query: %d\n", sent_to);
        }
    }
#endif /* PUBNUB_USE_IPV6 */
    tracking->need_retry = !tracking->sent_a
#if PUBNUB_USE_IPV6
                           || !tracking->sent_aaaa
#endif
        ;

    if (!tracking->sent_a
#if PUBNUB_USE_IPV6
        && !tracking->sent_aaaa
#endif
    )
        return any_blocked ? +1 : -1;
    if (tracking->need_retry && any_blocked)
        return +1;

    return 0;
}

#if PUBNUB_USE_IPV6
#define P_ADDR_IPV6_ARGUMENT , &addr_ipv6
#else
#define P_ADDR_IPV6_ARGUMENT
#endif


int read_dns_response(pb_socket_t      skt,
                      struct sockaddr* dest,
                      struct dns_queries_tracking* tracking PBDNS_OPTIONAL_PARAMS_DECLARATIONS)
{
    uint8_t  buf[8192];
    int      msg_size;
    unsigned sockaddr_size;
    int      responses_received = 0;
    int      expected_responses = 0;

    PUBNUB_ASSERT(SOCKET_INVALID != skt);

    if (tracking->sent_a)
        expected_responses++;
    if (tracking->received_a)
        responses_received++;
#if PUBNUB_USE_IPV6
    if (tracking->sent_aaaa)
        expected_responses++;
    if (tracking->received_aaaa)
        responses_received++;
#endif /* PUBNUB_USE_IPV6 */

    if (expected_responses == 0) {
        PUBNUB_LOG_ERROR("read_dns_response: No queries were sent!\n");
        return -1;
    }

    if (responses_received >= expected_responses) {
        PUBNUB_LOG_WARNING(
            "read_dns_response: Already received all responses\n");
        return 0;
    }

    PUBNUB_LOG_TRACE("Expecting %d response(s), already received %d\n",
                     expected_responses,
                     responses_received);

    switch (dest->sa_family) {
    case AF_INET:
        sockaddr_size                         = sizeof(struct sockaddr_in);
        ((struct sockaddr_in*)dest)->sin_port = htons(DNS_PORT);
        break;
#if PUBNUB_USE_IPV6
    case AF_INET6:
        sockaddr_size                           = sizeof(struct sockaddr_in6);
        ((struct sockaddr_in6*)dest)->sin6_port = htons(DNS_PORT);
        break;
#endif /* PUBNUB_USE_IPV6 */
    default:
        PUBNUB_LOG_ERROR("read_dns_response(socket=%d): invalid address family "
                         "dest->sa_family =%u\n",
                         skt,
                         dest->sa_family);
        return -1;
    }
    while (responses_received < expected_responses) {
        msg_size =
            recvfrom(skt, (char*)buf, sizeof buf, 0, dest, CAST & sockaddr_size);
        if (msg_size <= 0)
            return socket_would_block() ? +1 : -1;

        PUBNUB_LOG_TRACE("Received DNS response packet (%d bytes)\n", msg_size);
#if PUBNUB_USE_MULTIPLE_ADDRESSES
        if (responses_received == 0)
            time(&spare_addresses->time_of_the_last_dns_query);
#endif /* PUBNUB_USE_MULTIPLE_ADDRESSES */
        enum DNSqueryType          question_type;
        struct pubnub_ipv4_address this_ipv4 = { { 0 } };
#if PUBNUB_USE_IPV6
        struct pubnub_ipv6_address this_ipv6 = { { 0 } };
#endif

        if (pbdns_pick_resolved_addresses(buf,
                                          (size_t)msg_size,
                                          &question_type,
                                          &this_ipv4
#if PUBNUB_USE_IPV6
                                          ,
                                          &this_ipv6
#endif
                                              PBDNS_OPTIONAL_PARAMS)
            == 0) {
            if (dnsA == question_type && !tracking->received_a) {
                if (this_ipv4.ipv4[0] != 0) {
                    memcpy(&tracking->dns_a_addr.sin_addr.s_addr,
                           this_ipv4.ipv4,
                           sizeof this_ipv4.ipv4);
                    tracking->dns_a_addr.sin_family = AF_INET;
                }
                tracking->received_a = true;
                responses_received++;
            }
#if PUBNUB_USE_IPV6
            else if (dnsAAAA == question_type && !tracking->received_aaaa) {
                if (this_ipv6.ipv6[0] != 0 || this_ipv6.ipv6[1] != 0) {
                    struct sockaddr_in6* sin6 =
                        (struct sockaddr_in6*)&tracking->dns_aaaa_addr;
                    memcpy(sin6->sin6_addr.s6_addr,
                           this_ipv6.ipv6,
                           sizeof this_ipv6.ipv6);
                    sin6->sin6_family = AF_INET6;
                }
                tracking->received_aaaa = true;
                responses_received++;
            }
#endif
            else {
                PUBNUB_LOG_WARNING(
                    "Received duplicate or unexpected DNS response\n");
            }
        }
        else {
            responses_received++;
            if (dnsA == question_type) {
                tracking->received_a = true;
                PUBNUB_LOG_WARNING(
                    "There are no 'A' records for requested domain.\n");
            }
#if PUBNUB_USE_IPV6
            else if (dnsAAAA == question_type) {
                tracking->received_aaaa = true;
                PUBNUB_LOG_WARNING(
                    "There are no 'AAAA' records for requested domain.\n");
            }
#endif /* PUBNUB_USE_IPV6 */
            else
                PUBNUB_LOG_WARNING("Failed to parse DNS response.\n");
        }
    }

    PUBNUB_LOG_TRACE("All %d DNS responses received\n", expected_responses);

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
    struct dns_queries_tracking tracking;
    memset(&tracking, 0, sizeof(tracking));

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

    if (-1 == send_dns_query(skt, (struct sockaddr*)&dest, "facebook.com", &tracking)) {
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
        read_dns_response(skt, (struct sockaddr*)&dest, &tracking);
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

    if (-1 == send_dns_query(skt, (struct sockaddr*)&dest6, "facebook.com", &tracking)) {
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
        read_dns_response(skt, (struct sockaddr*)&dest6, &tracking);
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
