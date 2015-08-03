/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_INTERNAL
#define      INC_PUBNUB_INTERNAL


#include <winsock2.h>
#include <ws2tcpip.h>


typedef SOCKET pb_socket_t;


/** The Pubnub POSIX context */
struct pubnub_pal {
    pb_socket_t socket;

};


/** On Windows, one can set I/O to be blocking or non-blocking */
#define PUBNUB_BLOCKING_IO_SETTABLE 1


#if defined(_MSC_VER)
/** Microsoft C compiler (at least up to VS2015) does not provide a 
	standard-conforming snprintf(), so we bring our own.
	*/
int snprintf(char *buffer, size_t n, const char *format, ...);
#endif


#include "pubnub_internal_common.h"


#endif /* !defined INC_PUBNUB_INTERNAL */
