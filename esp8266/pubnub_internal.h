/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_INTERNAL
#define      INC_PUBNUB_INTERNAL


#include "espconn.h"

typedef struct espconn * pb_socket_t;

/** The Pubnub OpenSSL context */
struct pubnub_pal {
    pb_socket_t socket;
};

#define socket_set_rcv_timeout(socket, milliseconds) /* no socket timeout, yet */

/** espconn supports only non-blocking I/O
*/
#define PUBNUB_BLOCKING_IO_SETTABLE 0

#define PUBNUB_TIMERS_API 1


#include "pubnub_internal_common.h"



#endif /* !defined INC_PUBNUB_INTERNAL */
