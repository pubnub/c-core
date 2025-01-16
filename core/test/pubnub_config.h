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
#define PUBNUB_CTX_MAX 4

/** Maximum length of the HTTP buffer. This is a major component of
 * the memory size of the whole pubnub context, but it is also an
 * upper bound on URL-encoded form of published message, so if you
 * need to construct big messages, you may need to raise this.  */
#define PUBNUB_BUF_MAXLEN 256

/** Maximum length of the HTTP reply. The other major component of the
 * memory size of the PubNub context, beside #PUBNUB_BUF_MAXLEN.
 * Replies of API calls longer than this will be discarded and
 * instead, #PNR_FORMAT_ERROR will be reported. Specifically, this may
 * cause lost messages returned by subscribe if too many too large
 * messages got queued on the Pubnub server. */
#define PUBNUB_REPLY_MAXLEN 1024

/** If defined, the PubNub implementation will not try to catch-up on
 * messages it could miss while subscribe failed with an IO error or
 * such.  Use this if missing some messages is not a problem.  
 *
 * @note messages may sometimes still be lost due to potential @ref
 * PUBNUB_REPLY_MAXLEN overrun issue */
#define PUBNUB_MISSMSG_OK 1

/** This is the URL of the Pubnub server. Change only for testing
    purposes.
*/
#define PUBNUB_ORIGIN  "pubsub.pubnub.com"

/** The maximum length (in characters) of the host name of the proxy
    that will be saved in the Pubnub context.
*/
#define PUBNUB_MAX_PROXY_HOSTNAME_LENGTH 63

#define PUBNUB_HAVE_SHA1 0

#if !defined PUBNUB_USE_MDNS
/** If `1`, the MDNS module will be used to handle the DNS
        resolving. If `0` the "resolv" module will be used.
        This is a temporary solution, it is expected that ConTiki
        will unify those two modules.
*/
#define PUBNUB_USE_MDNS 1
#endif

#if defined(PUBNUB_CALLBACK_API)
#if !defined(PUBNUB_USE_IPV6)
/** If true (!=0), enable support for Ipv6 network addresses */
#define PUBNUB_USE_IPV6 1
#endif

#if !defined(PUBNUB_USE_MULTIPLE_ADDRESSES)
/** If true (!=0), enable support for trying different addresses
    from dns response when connecting to the server.
 */
#define PUBNUB_USE_MULTIPLE_ADDRESSES 1
#endif

#if PUBNUB_USE_MULTIPLE_ADDRESSES
#define PUBNUB_MAX_IPV4_ADDRESSES 2
#if PUBNUB_USE_IPV6
#define PUBNUB_MAX_IPV6_ADDRESSES 2
#endif
#endif /* PUBNUB_USE_MULTIPLE_ADDRESSES */

#if !defined(PUBNUB_SET_DNS_SERVERS)
/** If true (!=0), enable support for setting DNS servers */
#define PUBNUB_SET_DNS_SERVERS 1
#endif

#if PUBNUB_SET_DNS_SERVERS
/** If true (!=0), enable support for switching between DNS servers */
#define PUBNUB_CHANGE_DNS_SERVERS 1
#endif

#define PUBNUB_DEFAULT_DNS_SERVER "8.8.8.8"

/** Maximum number of consecutive retries when sending DNS query in a single transaction */
#define PUBNUB_MAX_DNS_QUERIES 3
#if PUBNUB_CHANGE_DNS_SERVERS
/** Maximum number of DNS servers list rotation in a single transaction */
#define PUBNUB_MAX_DNS_ROTATION 3
#endif /* PUBNUB_CHANGE_DNS_SERVERS */
#endif /* defined(PUBNUB_CALLBACK_API) */

#if !defined(PUBNUB_RECEIVE_GZIP_RESPONSE)
// If true (!=0), enables support for compressed content data
#define PUBNUB_RECEIVE_GZIP_RESPONSE 1
#endif

#if !defined(PUBNUB_USE_LOG_CALLBACK)
#define PUBNUB_USE_LOG_CALLBACK 0
#endif

#define PUBNUB_DEFAULT_TRANSACTION_TIMER    310000

#define PUBNUB_MIN_TRANSACTION_TIMER 200

/** Duration of the 'wait_connect_TCP_socket' timeout set during context
    initialization, in milliseconds. Can be changed later by the user.
    */
#define PUBNUB_DEFAULT_WAIT_CONNECT_TIMER    10000

/** Mininmal duration of the 'wait_connect_TCP_socket' timer, in milliseconds.
 *  You can't set less than this.
 */
#define PUBNUB_MIN_WAIT_CONNECT_TIMER 5000

#if !defined(PUBNUB_USE_ADVANCED_HISTORY)
/** If true (!=0) will enable using the advanced history API, which
    provides more data about (unread) messages. */
#define PUBNUB_USE_ADVANCED_HISTORY 1
#endif

#if !defined(PUBNUB_USE_FETCH_HISTORY)
/** If true (!=0) will enable using the advanced history API, which
    provides more data about single/multip channel messages. */
#define PUBNUB_USE_FETCH_HISTORY 1
#endif

#define PUBNUB_MAX_URL_PARAMS 10

#ifndef PUBNUB_RAND_INIT_VECTOR
#define PUBNUB_RAND_INIT_VECTOR 1
#endif
#endif /* !defined INC_PUBNUB_CONFIG */
