/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_GET_NATIVE_SOCKET
#define  INC_PUBNUB_GET_NATIVE_SOCKET

#include "pubnub_internal.h"


/** Returns the native socket under POSIX of the given Pubnub context.
    On Windows, native socket is a `SOCKET`, which is a `HANDLE` (most probably)
    , which is an opaque pointer.
 */
SOCKET pubnub_get_native_socket(pubnub_t *pb);


#endif /* !defined INC_PUBNUB_GET_NATIVE_SOCKET*/
