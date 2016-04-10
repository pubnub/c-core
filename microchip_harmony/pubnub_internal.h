/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_INTERNAL
#define      INC_PUBNUB_INTERNAL

#include <stdint.h>

#include "tcpip.h"


#if PUBNUB_USE_SSL
#include "net/pres/net_pres_socketapi.h"

typedef NET_PRES_SKT_HANDLE_T pb_socket_t;
#define socket_connect(socket) NET_PRES_SocketConnect(socket)
#define socket_send(socket, buf, len) NET_PRES_SocketWrite((socket), (buf), (len))
#define socket_recv(socket, buf, len) NET_PRES_SocketRead((socket), (buf), (len))
#define socket_close(socket) NET_PRES_SocketDisconnect(socket)
#define SOCKET_INVALID NET_PRES_INVALID_SOCKET

#else
#include "tcp.h"

typedef TCP_SOCKET pb_socket_t;
#define socket_connect(socket) TCPIP_TCP_Connect(socket)
#define socket_send(socket, buf, len) TCPIP_TCP_ArrayPut((socket), (buf), (len))
#define socket_recv(socket, buf, len) TCPIP_TCP_ArrayGet((socket), (buf), (len))
#define socket_close(socket) TCPIP_TCP_Disconnect(socket)
#define SOCKET_INVALID INVALID_SOCKET

#endif


/** The Pubnub FreeRTOS context */
struct pubnub_pal {
    pb_socket_t socket;
};

/** Microchip Harmony doesn't really have a blocking I/O interface.
    */
#define PUBNUB_BLOCKING_IO_SETTABLE 0

#define PUBNUB_TIMERS_API 1

#define PUBNUB_CALLBACK_API 1

#include "pubnub_internal_common.h"



#endif /* !defined INC_PUBNUB_INTERNAL */
