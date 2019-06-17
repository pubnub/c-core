/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_CONFIG
#define      INC_PUBNUB_CONFIG


/* -- Next few definitions can be tweaked by the user, but with care -- */

/** Maximum length of the HTTP buffer. This is a major component of
 * the memory size of the whole pubnub context, but it is also an
 * upper bound on URL-encoded form of published message, so if you
 * need to construct big messages, you may need to raise this.  */
#define PUBNUB_BUF_MAXLEN 1024

/** Set to 0 to use a static buffer and then set its size via
    #PUBNUB_REPLY_MAXLEN.  Set to anything !=0 to use a dynamic
    buffer, that is, dynamically try to allocate as much memory as
    needed for the buffer.
 */
#define PUBNUB_DYNAMIC_REPLY_BUFFER 1

#if !PUBNUB_DYNAMIC_REPLY_BUFFER

/** Maximum length of the HTTP reply when using a static buffer. The
 * other major component of the memory size of the PubNub context,
 * beside #PUBNUB_BUF_MAXLEN.  Replies of API calls longer than this
 * will be discarded and an error will be reported. Specifically, this
 * may cause lost messages returned by subscribe if too many too large
 * messages got queued on the Pubnub server. */
#define PUBNUB_REPLY_MAXLEN 2048

#endif

/** This is the URL of the Pubnub server. Change only for testing
    purposes.
*/
#define PUBNUB_ORIGIN  "pubsub.pubnub.com"

#define PUBNUB_HAVE_SHA1 0

/** The maximum channel name length */
#define PUBNUB_MAX_CHANNEL_NAME_LENGTH 92

#if !defined(PUBNUB_USE_ADVANCED_HISTORY)
/** If true (!=0) will enable using the advanced history API, which
    provides more data about (unread) messages. */
#define PUBNUB_USE_ADVANCED_HISTORY 1
#endif


/** Mininmal duration of the transaction timer, in milliseconds. You
 * can't set less than this.
 */
#define PUBNUB_MIN_TRANSACTION_TIMER 10000


#endif /* !defined INC_PUBNUB_CONFIG */
