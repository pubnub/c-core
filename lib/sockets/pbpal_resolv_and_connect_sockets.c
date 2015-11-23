/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pbpal.h"

#include "pubnub_internal.h"
#include "pubnub_assert.h"

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

    {
#if defined _WIN32
        DWORD tmval = 310 * 1000;
#else
        struct timeval tmval = { 310, 0 };
#endif
        setsockopt(pb->pal.socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&tmval, sizeof tmval);
    }

    switch (pbntf_got_socket(pb, pb->pal.socket)) {
    case 0: return PNR_STARTED; /* Should really be PNR_OK, see below */
    case +1: return PNR_STARTED;
    case -1: default: return PNR_CONNECT_FAILED;
    }
    /* If we return PNR_OK, then the whole transaction can finish
       in one call to Netcore FSM. That would be nice, but some
       tests want to be able to cancel a request, which would
       then be impossible. So, until we figure out how to handle
       that, we shall return PNR_STARTED.
    */
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
        DEBUG_PRINTF("select() Error!\n");
        return PNR_CONNECT_FAILED;
    }
    else if (rslt > 0) {
        DEBUG_PRINTF("select() event\n");
        return pbpal_resolv_and_connect(pb);
    }
    DEBUG_PRINTF("no select() events\n");
    return PNR_IN_PROGRESS;
}
