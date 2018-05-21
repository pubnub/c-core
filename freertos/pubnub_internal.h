/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_INTERNAL
#define      INC_PUBNUB_INTERNAL


#include "FreeRTOS.h"

#include "task.h"

#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"


#define socket_close(socket) FreeRTOS_closesocket(socket)
#define socket_send(socket, buf, len) FreeRTOS_send((socket), (buf), (len), 0)
#define socket_recv(socket, buf, len, flags) FreeRTOS_recv((socket), (buf), (len), (flags))

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

#define SOCKET_INVALID FREERTOS_INVALID_SOCKET


/* FreeRTOS+TCP never raises SIGPIPE, so, we're good. */
#define socket_disable_SIGPIPE(socket)


typedef Socket_t pb_socket_t;

/** The Pubnub FreeRTOS context */
struct pubnub_pal {
    pb_socket_t socket;
};

/** The sockets interface of FreeRTOS+TCP, doesn't provide a
    non-blocking blocking I/O. Another implementation may be
    provided that uses the FreeRTOS+TCP callback interface
    which is, by design, non-blocking (although, it is not
    really documented as a user API, yet).
    */
#define PUBNUB_BLOCKING_IO_SETTABLE 0

#define PUBNUB_TIMERS_API 1


#include "pubnub_internal_common.h"



#endif /* !defined INC_PUBNUB_INTERNAL */
