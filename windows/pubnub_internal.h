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

#include "pubnub_internal_common.h"



#endif /* !defined INC_PUBNUB_INTERNAL */
