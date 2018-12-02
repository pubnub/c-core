/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "lib/sockets/pbpal_adns_sockets.h"

#include "pubnub_internal.h"

#include "core/pubnub_log.h"
#include "lib/pubnub_dns_codec.h"

#if !defined(_WIN32)
#include <arpa/inet.h>
#define CAST
#else
#include <ws2tcpip.h>
#define CAST (int*)
#endif

#include <stdint.h>


#if PUBNUB_LOG_LEVEL >= PUBNUB_LOG_LEVEL_TRACE
#define TRACE_SOCKADDR(str, addr)                                              \
    do {                                                                       \
        char M_h_[50];                                                         \
        char M_s_[20];                                                         \
        getnameinfo(addr,                                                      \
                    sizeof *addr,                                              \
                    M_h_,                                                      \
                    sizeof M_h_,                                               \
                    M_s_,                                                      \
                    sizeof M_s_,                                               \
                    NI_NUMERICHOST | NI_NUMERICSERV);                          \
        PUBNUB_LOG_TRACE(str "%s:%s\n", M_h_, M_s_);                           \
    } while (0)
#else
#define TRACE_SOCKADDR(str, addr)
#endif


int send_dns_query(int skt, struct sockaddr const* dest, char const* host)
{
    uint8_t buf[4096];
    int     to_send;
    int                sent_to;

    if (-1 == pubnub_prepare_dns_request(buf, sizeof buf, host, &to_send)) {
        PUBNUB_LOG_ERROR("Couldn't prepare dns request! : #prepared bytes=%d\n", to_send);
        return -1;
    }
    TRACE_SOCKADDR("Sending DNS query to: ", dest);
    sent_to = sendto(skt, (char*)buf, to_send, 0, dest, sizeof *dest);
    if (sent_to <= 0) {
        return socket_would_block() ? +1 : -1;
    }
    else if (to_send != sent_to) {
        PUBNUB_LOG_ERROR("sendto() sent %d out of %d bytes!\n", sent_to, to_send);
        return -1;
    }
    return 0;
}


int read_dns_response(int skt, struct sockaddr* dest, struct sockaddr_in* resolved_addr)
{
    uint8_t  buf[8192];
    int      msg_size;
    unsigned addr_size = sizeof *dest;

    msg_size = recvfrom(skt, (char*)buf, sizeof buf, 0, dest, CAST & addr_size);
    if (msg_size <= 0) {
        return socket_would_block() ? +1 : -1;
    }
            resolved_addr->sin_family = AF_INET;

    return pubnub_pick_resolved_address(buf,
                                        (size_t)msg_size,
                                        (struct pubnub_ipv4_address*)&(resolved_addr->sin_addr.s_addr));
}


#if 0
#include <stdio.h>
#include <fcntl.h>
int main()
{
    struct sockaddr_in dest;

    int skt = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    int flags = fcntl(skt, F_GETFL, 0);
    fcntl(skt, F_SETFL, flags | O_NONBLOCK);

    dest.sin_family = AF_INET;
    dest.sin_port = htons(53);
    dest.sin_addr.s_addr = inet_addr("8.8.8.8");

    send_dns_query(skt, (struct sockaddr*)&dest, "pubsub.pubnub.com");

    fd_set read_set, write_set;
    int rslt;
    struct timeval timev = { 0, 300000 };

    FD_ZERO(&read_set);
    FD_SET(skt, &read_set);
    rslt = select(skt + 1, &read_set, NULL, NULL, &timev);
    if (-1 == rslt) {
        puts("select() Error!\n");
        return -1;
    }
    else if (rslt > 0) {
        struct sockaddr_in resolved_addr;
        printf("skt=%d, rslt=%d, timev.tv_sec=%ld, timev.tv_usec=%ld\n", skt, rslt, timev.tv_sec, timev.tv_usec);
        read_dns_response(skt, (struct sockaddr*)&dest, &resolved_addr);
    }
    else {
        puts("no select() event");
    }



    return 0;
}
#endif
