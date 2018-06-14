/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_INTERNAL
#define      INC_PUBNUB_INTERNAL


#include <winsock2.h>
#include <ws2tcpip.h>


typedef SOCKET pb_socket_t;


#define socket_close(socket) closesocket(socket)
#define socket_send(socket, buf, len) send((socket), (buf), (len), 0)
#define socket_recv(socket, buf, len, flags) recv((socket), (buf), (len), (flags))

/* Treating `WSAEINPROGRESS` the same as `WSAEWOULDBLOCK` isn't 
   the greatest solution, but it is good for now.
*/
#define socket_would_block() ((WSAGetLastError() == WSAEWOULDBLOCK) || (WSAGetLastError() == WSAEINPROGRESS))

#define socket_timed_out() (WSAGetLastError() == WSAETIMEDOUT)

/** On Windows, one needs to call WSAStartup(), which is not trivial */
int socket_platform_init(void);

#define SOCKET_INVALID INVALID_SOCKET

/* Maybe we could use `getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, &error, &len)`,
    but, its utility is questionable, so probably test extensively to see if it 
    really works for us.
    */
#define socket_is_connected(socket) true

#define socket_set_rcv_timeout(socket, milliseconds) do {                       \
    DWORD M_tm = (milliseconds);                                                \
    setsockopt((socket), SOL_SOCKET, SO_RCVTIMEO, (char*)&M_tm, sizeof M_tm);   \
    } while(0)


/* Winsock never raises SIGPIPE, so, we're good. */
#define socket_disable_SIGPIPE(socket)


/** The Pubnub Windows context */
struct pubnub_pal {
    pb_socket_t socket;
};


/** On Windows, one can set I/O to be blocking or non-blocking */
#define PUBNUB_BLOCKING_IO_SETTABLE 1

#define PUBNUB_TIMERS_API 1

#if defined(_MSC_VER) && (_MSC_VER < 1900)
/** Microsoft C compiler (before VS2015) does not provide a 
    standard-conforming snprintf(), so we bring our own.
    */
int snprintf(char *buffer, size_t n, const char *format, ...);
#endif


#include "core/pubnub_internal_common.h"


#endif /* !defined INC_PUBNUB_INTERNAL */
