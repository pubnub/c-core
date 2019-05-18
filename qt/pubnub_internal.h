/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_INTERNAL
#define      INC_PUBNUB_INTERNAL



typedef int pb_socket_t;

/** The Pubnub Qt (fake) context */
struct pubnub_pal {
    /* Some compilers insist on having at least one member
     * in a struct, so, let's make them happy, but this is
     * never used in Pubnub for Qt.
     */
    pb_socket_t fake;
};

/** The way we use Qt, blocking I/O is not applicable */
#define PUBNUB_BLOCKING_IO_SETTABLE 0

/** The maximum channel name length */
#define PUBNUB_MAX_CHANNEL_NAME_LENGTH 92

#if !defined(PUBNUB_USE_ADVANCED_HISTORY)
/** If true (!=0) will enable using the advanced history API, which
    provides more data about (unread) messages. */
#define PUBNUB_USE_ADVANCED_HISTORY 1
#endif


#include "core/pubnub_internal_common.h"



#endif /* !defined INC_PUBNUB_INTERNAL */
