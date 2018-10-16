/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_SUBSCRIBE_V2
#define INC_PUBNUB_SUBSCRIBE_V2

#include "pubnub_config.h"

#include "pubnub_api_types.h"
#include "pubnub_memory_block.h"


#if !PUBNUB_USE_SUBSCRIBE_V2
#error To use the subscribe V2 API you must define PUBNUB_USE_SUBSCRIBE_V2=1
#endif


/** @file pubnub_subscribe_v2.h

    This API is for the support of subscribe V2, which enables
    several features not supported in the "plain old" subscribe V1:

    - filter expressions (on message metadata)
    - get timestamp of each message
    - get various other info about a message

    It's more complex than V1, so, unless you need these extra
    features, using V1 will save you time and space.
*/

/** Options for V2 subscribe. */
struct pubnub_subscribe_v2_options {
    /** Channel group (a comma-delimited list of channel group names).
        If NULL, will not be used */
    char const* channel_group;
    /** The channel activity timeout, in seconds. If below the minumum
        value supported by Pubnub, the minimum value will be used
        (instead).
     */
    unsigned heartbeat;
    /** The filter expression to apply. It's a predicate to apply to
        the metadata of a message. If it returns `true`, message will
        be received, otherwise, it will be skipped (as if not
        published). Syntax is not trivial, but can be described as
        mostly Javascript on the metadata (which is JSON, thus,
        "integrates well" wtih Javascript). For example, if your
        metadata is: `{"zec":3}`, then this filter _would_ match it:
        `zec==3`, while `zec==4` would _not_.

        If message doesn't have metadata, this will be ignored.

        If NULL, will not be used.
     */
    char const* filter_expr;
};

/** This returns the default options for subscribe V2 transactions.
    Will set `channel_group = NULL`, `heartbeat` to default heartbeat
    value and `filter_expr = NULL`.
 */
struct pubnub_subscribe_v2_options pubnub_subscribe_v2_defopts(void);

/** The V2 subscribe. To get messages for subscribe V2, use
    pubnub_get_v2() - keep in mind that it can provide you with
    channel and channel group info.

    Basic usage:

        struct pubnub_subscribe_v2_options opt = pubnub_subscribe_v2_defopts();
        opt.heartbeat = 412;
        pbresult = pubnub_subscribe_v2(pn, "my_channel", opt);

    @param p The Pubnub context. Can't be NULL.
    @param channel The string with the channel name (or
    comma-delimited list of channel names) to subscribe for.
    @param opt Subscribe V2 options
    @return #PNR_STARTED on success, an error otherwise
*/
enum pubnub_res pubnub_subscribe_v2(pubnub_t*                          p,
                                    const char*                        channel,
                                    struct pubnub_subscribe_v2_options opts);


/** Pubnub V2 message has lots of data and here's how we express them
    for the pubnub_get_v2().

    The "string fields" are expressed as "Pascal strings", that is, a
    pointer with string length, and _don't_ include a NUL character.
    Also, these pointers are actually pointing into the full received
    message, so, their lifetime is tied to the message lifetime and
    any subsequent transaction on the same context will invalidate
    them.

*/
struct pubnub_v2_message {
    /** The time token of the message - when it was published. */
    struct pubnub_char_mem_block tt;
    /** Region of the message - not interesting in most cases */
    int region;
    /** Message flags */
    int flags;
    /** Channel that message was published to */
    struct pubnub_char_mem_block channel;
    /** Subscription match or the channel group */
    struct pubnub_char_mem_block match_or_group;
    /** The message itself */
    struct pubnub_char_mem_block payload;
    /** The message metadata, as published */
    struct pubnub_char_mem_block metadata;
};

/** Parse and return the next V2 message, if any.

    Do keep in mind that you can use pubnub_get() to get the full
    message in JSON, but, then you'll have to parse it yourself, and
    pubnub_channel() and pubnub_channel_group() would not work.

    If there are no more messages, this will return a struct "memset
    to zero". Since message must have the payload, the fail safe
    way of checking is:

        v2msg = pubnub_get_v2(pbp);
        if (NULL == v2msg.payload.ptr) {
            puts("No more messages");
        }
 */
struct pubnub_v2_message pubnub_get_v2(pubnub_t* pbp);




#endif /* !defined INC_PUBNUB_SUBSCRIBE_V2 */
