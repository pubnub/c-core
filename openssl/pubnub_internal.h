/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_INTERNAL
#define INC_PUBNUB_INTERNAL

#include "lib/msstopwatch/msstopwatch.h"

#ifdef _WIN32
/* Seems that since some version of OpenSSL (maybe in collusion
   w/Windows SDK version), one needs to include Winsock header(s)
   before including OpenSSL headers... This is kinda-ugly, but, until
   we find a better solution...
*/
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#endif

#include "pubnub_get_native_socket.h"

#include "openssl/bio.h"
#include "openssl/ssl.h"
#include "openssl/err.h"


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

/** On Windows, one needs to call WSAStartup(), which is not trivial */
int socket_platform_init(void);

#if !defined(_WIN32)

typedef int pb_socket_t;

#define SOCKET_INVALID -1
#define SOCKET_ERROR -1

#define socket_close(socket) close(socket)

/* Treating `EINPROGRESS` the same as `EWOULDBLOCK` isn't
   the greatest solution, but it is good for now.
*/
#define socket_would_block()                                                   \
    ((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINPROGRESS))

#define socket_timed_out() (errno == ETIMEDOUT)

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

#else
typedef SOCKET pb_socket_t;

#define SOCKET_INVALID INVALID_SOCKET

#define socket_close(socket) closesocket(socket)

#define socket_would_block()                                                   \
    ((WSAGetLastError() == WSAEWOULDBLOCK)                                     \
     || (WSAGetLastError() == WSAEINPROGRESS))

#define socket_timed_out() (WSAGetLastError() == WSAETIMEDOUT)

/* Winsock never raises SIGPIPE, so, we're good. */
#define socket_disable_SIGPIPE(socket)

#endif

/* Maybe we could use `getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, &error,
   &len)`, but, its utility is questionable, so probably test extensively to see
   if it really works for us.
    */
#define socket_is_connected(socket) true

/** If you need the 12 bytes of memory space and are _sure_ you'll
    only work on IPv4, set to 4 (because 16 is for IPv6)
*/
#define PUBNUB_MAX_IP_ADDR_OCTET_LENGTH 16

/** The Pubnub OpenSSL context */
struct pubnub_pal {
    pbpal_native_socket_t socket;
    SSL*         ssl;
    SSL_CTX*     ctx;
    SSL_SESSION* session;
    char         ip[PUBNUB_MAX_IP_ADDR_OCTET_LENGTH];
    size_t       ip_len;
    int          ip_family;
    time_t       ip_timeout;
    pbmsref_t    tryconn;
};

#ifdef _WIN32
#define socket_set_rcv_timeout(socket, milliseconds)                              \
    do {                                                                          \
        DWORD M_tm = (milliseconds);                                              \
        setsockopt((socket), SOL_SOCKET, SO_RCVTIMEO, (char*)&M_tm, sizeof M_tm); \
    } while (0)

#if _MSC_VER < 1900
/** Microsoft C compiler (before VS2015) does not provide a
    standard-conforming snprintf(), so we bring our own.
    */
int snprintf(char* buffer, size_t n, const char* format, ...);
#endif

#else

#include <unistd.h>
#include <sys/socket.h>

#define socket_set_rcv_timeout(socket, milliseconds)                              \
    do {                                                                          \
        struct timeval M_tm = { (milliseconds) / 1000,                            \
                                ((milliseconds) % 1000) * 1000 };                 \
        setsockopt((socket), SOL_SOCKET, SO_RCVTIMEO, (char*)&M_tm, sizeof M_tm); \
    } while (0)

#endif /* ifdef _WIN32 */

/** With OpenSSL, one can set I/O to be blocking or non-blocking,
    though it can only be done before establishing the connection.
*/
#define PUBNUB_BLOCKING_IO_SETTABLE 1

#define PUBNUB_TIMERS_API 1


#include "core/pubnub_internal_common.h"


#endif /* !defined INC_PUBNUB_INTERNAL */
