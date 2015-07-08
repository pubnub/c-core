/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_RES
#define      INC_PUBNUB_RES


/** Result codes for Pubnub functions and transactions.  */
enum pubnub_res {
    /** Success. Transaction finished successfully. */
    PNR_OK,
    /** Time out before the request has completed. */
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
    /** The buffer is to small. Increase #PUBNUB_BUF_MAXLEN.
    */
    PNR_TX_BUFF_TOO_SMALL,
    /** Channel specification / name is invalid. 
     */
    PNR_INVALID_CHANNEL
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
    /** History (get message history for the channel from Pubnub
     * server) operation/transaction */
    PBTT_HISTORY,
    /** History V2 (get message history for the channel from Pubnub
     * server) operation/transaction */
    PBTT_HISTORYV2,
    /** Here-now (get UUIDs of currently present users in channel(s))
     * operation/transaction */
    PBTT_HERENOW,
    /** Global here-now (get UUIDs of currently present users in all
        channels) operation/transaction */
    PBTT_GLOBAL_HERENOW,
    /** Where-now (get channels in which an user (identified by UUID)
        is cuurently present) operation/transaction */
    PBTT_WHERENOW,
    /** Set state (for a user (identified by UUID) on channel(s))
     * operation/transaction */
    PBTT_SET_STATE,
    /** Get state (for a user (identified by UUID) on channel(s))
     * operation/transaction */
    PBTT_STATE_GET,
};


#endif /* !defined INC_PUBNUB_RES */
