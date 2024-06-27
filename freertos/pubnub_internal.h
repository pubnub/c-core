/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_INTERNAL
#define      INC_PUBNUB_INTERNAL


#include "FreeRTOS.h"

#include "task.h"

#ifndef ESP_PLATFORM

#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"

#define socket_close(socket) FreeRTOS_closesocket(socket)
#define socket_send(socket, buf, len) FreeRTOS_send((socket), (buf), (len), 0)
#define socket_recv(socket, buf, len, flags) FreeRTOS_recv((socket), (buf), (len), (flags))
#define htons(port) FreeRTOS_htons(port)
#define gethostbyname(name) FreeRTOS_gethostbyname(name)
#define getnameinfo(addr, addrlen, host, hostlen, serv, servlen, flags) FreeRTOS_getnameinfo((addr), (addrlen), (host), (hostlen), (serv), (servlen), (flags))
#define socket(family, type, protocol) FreeRTOS_socket((family), (type), (protocol))
#define socket_connect(socket, addr, addrlen) FreeRTOS_connect((socket), (addr), (addrlen))
#define socket_setsockopt(socket, level, optname, optval, optlen) FreeRTOS_setsockopt((socket), (level), (optname), (optval), (optlen))
#define AF_INET FREERTOS_AF_INET
#define SOCK_STREAM FREERTOS_SOCK_STREAM
#define IPPROTO_TCP FREERTOS_IPPROTO_TCP
#define SO_RCVTIMEO FREERTOS_SO_RCVTIMEO
#define sockaddr freertos_sockaddr

typedef Socket_t pb_socket_t;

#define SOCKET_INVALID FREERTOS_INVALID_SOCKET

#else 

#include "lwip/netdb.h"
#include "lwip/inet.h"
#include "lwip/ip_addr.h"

#define socket_close(socket) closesocket(socket)
#define socket_send(socket, buf, len) send((socket), (buf), (len), 0)
#define socket_recv(socket, buf, len, flags) recv((socket), (buf), (len), (flags))
#define gethostbyname(name) lwip_gethostbyname(name)
#define getnameinfo(addr, addrlen, host, hostlen, serv, servlen, flags) lwip_getnameinfo((addr), (addrlen), (host), (hostlen), (serv), (servlen), (flags))
#define socket(family, type, protocol) lwip_socket((family), (type), (protocol))
#define socket_connect(socket, addr, addrlen) lwip_connect((socket), (addr), (addrlen))
#define socket_setsockopt(socket, level, optname, optval, optlen) lwip_setsockopt((socket), (level), (optname), (optval), (optlen))
#define sockaddr sockaddr_in

typedef int pb_socket_t;

#define SOCKET_INVALID ENETDOWN

#endif


/** FreeRTOS+TCP always blocks, so checking "if it would block"
    doesn't make sense */
#define socket_would_block() 0

/** FreeRTOS+TCP returns 0 on timeout - since we already treat that as
    a timeout, further checking is only in situations when it certainly
    isn't a timeout.
    */
#define socket_timed_out() 0

/** Although the FreeRTOS+TCP requires TCP/IP initialization,
    we expect the user to do it elsewhere, to avoid coupling
    the TCP/IP stack initiliazation to Pubnub, as TCP/IP may
    be used for other purposes by the user.
    */
#define socket_platform_init() 0

#define socket_is_connected(socket) (pdTRUE == FreeRTOS_issocketconnected(socket))



/* FreeRTOS+TCP never raises SIGPIPE, so, we're good. */
#define socket_disable_SIGPIPE(socket)


#ifndef PUBNUB_MBEDTLS
/** Flag to indicate that we are using MBEDTLS library support */
#define PUBNUB_MBEDTLS 1
#endif

// TODO: move pubnub_pal to other header to avoid multiple definitions
// of the same struct
// Maybe modularization of the Pubnub library is needed sooner than
// later in this SDK
#if !PUBNUB_MBEDTLS
/** The Pubnub FreeRTOS context */
struct pubnub_pal {
    pb_socket_t socket;
};
#else
#include "mbedtls/pubnub_pal.h"
#endif

/** The sockets interface of FreeRTOS+TCP, doesn't provide a
    non-blocking blocking I/O. Another implementation may be
    provided that uses the FreeRTOS+TCP callback interface
    which is, by design, non-blocking (although, it is not
    really documented as a user API, yet).
    */
#define PUBNUB_BLOCKING_IO_SETTABLE 0

#define PUBNUB_TIMERS_API 1

#if !defined(PUBNUB_MAX_URL_PARAMS)
/** The maximum number of URL parameters that can be saved in the Pubnub
    context.
*/
#define PUBNUB_MAX_URL_PARAMS 10 
#endif

#if !defined(PUBNUB_MIN_WAIT_CONNECT_TIMER)
/** The minimum duration of the wait connect timer, in milliseconds. */
#define PUBNUB_MIN_WAIT_CONNECT_TIMER 5000 
#endif

#if !defined(PUBNUB_DEFAULT_WAIT_CONNECT_TIMER)
/** The default duration of the wait connect timer, in milliseconds. */
#define PUBNUB_DEFAULT_WAIT_CONNECT_TIMER 10000
#endif

#include "pubnub_internal_common.h"

#endif /* !defined INC_PUBNUB_INTERNAL */
