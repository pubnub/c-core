/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pbpal.h"

#include "pubnub_internal.h"
#include "pubnub_assert.h"
#include "pubnub_log.h"
#include "pbpal_adns_sockets.h"

#include <sys/types.h>


#define HTTP_PORT 80

#define DNS_PORT 53


#if !PUBNUB_USE_ADNS
#define send_dns_query(x,y,z) -1
#define read_response(x,y,z,v) -1
#endif


enum pbpal_resolv_n_connect_result pbpal_resolv_and_connect(pubnub_t *pb)
{
    int error;
    uint16_t port = HTTP_PORT;
    char const* origin = PUBNUB_ORIGIN_SETTABLE ? pb->origin : PUBNUB_ORIGIN;

#if PUBNUB_PROXY_API
    switch (pb->proxy_type) {
    case pbproxyHTTP_CONNECT:
        if (!pb->proxy_tunnel_established) {
            origin = pb->proxy_hostname;
        }
        port = pb->proxy_port;
        break;
    case pbproxyHTTP_GET:
        origin = pb->proxy_hostname;
        port = pb->proxy_port;
        break;
    default:
        break;
    }
#endif

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_ASSERT_OPT((pb->state == PBS_READY) || (pb->state == PBS_WAIT_DNS_SEND)  || (pb->state == PBS_WAIT_DNS_RCV));

    if (PUBNUB_USE_ADNS) {
        struct sockaddr_in dest;
        int skt = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        if (SOCKET_INVALID == skt) {
            return pbpal_resolv_resource_failure;
        }
        pb->options.use_blocking_io = false;
        pbpal_set_blocking_io(pb);
        dest.sin_family = AF_INET;
        dest.sin_port = htons(DNS_PORT);
        inet_pton(AF_INET, "8.8.8.8", &dest.sin_addr.s_addr);
        error = send_dns_query(skt, (struct sockaddr*)&dest, (unsigned char*)origin);
        if (error < 0) {
            socket_close(skt);
            return pbpal_resolv_failed_send;
        }
        else if (error > 0) {
            return pbpal_resolv_send_wouldblock;
        }
        pb->pal.socket = skt;
        return pbpal_resolv_sent;
    }
    else {
        char port_string[20];
        struct addrinfo *result;
        struct addrinfo *it;
        struct addrinfo hint;

        hint.ai_socktype = SOCK_STREAM;
        hint.ai_family = AF_UNSPEC;
        hint.ai_protocol = hint.ai_flags = hint.ai_addrlen = 0;
        hint.ai_addr = NULL;
        hint.ai_canonname = NULL;
        hint.ai_next = NULL;

        sprintf(port_string, "%d", port);
        error = getaddrinfo(origin, port_string, &hint, &result);
        if (error != 0) {
            return pbpal_resolv_failed_processing;
        }

        for (it = result; it != NULL; it = it->ai_next) {
            pb->pal.socket = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
            if (pb->pal.socket == SOCKET_INVALID) {
                continue;
            }
            pbpal_set_blocking_io(pb);
            if (connect(pb->pal.socket, it->ai_addr, it->ai_addrlen) == SOCKET_ERROR) {
                if (socket_would_block()) {
                    error = 1;
                    break;
                }
                else {
                    PUBNUB_LOG_WARNING("socket connect() failed, will try another IP address, if available\n");
                    socket_close(pb->pal.socket);
                    pb->pal.socket = SOCKET_INVALID;
                    continue;
                }
            }
            break;
        }
        freeaddrinfo(result);

        if (NULL == it) {
            return pbpal_connect_failed;
        }

        socket_set_rcv_timeout(pb->pal.socket, pb->transaction_timeout_ms);

        return error ? pbpal_connect_wouldblock : pbpal_connect_success;
    }
}


enum pbpal_resolv_n_connect_result pbpal_check_resolv_and_connect(pubnub_t *pb)
{
#if PUBNUB_USE_ADNS

    uint8_t const* const origin = (uint8_t*)(PUBNUB_ORIGIN_SETTABLE ? pb->origin : PUBNUB_ORIGIN);
    struct sockaddr_in dns_server;
    struct sockaddr_in dest;
    uint16_t port = HTTP_PORT;
    int skt = pb->pal.socket;

#if PUBNUB_PROXY_API
    port = pb->proxy_port;
#endif

    dns_server.sin_family = AF_INET;
    dns_server.sin_port = htons(DNS_PORT);
    inet_pton(AF_INET, "8.8.8.8", &dns_server.sin_addr.s_addr);
    switch (read_response(skt, (struct sockaddr*)&dns_server, origin, &dest)) {
    case -1: return pbpal_resolv_failed_rcv;
    case +1: return pbpal_resolv_rcv_wouldblock;
    case 0: break;
    }
    socket_close(skt);
    skt = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (SOCKET_INVALID == skt) {
        return pbpal_connect_resource_failure;
    }
    pb->pal.socket = skt;
    dest.sin_port = htons(port);
    if (SOCKET_ERROR == connect(skt, (struct sockaddr*)&dest, sizeof dest)) {
        return socket_would_block() ? pbpal_connect_wouldblock : pbpal_connect_failed;
    }
    
    return pbpal_connect_success;

#else

    /* Under regular BSD-ish sockets, this function should not be
       called unless using async DNS, so this is an error */
    return pbpal_connect_failed;

#endif
}


enum pbpal_resolv_n_connect_result pbpal_check_connect(pubnub_t *pb)
{
    fd_set write_set;
    int rslt;
    struct timeval timev = { 0, 300000 };
    
    FD_ZERO(&write_set);
    FD_SET(pb->pal.socket, &write_set);
    rslt = select(pb->pal.socket + 1, NULL, &write_set, NULL, &timev);
    if (SOCKET_ERROR == rslt) {
        PUBNUB_LOG_ERROR("pbpal_connected(): select() Error!\n");
        return pbpal_connect_resource_failure;
    }
    else if (rslt > 0) {
        PUBNUB_LOG_TRACE("pbpal_connected(): select() event\n");
        return pbpal_connect_success;
    }
    PUBNUB_LOG_TRACE("pbpal_connected(): no select() events\n");
    return pbpal_connect_wouldblock;
}
