/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_INTERNAL
#define INC_PUBNUB_INTERNAL


#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>


typedef int pb_socket_t;

#define socket_close(socket) close(socket)

#if defined(__linux__)
/* We assume that one doesn't want to receive and handle SIGPIPE.
   But we may upgrade this to a compile time option in the future,
   if people actually want to handle SIGPIPE.
*/
#define socket_send(socket, buf, len) send((socket), (buf), (len), MSG_NOSIGNAL)
#else
#define socket_send(socket, buf, len) send((socket), (buf), (len), 0)
#endif

#define socket_recv(socket, buf, len, flags)                                   \
    recv((socket), (buf), (len), (flags))

/* Treating `EINPROGRESS` the same as `EWOULDBLOCK` isn't
   the greatest solution, but it is good for now.
*/
#define socket_would_block()                                                   \
    ((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINPROGRESS))

#define socket_timed_out() (errno == ETIMEDOUT)

#define socket_platform_init() 0

#define SOCKET_INVALID -1
#define SOCKET_ERROR -1

/* Maybe we could use `getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, &error,
   &len)`, but, its utility is questionable, so probably test extensively to see
   if it really works for us.
    */
#define socket_is_connected(socket) true

#define socket_set_rcv_timeout(socket, milliseconds)                              \
    do {                                                                          \
        struct timeval M_tm = { (milliseconds) / 1000,                            \
                                ((milliseconds) % 1000) * 1000 };                 \
        setsockopt((socket), SOL_SOCKET, SO_RCVTIMEO, (char*)&M_tm, sizeof M_tm); \
    } while (0)


#if __APPLE__
/* Actually, BSD in general provides SO_NOSIGPIPE, but we don't know
   what's the status of this support across various BSDs... So, for
   now, we only do this for MacOS.
*/
#define socket_disable_SIGPIPE(socket)                                             \
    do {                                                                           \
        int on = 1;                                                                \
        if (setsockopt(socket, SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof(on)) == -1) { \
            PUBNUB_LOG_WARNING("Failed to set SO_NOSIGPIPE, errno=%d\n", errno);   \
        }                                                                          \
    } while (0)
#else
#define socket_disable_SIGPIPE(socket)
#endif


/** The Pubnub POSIX context */
struct pubnub_pal {
    pb_socket_t socket;
};


/** On POSIX, one can set I/O to be blocking or non-blocking */
#define PUBNUB_BLOCKING_IO_SETTABLE 1


#define PUBNUB_TIMERS_API 1


#include "core/pubnub_internal_common.h"


#endif /* !defined INC_PUBNUB_INTERNAL */
