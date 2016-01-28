/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pbpal.h"

#include "pubnub_internal.h"
#include "pubnub_assert.h"
#include "pubnub_log.h"

#include <sys/types.h>

#if !defined _WIN32
#include <sys/select.h>
#endif


#define HTTP_PORT_STRING "80"


enum pubnub_res pbpal_resolv_and_connect(pubnub_t *pb)
{
    struct addrinfo *result;
    struct addrinfo *it;
    struct addrinfo hint;
    int error;
    char const* origin = PUBNUB_ORIGIN_SETTABLE ? pb->origin : PUBNUB_ORIGIN;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_ASSERT_OPT((pb->state == PBS_IDLE) || (pb->state == PBS_WAIT_DNS));

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
        if (connect(pb->pal.socket, it->ai_addr, it->ai_addrlen) == -1) {
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

    return PNR_OK;
}


enum pubnub_res pbpal_check_resolv_and_connect(pubnub_t *pb)
{
    fd_set read_set, write_set;
    int rslt;
    struct timeval timev = { 0, 300000 };
    
    FD_ZERO(&read_set);
    FD_ZERO(&write_set);
    FD_SET(pb->pal.socket, &read_set);
    FD_SET(pb->pal.socket, &write_set);
    rslt = select(pb->pal.socket + 1, &read_set, &write_set, NULL, &timev);
    if (-1 == rslt) {
        PUBNUB_LOG_ERROR("select() Error!\n");
        return PNR_CONNECT_FAILED;
    }
    else if (rslt > 0) {
        PUBNUB_LOG_TRACE("select() event\n");
        return pbpal_resolv_and_connect(pb);
    }
    PUBNUB_LOG_TRACE("no select() events\n");
    return PNR_IN_PROGRESS;
}
