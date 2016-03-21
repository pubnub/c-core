/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pbpal.h"

#include "pubnub_internal.h"
#include "pubnub_assert.h"
#include "pubnub_log.h"
#include "pbpal_adns_sockets.h"

#include <sys/types.h>


#define HTTP_PORT_STRING "80"
#define HTTP_PORT 80

#define DNS_PORT 53


#if !PUBNUB_USE_ADNS
#define send_dns_query(x,y,z) -1
#define read_response(x,y,z,v) -1
#endif


enum pubnub_res pbpal_resolv_and_connect(pubnub_t *pb)
{
    int error;
    char const* origin = PUBNUB_ORIGIN_SETTABLE ? pb->origin : PUBNUB_ORIGIN;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_ASSERT_OPT((pb->state == PBS_IDLE) || (pb->state == PBS_WAIT_DNS));

    if (PUBNUB_USE_ADNS) {
        int flags;
        struct sockaddr_in dest;
        int skt = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        if (SOCKET_INVALID == skt) {
            return PNR_ADDR_RESOLUTION_FAILED;
        }
        pb->options.use_blocking_io = false;
        pbpal_set_blocking_io(pb);
        dest.sin_family = AF_INET;
        dest.sin_port = htons(DNS_PORT);
        inet_pton(AF_INET, "8.8.8.8", &dest.sin_addr.s_addr);
        flags = send_dns_query(skt, (struct sockaddr*)&dest, (unsigned char*)origin);
        if (flags != 0) {
            socket_close(skt);
            return (flags < 0) ? PNR_ADDR_RESOLUTION_FAILED : PNR_STARTED;
        }
        pb->pal.socket = skt;
        return PNR_STARTED;
    }
    else {
        struct addrinfo *result;
        struct addrinfo *it;
        struct addrinfo hint;

        hint.ai_socktype = SOCK_STREAM;
        hint.ai_family = AF_UNSPEC;
        hint.ai_protocol = hint.ai_flags = hint.ai_addrlen = 0;
        hint.ai_addr = NULL;
        hint.ai_canonname = NULL;
        hint.ai_next = NULL; 
        error = getaddrinfo(origin, HTTP_PORT_STRING, &hint, &result);
        if (error != 0) {
            return PNR_ADDR_RESOLUTION_FAILED;
        }
        
        for (it = result; it != NULL; it = it->ai_next) {
            pb->pal.socket = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
            if (pb->pal.socket == SOCKET_INVALID) {
                continue;
            }
            if (connect(pb->pal.socket, it->ai_addr, it->ai_addrlen) == SOCKET_ERROR) {
                socket_close(pb->pal.socket);
                pb->pal.socket = -1;
                continue;
            }
            break;
        }
        freeaddrinfo(result);
    
        if (NULL == it) {
            return PNR_CONNECT_FAILED;
        }

        socket_set_rcv_timeout(pb->pal.socket, pb->transaction_timeout_ms);
    }

    return PNR_OK;
}


enum pubnub_res pbpal_check_resolv_and_connect(pubnub_t *pb)
{
    if (PUBNUB_USE_ADNS) {
        uint8_t const* origin = (uint8_t*)(PUBNUB_ORIGIN_SETTABLE ? pb->origin : PUBNUB_ORIGIN);
        struct sockaddr_in dns_server;
        struct sockaddr_in dest;
        int skt = pb->pal.socket;
        dns_server.sin_family = AF_INET;
        dns_server.sin_port = htons(DNS_PORT);
        inet_pton(AF_INET, "8.8.8.8", &dns_server.sin_addr.s_addr);
        switch (read_response(skt, (struct sockaddr*)&dns_server, origin, &dest)) {
        case -1: return PNR_ADDR_RESOLUTION_FAILED;
        case +1: return PNR_STARTED;
        case 0: break;
        }
        socket_close(skt);
        skt = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (SOCKET_INVALID == skt) {
            return PNR_ADDR_RESOLUTION_FAILED;
        }
        pb->pal.socket = skt;
        dest.sin_port = htons(HTTP_PORT);
        if (SOCKET_ERROR == connect(skt, (struct sockaddr*)&dest, sizeof dest)) {
            return socket_would_block() ? PNR_IN_PROGRESS : PNR_ADDR_RESOLUTION_FAILED;
        }
        
        return PNR_OK;
    }

    return PNR_OK;
}
