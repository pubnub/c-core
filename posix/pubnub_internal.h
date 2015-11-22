/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_INTERNAL
#define      INC_PUBNUB_INTERNAL


#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>


typedef int pb_socket_t;

#define socket_close(socket) close(socket)
#define socket_send(socket, buf, len, flags) send((socket), (buf), (len), (flags))
#define socket_recv(socket, buf, len, flags) recv((socket), (buf), (len), (flags))

#define socket_would_block() (errno == EWOULDBLOCK)

#define socket_platform_init 0

#define SOCKET_INVALID -1


/** The Pubnub POSIX context */
struct pubnub_pal {
    pb_socket_t socket;
};


/** On POSIX, one can set I/O to be blocking or non-blocking */
#define PUBNUB_BLOCKING_IO_SETTABLE 1


#include "pubnub_internal_common.h"



#endif /* !defined INC_PUBNUB_INTERNAL */
