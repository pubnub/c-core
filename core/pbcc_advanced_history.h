/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if PUBNUB_USE_ADVANCED_HISTORY
#if !defined INC_PBCC_ADVANCED_HISTORY
#define INC_PBCC_ADVANCED_HISTORY

/** @file pbcc_advanced_history.h

    This has the functions for formating and parsing the
    requests and responses for 'advanced history' transactions
*/

typedef struct pubnub_char_mem_block pubnub_chamebl_t;

struct pubnub_chan_msg_count;

struct pbcc_context;


/** Parses server response on 'message_counts' transaction request and prepares
    msg offset for reading the content of json object for 'channels' key containing
    '"channel_name":nessage_count' pairs.
    @retval PNR_OK parsing successful
    @retval PNR_FORMAT_ERROR something's wrong with message format
    @retval PNR_ERROR_ON_SERVER server reported an error
 */
enum pubnub_res pbcc_parse_message_counts_response(struct pbcc_context* p);

/** Extracts 'error_message' attribute value from the transaction response on the
    pbcc_context @p p into @p o_msg.
    Can be called for any response, if it is regular json object, in case
    server reported an error 
    @retval 0 error message successfully picked up
    @retval -1 on error
 */
int pbcc_get_error_message(struct pbcc_context* p, pubnub_chamebl_t* o_msg);

/** If successful returns number of members(key:value pairs) of JSON object
    'channels', or -1 on error
 */
int pbcc_get_chan_msg_counts_size(struct pbcc_context* p);

/** On input, @p io_count is the number of allocated "counters per channel"(array
    dimension of @p chan_msg_counters). On output(@p io_count), number of counters per
    channel in the answer. If there are more in the answer than there are in the allocated
    array("can't fit all"), wan't be considered an error.
    @retval 0 on success
    @retval -1 on error
 */
int pbcc_get_chan_msg_counts(struct pbcc_context* p, 
                             size_t* io_count, 
                             struct pubnub_chan_msg_count* chan_msg_counters);

/** @p io_count is the array allocated by the user that has the same number of elements as
    the corresponding @p channel(s string) has channel names. The array is filled upon
    successful retrieval of message counters from the response.
    If some of the channels are not found in the answer, corresponding @p o_count elemets
    have negative values.
    
    @retval 0 on success
    @retval -1 on error
 */
int pbcc_get_message_counts(struct pbcc_context* p, char const* channel, int* o_count);

/** Prepares the 'message_counts' operation (transaction), mostly by
    formatting the URI of the HTTP request.
 */
enum pubnub_res pbcc_message_counts_prep(
                                         enum pubnub_trans pt,
                                         struct pbcc_context* p,
                                         char const*          channel,
                                         char const*          timetoken,
                                         char const*          channel_timetokens);

/**
 * @brief Prepare `delete messages` operation (transaction), mostly by
 *        formatting the URI of the HTTP request.
 *
 * @param pb      PubNub context which provides resources to send request.
 * @param channel Channel from which messages should be deleted.
 * @param start   Timetoken delimiting the start of time slice (exclusive) to
 *                delete messages from.
 * @param end     Timetoken delimiting the end of time slice (inclusive) to
 *                delete messages to.
 * @return Results of `delete messages` transaction call.
 */
enum pubnub_res pbcc_delete_messages_prep(struct pbcc_context* pb,
                                          char const*          channel,
                                          char const*          start,
                                          char const*          end);

/**
 * @brief Get `delete messages` service response.
 *
 * @param pb PubNub context which has been used to delete channel messages.
 * @return `pubnub_chamebl_t` with pointer to string with response.
 */
pubnub_chamebl_t pbcc_get_delete_messages_response(struct pbcc_context* pb);



/**
 * @brief Parses the string received as a response for a history v3
 *        operation (transaction).
 *
 * @param pb PubNub context to parse delete channel messages response.
 * @return PNR_OK: OK, !PNR_OK: error (invalid response)
*/
enum pubnub_res pbcc_parse_delete_messages_response(struct pbcc_context* pb);

#endif /* INC_PBCC_ADVANCED_HISTORY */
#endif /* PUBNUB_USE_ADVANCED_HISTORY */

