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


int pbpal_resolv_and_connect(pubnub_t *pb)
{
    struct addrinfo *result;
    struct addrinfo *res;
    int error;
    
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_ASSERT_OPT((pb->state == PBS_IDLE) || (pb->state == PBS_WAIT_DNS));

    error = getaddrinfo(PUBNUB_ORIGIN, HTTP_PORT_STRING, NULL, &result);
    if (error != 0) {
        return -1;
    }
    for (res = result; res != NULL; res = res->ai_next) {
        pb->pal.socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (pb->pal.socket == INVALID_SOCKET) {
            continue;
        }
        if (connect(pb->pal.socket, res->ai_addr, res->ai_addrlen) == SOCKET_ERROR) {
            closesocket(pb->pal.socket);
            pb->pal.socket = -1;
            continue;
        }
        break;
    }
    freeaddrinfo(result);

    if (NULL == res) {
        return -1;
    }
    
    return pbntf_got_socket(pb, pb->pal.socket);
}
