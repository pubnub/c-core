/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_KEEP_ALIVE
#define INC_PUBNUB_KEEP_ALIVE


#include "pubnub_api_types.h"
#include "lib/pb_extern.h"


/** @file pubnub_keep_alive.h

    API for setting the Keep-Alive options.
*/

#if !PUBNUB_ADVANCED_KEEP_ALIVE
#error This API is only supported if PUBNUB_ADVANCED_KEEP_ALIVE macro constant is 'true'
#endif

/** Set Keep-Alive options, according to "HYBI" Internat draft.

    @param timeout indicating the minimum amount of time an idle
    connection has to be kept opened (in seconds). Note that timeouts
    longer than the TCP timeout may be ignored if no keep-alive TCP
    message is set at the transport level. Setting to 0 has the effect
    of not using keep-alive.

    @param max indicating the maximum number of requests that can be
    sent on this connection before closing it. If 0, it is ignored.
    Setting to 1 has the effect of not using keep-alive.
 */
PUBNUB_EXTERN void pubnub_set_keep_alive_param(pubnub_t* pb, unsigned timeout, unsigned max);


#endif /* !defined INC_PUBNUB_KEEP_ALIVE */
