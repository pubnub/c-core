/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_SYNC_SUBSCRIBE_LOOP
#define	INC_PUBNUB_SYNC_SUBSCRIBE_LOOP


#include "pubnub_api_types.h"
#include "pubnub_coreapi_ex.h"
#include "lib/pb_extern.h"


/** @file pubnub_sync_subscribe_loop.h 

    This module implements a subscribe loop for the sync interface.
*/


/** A subscribe loop descriptor. Has data that is needed to run a
    loop. It doesn't own any of this data, the lifetime is "someone
    else's problem".
 */
struct pubnub_subloop_descriptor {
    /** The context to use */
    pubnub_t *pbp;
    /** Channel to subscribe to */
    char const* channel;
    /** Extended subscribe options */
    struct pubnub_subscribe_options options;
};

/** Helper to make a subscribe loop descriptor, which it returns, by
    value. For the values that are part of the descriptor, but are not
    provided as parameters of this function, defaults will be used.

    @par Basic usage
    @snippet pubnub_sync_subloop_sample.c Define subscribe Loop

    @param[in] p The Pubnub context to use for the loop
    @param[in] channel The channel(s) to subscribe to

    @result The subscribe loop descriptor made 
 */
PUBNUB_EXTERN struct pubnub_subloop_descriptor pubnub_subloop_define(pubnub_t *p, char const *channel);

/** Designed to be called once in every iteration of a subscribe loop.
    Fetches the next message on the given @p channel and/or @p
    channel_group using the context @p p, automatically subscribing if
    there are no messages left in @p p. Thus, this might block for a
    significant time waiting for message(s) to arrive.

    @remark Changing the @p pbsld descriptor during the loop is
    possible, but your changes may take many iterations of the loop to
    take effect.

    @par Basic usage
    @snippet pubnub_sync_subloop_sample.c Subscribe Loop

    @param[in] pbsld The subscribe loop descriptor 
    @param[out] message The message that was fetched, or NULL if no
    message fetched

    @retval PNR_OK Success
    @retval other Indicates the reason for failure 
 */
PUBNUB_EXTERN enum pubnub_res pubnub_subloop_fetch(struct pubnub_subloop_descriptor const* pbsld, char const** message);


#endif /* !defined INC_PUBNUB_SYNC_SUBSCRIBE_LOOP */
