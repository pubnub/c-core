/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_API_TYPES
#define      INC_PUBNUB_API_TYPES


/** @file pubnub_api_types.h

    These are the definitions of types used by most functions of the
    Pubnub C-core API. Users of the SDK in general don't need to
    include it, as other headers of the API will include it for them.
  */

#include "pubnub_config.h"

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
    /** Time-out before the TCP connection was esatblished. This is reported
        for a time-out detected by Pubnub client itself, not some
        reported by others.
	*/
    PNR_WAIT_CONNECT_TIMEOUT,
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
    /** Subscribe Timetoken not in expected format */
    PNR_SUB_TT_FORMAT_ERROR,
    /** No Timetoken in the subscribe response */
    PNR_SUB_NO_TT_ERROR,
    /** No Region in the subscribe response */
    PNR_SUB_NO_REG_ERROR,
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
    /** Ran out of dynamic memory */
    PNR_OUT_OF_MEMORY,
    /** Encryption (and decryption) not supported */
    PNR_CRYPTO_NOT_SUPPORTED,
    /** Bad compression format or compressed data corrupted */
    PNR_BAD_COMPRESSION_FORMAT,
    /** Invalid input parameters passed to a given function */
    PNR_INVALID_PARAMETERS,
    /** Server reports an error in the response */
    PNR_ERROR_ON_SERVER,
    /** Proxy authentication failed */
    PNR_AUTHENTICATION_FAILED,
    /** Objects API invalid parameter */
    PNR_OBJECTS_API_INVALID_PARAM,
    /** Objects API transaction reported an error */
    PNR_OBJECTS_API_ERROR,
    /** Actions API pbcc_get_actions_more() did not find another hyperlink to the rest */
    PNR_GOT_ALL_ACTIONS,
    /** Actions API transaction reported an error */
    PNR_ACTIONS_API_ERROR,
    /** Grant Token API transaction reported an error */
    PNR_GRANT_TOKEN_API_ERROR,
    /** Fetch History API transaction reported an error */
    PNR_FETCH_HISTORY_ERROR,
    /** Revoke Token API transaction reported an error */
    PNR_REVOKE_TOKEN_API_ERROR,
    /** Access/Permission denied */
    PNR_ACCESS_DENIED,
    /** No Channels in the ChannelGroup */
    PNR_GROUP_EMPTY
};

/** 'pubnub_cancel()' return value */
enum pubnub_cancel_res {
    /** 'cancel' finished */
    PN_CANCEL_FINISHED,
    /** 'cancel' started */
    PN_CANCEL_STARTED
};

/** Type of Pubnub operation/transaction */
enum pubnub_trans {
    /** No transaction at all */
    PBTT_NONE,
    /** Subscribe operation/transaction */
    PBTT_SUBSCRIBE,
    /** Publish operation/transaction */
    PBTT_PUBLISH,
    /** Signal operation/transaction */
    PBTT_SIGNAL,
    /** Leave (channel(s)) operation/transaction */
    PBTT_LEAVE,
    /** Time (get from Pubnub server) operation/transaction */
    PBTT_TIME,
    /** History V2 (get message history for the channel from Pubnub
     * server) operation/transaction */
    PBTT_HISTORY,
    /** Here-now (get USER_IDs of currently present users in channel(s))
     * operation/transaction */
    PBTT_HERENOW,
    /** Global here-now (get USER_IDs of currently present users in all
        channels) operation/transaction */
    PBTT_GLOBAL_HERENOW,
    /** Where-now (get channels in which an user (identified by USER_ID)
        is currently present) operation/transaction */
    PBTT_WHERENOW,
    /** Set state (for a user (identified by USER_ID) on channel(s))
     * operation/transaction */
    PBTT_SET_STATE,
    /** Get state (for a user (identified by USER_ID) on channel(s))
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
    /** Inform Pubnub that we're still working on channel and/or
        channel_group operation/transaction */
    PBTT_HEARTBEAT,
#if PUBNUB_USE_SUBSCRIBE_V2
    /** Subscribe V2 operation/transaction */
    PBTT_SUBSCRIBE_V2,
#endif
#if PUBNUB_USE_ADVANCED_HISTORY
    /** Message counts(get counters of unread messages for a user, identified by USER_ID,
        for the list of channels specified) starting from given timetoken, or(exclusive or)
        list of timetokens per channel.
        If neither timetoken, nor channel_timetokens are specified, gets entire message
        history counts for channels listed.
      */
    PBTT_MESSAGE_COUNTS,
    /**
     * @brief History V3 (delete messages) operation/transaction.
     *
     * @note All `channel` messages will be removed If neither `start`, nor
     *       `end` has been passed.
     */
    PBTT_DELETE_MESSAGES,
#endif
#if PUBNUB_USE_OBJECTS_API
    /** Objects API transaction Returns a paginated list of users associated with the
        subscription key.
      */
    PBTT_GETALL_UUIDMETADATA,
    /** Objects API transaction Creates a user with the attributes specified. */
    PBTT_SET_UUIDMETADATA,
    /** Objects API transaction Returns the user object specified with metadata_uuid */
    PBTT_GET_UUIDMETADATA,
    /** Objects API transaction Deletes user data( on pubnub server) specified with metadata_uuid */
    PBTT_DELETE_UUIDMETADATA,
    /** Objects API transaction Returns a paginated list of spaces associated with the
        subscription key.
      */
    PBTT_GETALL_CHANNELMETADATA,
    /** Objects API transaction Creates a space with the attributes specified. */
    PBTT_SET_CHANNELMETADATA,
    /** Objects API transaction Returns the space object specified with space_id */
    PBTT_GET_CHANNELMETADATA,
    /** Objects API transaction Deletes space data( on pubnub server) specified with space_id */
    PBTT_REMOVE_CHANNELMETADATA,
    /** Objects API transaction Returns the space memberships of the user specified with user_id.
      */
    PBTT_GET_MEMBERSHIPS,
    /** Objects API transaction Updates the users space memberships specified with user_id.
      */
    PBTT_SET_MEMBERSHIPS,
    /** Objects API transaction Removes the users space memberships specified with user_id.
      */
    PBTT_REMOVE_MEMBERSHIPS,
    /** Objects API transaction Returns all users in the space specified by space_id.
      */
    PBTT_GET_MEMBERS,
    /** Objects API transaction Adds the list of members of the space specified with space_id.
      */
    PBTT_ADD_MEMBERS,
    /** Objects API transaction Updates the list of members of the space specified with space_id.
      */
    PBTT_SET_MEMBERS,
    /** Objects API transaction Removes the list of members of the space specified with space_id.
      */
    PBTT_REMOVE_MEMBERS,

#endif /* PUBNUB_USE_OBJECTS_API */
#if PUBNUB_USE_ACTIONS_API
    /** Actions API transaction Adds the action to the message.
      */
    PBTT_ADD_ACTION,
    /** Actions API transaction Removes the action from the message.
      */
    PBTT_REMOVE_ACTION,
    /** Actions API transaction Gets the actions received on a given channel.
      */
    PBTT_GET_ACTIONS,
    /** Actions API transaction Gets the message history with actions on them.
      */
    PBTT_HISTORY_WITH_ACTIONS,
#endif /* PUBNUB_USE_ACTIONS_API */
#if PUBNUB_USE_GRANT_TOKEN_API
    /** PAMv3 Grant API transaction sets the permissions for resources and patterns.
      */
    PBTT_GRANT_TOKEN,
#endif /* PUBNUB_USE_GRANT_TOKEN_API */
#if PUBNUB_USE_FETCH_HISTORY
    /** History V3 (get fetch history for the channel(s) from Pubnub
     * server) operation/transaction */
    PBTT_FETCH_HISTORY,
#endif
#if PUBNUB_USE_REVOKE_TOKEN_API
    /** PAMv3 Revoke API transaction revokes the PAMv3 token .
      */
    PBTT_REVOKE_TOKEN,
#endif /* PUBNUB_USE_REVOKE_TOKEN_API */
    /** Count the number of transaction types */
    PBTT_MAX
};

/** The 3-state bool. For Electrical Enginners among you, this could
    be a digital line utilizing high impedance ("High Z"). For
    mathematicians, a ternary logic value. For physicts, a quantum
    binary value (with super-position being the "third" state). 
    
    For the rest of us, it has `true`, `false` and the third `not set`
    or `indeterminate` state.
*/
enum pubnub_tribool {
    pbccFalse,
    pbccTrue,
    pbccNotSet
};

enum pubnub_method {
    pubnubSendViaGET,
    pubnubSendViaPOST,
    pubnubUsePATCH,
    pubnubSendViaPOSTwithGZIP,
    pubnubUsePATCHwithGZIP,
    pubnubUseDELETE
};

/** Enum that describes an error when checking parameters passed to a function */
enum pubnub_parameter_error {
    /** All parameters checked are valid */
    pnarg_PARAMS_OK,
    /** Invalid channel name */
    pnarg_INVALID_CHANNEL,
    /** Invalid timetoken parameter */
    pnarg_INVALID_TIMETOKEN,
    /** Number of channels and number of timetokens from corresponding lists are
        not equal(when they should be).
      */
    pnarg_CHANNEL_TIMETOKEN_COUNT_MISMATCH
};

#endif /* !defined INC_PUBNUB_API_TYPES */
