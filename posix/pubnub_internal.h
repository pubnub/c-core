/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_INTERNAL
#define      INC_PUBNUB_INTERNAL


#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>


typedef int pb_socket_t;

#define socket_close(socket) close(socket)
#define socket_send(socket, buf, len, flags) send((socket), (buf), (len), (flags))
#define socket_recv(socket, buf, len, flags) recv((socket), (buf), (len), (flags))

/* Treating `EINPROGRESS` the same as `EWOULDBLOCK` isn't 
   the greatest solution, but it is good for now.
*/
#define socket_would_block() ((errno == EWOULDBLOCK) || (errno == EINPROGRESS))

#define socket_timed_out() (errno == ETIMEDOUT)

#define socket_platform_init() 0

#define SOCKET_INVALID -1
#define SOCKET_ERROR -1

/* Maybe we could use `getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, &error, &len)`,
    but, its utility is questionable, so probably test extensively to see if it 
    really works for us.
    */
#define socket_is_connected(socket) true

#define socket_set_rcv_timeout(socket, milliseconds) do {                           \
    struct timeval M_tm = { (milliseconds)/1000, ((milliseconds)%1000) * 1000 };    \
    setsockopt((socket), SOL_SOCKET, SO_RCVTIMEO, (char*)&M_tm, sizeof M_tm);       \
    } while(0)


/** The Pubnub POSIX context */
struct pubnub_pal {
    pb_socket_t socket;
};


/** On POSIX, one can set I/O to be blocking or non-blocking */
#define PUBNUB_BLOCKING_IO_SETTABLE 1


#define PUBNUB_TIMERS_API 1


#include "pubnub_internal_common.h"



#endif /* !defined INC_PUBNUB_INTERNAL */
