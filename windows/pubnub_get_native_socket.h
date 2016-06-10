/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_GET_NATIVE_SOCKET
#define  INC_PUBNUB_GET_NATIVE_SOCKET

#include "pubnub_internal.h"


typedef SOCKET pbpal_native_socket_t;

/** Returns the native socket of the given Pubnub context @p pb.
    If we're using native platform socket, this is trivial, but, if 
    we're using some library that "hides" the socket (like OpenSSL),
    and we need the actual platform socket for some purpose, this 
    will return the actual platform socket.
*/
pbpal_native_socket_t pubnub_get_native_socket(pubnub_t *pb);


#endif /* !defined INC_PUBNUB_GET_NATIVE_SOCKET*/
