/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"


int pubnub_get_native_socket(pubnub_t *pb)
{
    int socket;
//    printf("pubnub_get_native_socket, pb_socket=%p\n", pb_socket);
    if (-1 == BIO_get_fd(pb->pal.socket, &socket)) {
        DEBUG_PRINTF("Uninitialized BIO!\n");
        return -1;
    }
    return socket;
}
