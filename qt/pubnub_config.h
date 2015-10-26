/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_CONFIG
#define      INC_PUBNUB_CONFIG


/* -- Next few definitions can be tweaked by the user, but with care -- */

/** Maximum number of PubNub contexts. It is used only if the
 * contexts are statically allocated.
 * A context is used to publish messages or subscribe to (get) them.
 *
 * Doesn't make much sense to have less than 1. :)
 * OTOH, don't put too many, as each context takes (for our purposes)
 * a significant amount of memory - app. 128 + @ref PUBNUB_BUF_MAXLEN +
 * @ref PUBNUB_REPLY_MAXLEN bytes.
 *
 * A typical configuration may consist of a single pubnub context for
 * channel subscription and another pubnub context that will periodically
 * publish messages about device status (with timeout lower than message
 * generation frequency).
 *
 * Another typical setup may have a single subscription context and
 * maintain a pool of contexts for each publish call triggered by an
 * external event (e.g. a button push).
 *
 * Of course, there is nothing wrong with having just one context, but
 * you can't publish and subscribe at the same time on the same context.
 * This isn't as bad as it sounds, but may be a source of headaches
 * (lost messages, etc).
 */
#define PUBNUB_CTX_MAX 2

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

#define PUBNUB_HAVE_MD5 0
#define PUBNUB_HAVE_SHA1 0


#endif /* !defined INC_PUBNUB_CONFIG */
