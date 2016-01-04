/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_INTERNAL
#define      INC_PUBNUB_INTERNAL


#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>

#define closesocket(x) close(x)


typedef int pb_socket_t;

/** The Pubnub POSIX context */
struct pubnub_pal {
    pb_socket_t socket;

};

#define PUBNUB_BLOCKING_IO_SETTABLE 1
#define PUBNUB_ORIGIN_SETTABLE 1

#define PUBNUB_TIMERS_API 1

#include "pubnub_internal_common.h"



#endif /* !defined INC_PUBNUB_INTERNAL */
