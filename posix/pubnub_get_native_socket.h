/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_GET_NATIVE_SOCKET
#define  INC_PUBNUB_GET_NATIVE_SOCKET

#include "pubnub_internal.h"


/** Returns the native socket under POSIX of the given Pubnub context.
    On POSIX, native (BSD) socket is an integer handle.
 */
int pubnub_get_native_socket(pubnub_t *pb);


#endif /* !defined INC_PUBNUB_GET_NATIVE_SOCKET*/
