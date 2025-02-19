/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_CALLBACK_SUBSCRIBE_LOOP
#define INC_PUBNUB_CALLBACK_SUBSCRIBE_LOOP


#include "pubnub_api_types.h"
#include "pubnub_coreapi_ex.h"
#include "lib/pb_extern.h"


/** @file pubnub_callback_subscribe_loop.h 

    This module implements a subscribe loop for the callback interface.
*/


/** A subscribe loop descriptor. Has data that is needed to run a
    loop. An opaque data structure.
 */
struct pubnub_subloop_descriptor;

/** A helper typedef of a subscribe loop descriptor */
typedef struct pubnub_subloop_descriptor pubnub_subloop_t;

/** Prototype of a function that will be called back when a message is
    received during a subscribe loop, or on a failed subscribe.
*/
typedef void (*pubnub_subloop_callback_t)(pubnub_t *pbp, char const* message, enum pubnub_res result);

/** Helper to create a subscribe loop descriptor. For the values that
    are part of the descriptor, but are not provided as parameters of
    this function, defaults will be used.

    @par Basic usage
    @snippet pubnub_callback_subloop_sample.c Define subscribe Loop

    @param[in] p The Pubnub context to use for the loop
    @param[in] channel The channel(s) to subscribe to
    @param[in] options Subscribe options to use. To get the default
    options, use pubnub_subscribe_defopts()
    @param[in] cb (pointer to) Function to be called on each received message

    @retval NULL Failed to create a descriptor
    @result The subscribe loop descriptor created 
 */
#if defined PUBNUB_NTF_RUNTIME_SELECTION
PUBNUB_EXTERN pubnub_subloop_t* pubnub_callback_subloop_define(pubnub_t *p, char const *channel, struct pubnub_subscribe_options options, pubnub_subloop_callback_t cb);
#else
PUBNUB_EXTERN pubnub_subloop_t* pubnub_subloop_define(pubnub_t *p, char const *channel, struct pubnub_subscribe_options options, pubnub_subloop_callback_t cb);
#endif

/** Starts a subscribe loop.

    The callback given in pubnub_subloop_define() will be called for
    each message received.

    A running subscribe loop will "take-over" the context and it
    will receive all the callbacks for it.

    @warning Changing the callback on the context during the
    running of the subscribe loop will "break" the loop.

    @warning You should _not_ change the subscribe loop descriptor
    while the loop is "running". Stop it first, with
    pubnub_subloop_stop().

    @par Basic usage
    @snippet pubnub_sync_subloop_sample.c Subscribe Loop

    @param[in] pbsld The subscribe loop descriptor 

    @retval PNR_OK Success
    @retval other Indicates the reason for failure 
 */
PUBNUB_EXTERN enum pubnub_res pubnub_subloop_start(pubnub_subloop_t* pbsld);

/** Stops a subscribe loop. If loop is calling the callback
    ("delivering message(s)"), stop will be done once that is
    finished.

    After a stop of the subscribe loop, the context can be
    used in a "regular" manner. The callback that was set
    on the context will be restored.

    @param[in] pbsld The subscribe loop descriptor 
    
 */
PUBNUB_EXTERN void pubnub_subloop_stop(pubnub_subloop_t* pbsld);

/** Undefines - releases the subscribe loop descriptor */
PUBNUB_EXTERN void pubnub_subloop_undef(pubnub_subloop_t* pbsld);


#endif /* !defined INC_PUBNUB_CALLBACK_SUBSCRIBE_LOOP */
