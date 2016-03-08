/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_SUBSCRIBE_WITH_STATE
#define	INC_PUBNUB_SUBSCRIBE_WITH_STATE


#include "pubnub_api_types.h"

#include <stdbool.h>


/** @file pubnub_subscribe_with_state.h This is the API of "subscribe
    with state" Pubnub transaction.  It is deprecated, provided as a
    workaround for existing users.
*/


/** Starts a "subscribe with state" Pubnub transaction. There are
    various subtle differences in behavior when subscribing with state
    (as opposed to "regular" subscribing without state).

    It is in all aspects quite similar to pubnub_subscribe(), except that
    the caller should provide a JSON object @p state.

    @deprecated This is provided as a workaround for existing
    users. Please switch to "set state, then subscribe" or other usage
    pattern.

    @param p The pubnub context. Can't be NULL
    @param channel The string with the channel name (or comma-delimited list
    of channel names) to subscribe to.
    @param channel_group The string with the channel group name (or
    comma-delimited list of channel group names) to subscribe to.
    @param state A string with a JSON object which identifies the state

    @return #PNR_STARTED on success, an error otherwise
    
    @see pubnub_subscribe
 */
enum pubnub_res pubnub_subscribe_with_state(pubnub_t *p, const char *channel, const char *channel_group, char const *state);



#endif /* defined INC_PUBNUB_SUBSCRIBE_WITH_STATE */
