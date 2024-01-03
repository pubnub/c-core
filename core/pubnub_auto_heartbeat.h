/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#if !defined INC_PUBNUB_AUTO_HEARTBEAT
#define INC_PUBNUB_AUTO_HEARTBEAT

#if PUBNUB_USE_AUTO_HEARTBEAT

#include "lib/pb_extern.h"

/** Enables periodical heartbeats that keep presence on subscribed channels and channel
    groups for user id provided in @p pb context and sets chosen heartbeat period.
    Initially auto heartbeat on @p pb context is disabled.

    This module keeps presence by performing pubnub_heartbeat() transaction periodicaly
    with user id given whenever subscription on @p pb context is not in progress and
    auto heartbeat is enabled. This process is independent from anything user
    may be doing with the context when its not subscribing(, or heartbeating).

    If the user id(or any other relevant data, like dns server, or proxy) is changed at
    some point, the module will update it automatically in its heartbeats.
    The same goes if the user id leaves some of the channels, or channel groups.

    @param pb The pubnub context. Can't be NULL
    @param period_sec Auto heartbeat thumping period in seconds
    @return 0 on success, -1 otherwise
  */
PUBNUB_EXTERN int pubnub_enable_auto_heartbeat(pubnub_t* pb, size_t period_sec);

/** Changes auto heartbeat thumping period. If auto heartbeat is desabled on
    the @p pb context the period wan't be changed and function returns error.
    @param pb The pubnub context. Can't be NULL
    @param period_sec Auto heartbeat thumping period in seconds
    @return 0 on success, -1 otherwise
  */
PUBNUB_EXTERN int pubnub_set_heartbeat_period(pubnub_t* pb, size_t period_sec);

/** Disables auto heartbeat on the @p pb context.
  */
PUBNUB_EXTERN void pubnub_disable_auto_heartbeat(pubnub_t* pb);

/** Returns if auto heartbeat is enabled on the @p pb context.
  */
PUBNUB_EXTERN bool pubnub_is_auto_heartbeat_enabled(pubnub_t* pb);

/** Releases all allocated heartbeat thumpers.
  */
PUBNUB_EXTERN void pubnub_heartbeat_free_thumpers(void);
#else
#define pubnub_enable_auto_heartbeat(pb, period_sec) -1
#define pubnub_set_heartbeat_period(pb, period_sec) -1
#define pubnub_disable_auto_heartbeat(pb)
#define pubnub_is_auto_heartbeat_enabled(pb) false
#define pubnub_heartbeat_free_thumpers()
#endif /* PUBNUB_USE_AUTO_HEARTBEAT */

#endif /* !defined INC_PUBNUB_AUTO_HEARTBEAT */
