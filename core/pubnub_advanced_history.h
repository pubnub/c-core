/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#if PUBNUB_USE_ADVANCED_HISTORY

#if !defined INC_PUBNUB_ADVANCED_HISTORY
#define INC_PUBNUB_ADVANCED_HISTORY

#include "lib/pb_extern.h"
#include "pbcc_advanced_history.h"

/** Structure containing channel name as char memory block and field with
    message count for messages received on the channel since given point in time
    (defined by parameters passed to the function).
    Used to store information retrieved by 'advanced history' message_counts operation
 */
struct pubnub_chan_msg_count {
    /* Channel name as char memory block */
    pubnub_chamebl_t channel;
    /* Message count for the corresponding channel since given point in time */
    size_t message_count;
};

// Delete messages endpoint configuration definition.
struct pubnub_delete_messages_options {
    /**
     * @brief Timetoken delimiting the start of time slice (exclusive) to delete
     *        messages from.
     */
    char const* start;
    /**
     * @brief Timetoken delimiting the end of time slice (inclusive) to delete
     *        messages to.
     */
    char const* end;
};


/** If successful returns number of members(key:value pairs) of JSON object
    'channels', or -1 on error(transaction still in progress, or so)
 */
PUBNUB_EXTERN int pubnub_get_chan_msg_counts_size(pubnub_t* pb);

/** Starts the transaction 'pubnub_message_counts' on the context @p pb for the
    list of channels @p channel for unread messages counts starting from @p timeoken
    which can be a single timetoken, or list of timetokens(corresponding to the
    'channel' list).
    @retval PNR_STARTED request has been sent, but transaction is still in progress
    @retval PNR_IN_PROGRESS can't start transaction because previous one is still
                            in progress(hasn't finished yet)
    @retval otherwise a result with the same meaning as for any other transaction
 */
PUBNUB_EXTERN enum pubnub_res pubnub_message_counts(pubnub_t*   pb,
                                      char const* channel, 
                                      char const* timetoken);

/** On input, @p io_count is the number of allocated "counters per channel"(array
    dimension of @p chan_msg_counters). On output(@p io_count), number of counters per
    channel in the answer. If there are more in the answer than there are in the allocated
    array("can't fit all"), wan't be considered an error.
    @retval 0 on success
    @retval -1 on error(transaction in progress, or format error)
 */
PUBNUB_EXTERN int pubnub_get_chan_msg_counts(pubnub_t* pb, 
                               size_t* io_count, 
                               struct pubnub_chan_msg_count* chan_msg_counters);

/** Array dimension for @p o_count is the number of channels from channel list
    @p channel and it('o_count' array) has to be provided by the user.
    Message counts order in `o_count` is corresponding to channel order in `channel`
    list respectively, even if the answer itself is different. If the requested
    channel(from 'o_count' array) is not found in the answer, the message counter
    in the respective 'o_count' array member has negative value. 
    If there is a channel name in the answer, not to be found in requested
    `channel` list, that won't be considered an error.
    @retval 0 on success
    @retval -1 on error(transaction in progress, or format error)
  */
PUBNUB_EXTERN int pubnub_get_message_counts(pubnub_t* pb, char const*channel, int* o_count);

/**
 * @brief Delete message transaction default options.
 *
 * @note It's best to always call `pubnub_delete_messages_defopts()` to
 *       initialize `pubnub_delete_messages_options`. since it has serveral
 *       parameters which maybe extended in the future.
 *
 * @return Default options for delete message transaction.
 */
PUBNUB_EXTERN struct pubnub_delete_messages_options pubnub_delete_messages_defopts(void);

/**
 * @brief Remove messages from the `channel`.
 *
 * Start `delete_messages` transaction to permanently remove messages from the `channel` storage.
 *
 * @param pb      PubNub context which should be used to perform
 *                `delete messages` transaction.
 * @param channel Channel from which messages should be deleted.
 * @param options Additional `delete messages` transaction configuration object.
 * @return Results of `delete messages` transaction call.
 */
PUBNUB_EXTERN enum pubnub_res
pubnub_delete_messages(pubnub_t*                             pb,
                       char const*                           channel,
                       struct pubnub_delete_messages_options options);

/**
 * @brief Get `delete messages` service response.
 *
 * @param pb PubNub context which has been used to delete channel messages.
 * @return `pubnub_chamebl_t` with pointer to string with response.
 */
PUBNUB_EXTERN pubnub_chamebl_t pubnub_get_delete_messages_response(pubnub_t* pb);

#endif /* !defined INC_PUBNUB_ADVANCED_HISTORY */

#endif /* PUBNUB_USE_ADVANCED_HISTORY */

