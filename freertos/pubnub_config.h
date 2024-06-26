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

/** Set to 0 to disable changing the origin from the default
    #PUBNUB_ORIGIN.  Set to anything != 0 to enable changing the
    origin (per context).
 */
#define PUBNUB_ORIGIN_SETTABLE 1

/** Duration of the transaction timeout set during context initialization,
    in milliseconds. Timeout duration in the context can be changed by the 
    user after initialization.
    */
#define PUBNUB_DEFAULT_TRANSACTION_TIMER    310000

/** Mininmal duration of the transaction timer, in milliseconds. You
 * can't set less than this.
 */
#define PUBNUB_MIN_TRANSACTION_TIMER 10000

#define PUBNUB_HAVE_SHA1 0

#if !defined(PUBNUB_PROXY_API)
/** If true (!=0), enable support for (HTTP/S) proxy */
#define PUBNUB_PROXY_API 1
#endif

#if PUBNUB_USE_GZIP_COMPRESSION
/* Maximum compressed message length allowed. Could be shortened by the user */
#define PUBNUB_COMPRESSED_MAXLEN 1024
#endif

/** The maximum length (in characters) of the host name of the proxy
    that will be saved in the Pubnub context.
*/
#define PUBNUB_MAX_PROXY_HOSTNAME_LENGTH 63

/** If true (!=0), enable support for message encryption/decryption */
#define PUBNUB_CRYPTO_API 0


#if !defined(PUBNUB_ONLY_PUBSUB)
/** If true (!=0), will enable only publish and subscribe. All
    other transactions will fail.

    For use in embedded systems and, in general, when you know
    you won't be needing anything but publish and subscribe,
    to reduce the memory footprint.
*/
#define PUBNUB_ONLY_PUBSUB_API 0
#endif

#if !defined(PUBNUB_USE_SUBSCRIBE_V2)
/** If true (!=0) will enable using the subscribe v2 API, which
    provides filter expressions and more data about messages. */
#define PUBNUB_USE_SUBSCRIBE_V2 1
#endif

#if !defined(PUBNUB_USE_ADVANCED_HISTORY)
/** If true (!=0) will enable using the advanced history API, which
    provides more data about (unread) messages. */
#define PUBNUB_USE_ADVANCED_HISTORY 1
#endif

#if !defined(PUBNUB_USE_FETCH_HISTORY)
/** If true (!=0) will enable using the fetch history API, which
    provides more data about single/multip channel messages. */
#define PUBNUB_USE_FETCH_HISTORY 1
#endif

#if !defined(PUBNUB_USE_OBJECTS_API)
/** If true (!=0) will enable using the objects API, which is a
    collection of Rest API features that enables "CRUD"(Create, Read, Update and Delete)
    on two new pubnub objects: User and Space, as well as manipulating connections
    between them. */
#define PUBNUB_USE_OBJECTS_API 1
#endif

#if !defined(PUBNUB_USE_ACTIONS_API)
/** If true (!=0) will enable using the Actions API, which is a collection
    of Rest API features that enables adding on, reading and removing actions
    from published messages */
#define PUBNUB_USE_ACTIONS_API 1
#endif

#if !defined(PUBNUB_USE_GRANT_TOKEN_API)
/** If true (!=0) will enable using the Grant Token API */
#define PUBNUB_USE_GRANT_TOKEN_API 1
#endif

#if !defined(PUBNUB_USE_REVOKE_TOKEN_API)
/** If true (!=0) will enable using the Revoke Token API */
#define PUBNUB_USE_REVOKE_TOKEN_API 1
#endif

#if !defined(PUBNUB_USE_AUTO_HEARTBEAT)
/** If true (!=0) will enable using the Auto Heartbeat Thumps(beats), which is a feature
    that enables keeping presence of the given uuids on channels and channel groups during
    longer periods without subscription.
    This gives more freedom to the user while coding whom, othrewise, should take care of
    these things all by himself using pubnub_heartbeat() transaction */
#define PUBNUB_USE_AUTO_HEARTBEAT 1
#endif

#ifndef PUBNUB_USE_SSL
/** If true (!=0), will enable SSL/TLS support.  If false (==0), will
    disable SSL/TLS support.  If not defined, will enable SSL/TLS
    support. */
#define PUBNUB_USE_SSL 1
#endif


#endif /* !defined INC_PUBNUB_CONFIG */
