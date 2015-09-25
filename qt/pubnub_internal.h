/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_INTERNAL
#define      INC_PUBNUB_INTERNAL



typedef int pb_socket_t;

/** The Pubnub Qt (fake) context */
struct pubnub_pal {
    /* Some compilers insist on having at least one member
     * in a struct, so, let's make them happy, but this is
     * never used in Pubnuf for Qt.
     */
    pb_socket_t fake;
};

/** The way we use Qt, blocking I/O is not applicable */
#define PUBNUB_BLOCKING_IO_SETTABLE 0


#include "pubnub_internal_common.h"



#endif /* !defined INC_PUBNUB_INTERNAL */
