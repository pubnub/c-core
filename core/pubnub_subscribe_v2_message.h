/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#if PUBNUB_USE_SUBSCRIBE_V2

#if !defined INC_PUBNUB_SUBSCRIBE_V2_MESSAGE
#define      INC_PUBNUB_SUBSCRIBE_V2_MESSAGE

#if !PUBNUB_USE_SUBSCRIBE_V2
#error To use the subscribe V2 API you must define PUBNUB_USE_SUBSCRIBE_V2=1
#endif

#include "pubnub_memory_block.h"

/* subscribe_v2 message types */
enum pubnub_message_type {
    /* Indicates that message was received as a signal */ 
    pbsbSignal,
    /* Indicates that message was published */ 
    pbsbPublished,
    /* Indicates action on published message */
    pbsbAction,
    /* Message about Objects */
    pbsbObjects,
    /* Message about Files */
    pbsbFiles,
};

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
    /** Indicates the message type: a signal, published, or something else */ 
    enum pubnub_message_type message_type;
    /** The message information about publisher */
    struct pubnub_char_mem_block publisher;
    /** User-provided message type. */
    struct pubnub_char_mem_block custom_message_type;
};


#endif /* INC_PUBNUB_SUBSCRIBE_V2_MESSAGE */

#endif /* PUBNUB_USE_SUBSCRIBE_V2 */

