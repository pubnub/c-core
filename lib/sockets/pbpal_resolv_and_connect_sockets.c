/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pbpal.h"

#define POSIX
#include "pubnub_internal.h"
#include "pubnub_assert.h"

#include <sys/types.h>

#if defined _WIN32
/* We're good, no need for renaming */
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#define closesocket(x) close(x)
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif


#define HTTP_PORT_STRING "80"


enum pubnub_res pbpal_resolv_and_connect(pubnub_t *pb)
{
    struct addrinfo *result;
    struct addrinfo *it;
    struct addrinfo hint;
    int error;
    
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_ASSERT_OPT((pb->state == PBS_IDLE) || (pb->state == PBS_WAIT_DNS));

    hint.ai_socktype = SOCK_STREAM;
    hint.ai_family = AF_UNSPEC;
    hint.ai_protocol = hint.ai_flags = hint.ai_addrlen = 0;
    hint.ai_addr = NULL;
    hint.ai_canonname = NULL;
    hint.ai_next = NULL; 
    error = getaddrinfo(PUBNUB_ORIGIN, HTTP_PORT_STRING, &hint, &result);
    if (error != 0) {
        return PNR_ADDR_RESOLUTION_FAILED;
    }
    for (it = result; it != NULL; it = it->ai_next) {
        pb->pal.socket = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
        if (pb->pal.socket == INVALID_SOCKET) {
            continue;
        }
        if (connect(pb->pal.socket, it->ai_addr, it->ai_addrlen) == SOCKET_ERROR) {
            closesocket(pb->pal.socket);
            pb->pal.socket = -1;
            continue;
        }
        break;
    }
    freeaddrinfo(result);

    if (NULL == it) {
        return PNR_CONNECT_FAILED;
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
