/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_GET_NATIVE_SOCKET
#define  INC_PUBNUB_GET_NATIVE_SOCKET

#include "pubnub_internal.h"

typedef int pbpal_native_socket_t;

/** Returns the native socket under POSIX of the given Pubnub context.
 */
pbpal_native_socket_t pubnub_get_native_socket(pubnub_t *pb);


#endif /* !defined INC_PUBNUB_GET_NATIVE_SOCKET*/
