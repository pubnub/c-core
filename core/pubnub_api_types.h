/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_API_TYPES
#define      INC_PUBNUB_API_TYPES


/** @file pubnub_api_types.h

    These are the definitions of types used by most functions of the
    Pubnub C-core API. Users of the SDK in general don't need to
    include it, as other headers of the API will include it for them.
 */

struct pubnub_;

/** A pubnub context. An opaque data structure that holds all the data
    needed for a context.
 */
typedef struct pubnub_ pubnub_t;


/** Result codes for Pubnub functions and transactions.  */
enum pubnub_res {
    /** Success. Transaction finished successfully. */
    PNR_OK,
    /** Pubnub host name resolution failed. We failed to get
        an IP address from the Pubnub host name ("origin").
        Most of the time, this comes down to a DNS error.
    */
    PNR_ADDR_RESOLUTION_FAILED,
    /** Connecting to Pubnub server failed. Most often,
        this means a network outage, but could be many things.
        If using SSL/TLS, it could be some of its errors.
    */
    PNR_CONNECT_FAILED,
    /** A time-out happened in the network. Mostly, this is because
        a network outage happened while being connected to the Pubnub
        server, but could be other things.
    */
    PNR_CONNECTION_TIMEOUT,
    /** Time-out before the request has completed. This is reported
        for a time-out detected by Pubnub client itself, not some
        reported by others (i.e. the TCP/IP stack). 
	*/
    PNR_TIMEOUT,
    /** Connection to Pubnub aborted (in most cases, a TCP reset was
        received) */
    PNR_ABORTED,
    /** Communication error (network or HTTP response format). */
    PNR_IO_ERROR,
    /** HTTP error. Call pubnub_last_http_code() to get the error
     * code. */
    PNR_HTTP_ERROR,
    /** Unexpected input in received JSON. */
    PNR_FORMAT_ERROR,
    /** Request cancelled by user. */
    PNR_CANCELLED,
    /** Transaction started. Await the outcome. */
    PNR_STARTED,
    /** Transaction (already) ongoing. Can't start a new transaction
        while the old one is in progress. */
    PNR_IN_PROGRESS,
    /** Receive buffer (from previous transaction) not read, new
        subscription not allowed.
    */
    PNR_RX_BUFF_NOT_EMPTY,
    /** The buffer is too small. Increase #PUBNUB_BUF_MAXLEN.
    */
    PNR_TX_BUFF_TOO_SMALL,
    /** Channel specification / name is invalid. 
     */
    PNR_INVALID_CHANNEL,
    /** Publish transaction failed - error returned from Pubnub. To
        see the reason describing the failure, call
        pubnub_last_publish_result().
     */
    PNR_PUBLISH_FAILED,
    /** A transaction related to channel registry failed - error
        returned from Pubnub. To see the reason describing the
        failure, get the value for key "message" from the response
        (which is a JSON object) and value for key "status" for the
        numeric code of the error.
     */
    PNR_CHANNEL_REGISTRY_ERROR,
    /** Reply is too big to fit in our reply buffer. This same error
        is reported if the reply buffer is statically or dynamically
        allocated.
    */
    PNR_REPLY_TOO_BIG,
    /** An internal error in processing */
    PNR_INTERNAL_ERROR,
    /** Encryption (and decryption) not supported */
    PNR_CRYPTO_NOT_SUPPORTED
};


/** Type of Pubnub operation/transaction */
enum pubnub_trans {
    /** No transaction at all */
    PBTT_NONE,
    /** Subscribe operation/transaction */
    PBTT_SUBSCRIBE,
    /** Publish operation/transaction */
    PBTT_PUBLISH,
    /** Leave (channel(s)) operation/transaction */
    PBTT_LEAVE,
    /** Time (get from Pubnub server) operation/transaction */
    PBTT_TIME,
    /** History V2 (get message history for the channel from Pubnub
     * server) operation/transaction */
    PBTT_HISTORY,
    /** Here-now (get UUIDs of currently present users in channel(s))
     * operation/transaction */
    PBTT_HERENOW,
    /** Global here-now (get UUIDs of currently present users in all
        channels) operation/transaction */
    PBTT_GLOBAL_HERENOW,
    /** Where-now (get channels in which an user (identified by UUID)
        is currently present) operation/transaction */
    PBTT_WHERENOW,
    /** Set state (for a user (identified by UUID) on channel(s))
     * operation/transaction */
    PBTT_SET_STATE,
    /** Get state (for a user (identified by UUID) on channel(s))
     * operation/transaction */
    PBTT_STATE_GET,
    /** Remove a channel group (from the channel-registry)
     * operation/transaction */
    PBTT_REMOVE_CHANNEL_GROUP,
    /** Remove a channel from a channel group (in the channel-registry)
     * operation/transaction */
    PBTT_REMOVE_CHANNEL_FROM_GROUP,
    /** Add a channel to a channel group (in the channel-registry)
     * operation/transaction */
    PBTT_ADD_CHANNEL_TO_GROUP,
    /** Get a list of all channels in a channel group (from the
     * channel-registry) operation/transaction */
    PBTT_LIST_CHANNEL_GROUP,
    /** Inform Pubnub that we're still working on @p channel and/or @p
        channel_group operation/transaction */
    PBTT_HEARTBEAT,
};


#endif /* !defined INC_PUBNUB_API_TYPES */
