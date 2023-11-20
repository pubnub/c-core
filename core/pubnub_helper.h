/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_HELPER
#define	INC_PUBNUB_HELPER


#include "pubnub_api_types.h"
#include "pbpal.h"
#include "lib/pb_extern.h"


/** @file pubnub_helper.h 

    This is a set of "Helper" functions of the Pubnub client library.
    You don't need this to work with Pubnub client, but they may come
    in handy.
*/


/** Possible (known) publish results (outcomes) in the description
    received from Pubnub.
 */
enum pubnub_publish_res {
    /** Publish succeeded, message sent on the channel */
    PNPUB_SENT,
    /** Publish failed, the message had invalid JSON */
    PNPUB_INVALID_JSON,
    /** Publish failed, the channel had an invalid character in the name */
    PNPUB_INVALID_CHAR_IN_CHAN_NAME,
    /** The publishing quota for this account was exceeded */
    PNPUB_ACCOUNT_QUOTA_EXCEEDED,
    /** Message is too large to be published */
    PNPUB_MESSAGE_TOO_LARGE,
    /** The publish key is invalid */
    PNPUB_INVALID_PUBLISH_KEY,
    /** The subscribe key is invalid */
    PNPUB_INVALID_SUBSCRIBE_KEY,
    /** Publish failed and the response is a JSON object we weren't
        able to interpret.
    */
    PNPUB_UNKNOWN_JSON_OBJECT,
    /** Publish failed, but we were not able to parse the error description */
    PNPUB_UNKNOWN_ERROR,
};

/** Parses the given publish @p result. You usually obtain this with
    pubnub_last_publish_result().
 */
PUBNUB_EXTERN enum pubnub_publish_res pubnub_parse_publish_result(char const *result);

/** Returns a string (in English) describing a Pubnub result enum value 
    @p e.
 */
PUBNUB_EXTERN char const* pubnub_res_2_string(enum pubnub_res e);

/** Returns a string literal describing enum value @p e
    Used for debugging.
 */
PUBNUB_EXTERN char const* pbpal_resolv_n_connect_res_2_string(enum pbpal_resolv_n_connect_result e);

#if PUBNUB_USE_SUBSCRIBE_V2
#include "pubnub_subscribe_v2_message.h"
/** Returns a string literal describing enum value @p type
    Used when looking at pubnub V2 message received by pubnub_subscribe_v2().
 */
PUBNUB_EXTERN char const* pubnub_msg_type_to_str(enum pubnub_message_type type);
#endif

/** Returns whether retrying a Pubnub transaction makes sense.  This
    is mostly interesting for publishing, but is useful in general. It
    is least useful for subscribing, because you will most probably
    subscribe again at some later point in time, even if you're not in
    a "subscribe loop".

    @par Basic usage
    @snippet pubnub_sync_publish_retry.c Publish retry

    @param e The Pubnub result returned by a C-core function, for which
    the user wants to find out if retrying makes sense

    @retval pbccFalse It doesn't benefit you to re-try the same
    transaction.

    @retval pbccTrue It's safe to retry, though there is no guarantee
    that it will help.

    @retval pbccNotSet Retry might help, but, it also can make things
    worse. For example, for a #PNR_TIMEOUT, it may very well be that
    the message was delivered to Pubnub, but, the response from Pubnub
    never reached us due to some networking issue, resulting in a
    timeout.  In that case, retrying would send the same message
    again, duplicating it.
 */
PUBNUB_EXTERN enum pubnub_tribool pubnub_should_retry(enum pubnub_res e);

/** Replace specified char in string.
 *
 * @param str Source in which char should be replaced.
 * @param find What should be replaced.
 * @param replace What should be used instead of searched needle.
 * @return Updated string.
 */
PUBNUB_EXTERN char* replace_char(char* str, char find, char replace);

#endif /* defined INC_PUBNUB_HELPER */
