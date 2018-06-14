/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_GET_NATIVE_SOCKET
#define  INC_PUBNUB_GET_NATIVE_SOCKET

#include "core/pubnub_api_types.h"

#if defined(_WIN32)
/// On Windows, native socket is a `SOCKET`, which is a `HANDLE` (most probably), 
/// which is an opaque pointer.
#include <winsock2.h>
typedef SOCKET pbpal_native_socket_t;
#else
/// On POSIX, native(BSD) socket is an integer handle.
typedef int pbpal_native_socket_t;
#endif

/** Returns the native socket of the given Pubnub context @p pb.
    If we're using native platform socket, this is trivial, but, if 
    we're using some library that "hides" the socket (like OpenSSL),
    and we need the actual platform socket for some purpose, this 
    will return the actual platform socket.
*/
pbpal_native_socket_t pubnub_get_native_socket(pubnub_t *pb);


#endif /* !defined INC_PUBNUB_GET_NATIVE_SOCKET*/
