/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if PUBNUB_USE_FETCH_HISTORY

#if !defined INC_PUBNUB_FETCH_HISTORY
#define INC_PUBNUB_FETCH_HISTORY

#include "pbcc_fetch_history.h"
#include "lib/pb_extern.h"

/** Options for fetch history. */
struct pubnub_fetch_history_options {
    /** The maximum number of messages to return per channel if multiple channels provided. 
     * Has to be between 1 and 25 messages. Default is 25.
     * If single channel is provided, maximum 100 messages. Default is 100.
     */
    int max_per_channel;
    /** Direction of time traversal. Default is false, which means
     * timeline is traversed newest to oldest. */
    bool reverse;
    /** If provided (not NULL), lets you select a "start date", in
     * Timetoken format. If not provided, it will default to current
     * time. Page through results by providing a start OR end time
     * token. Retrieve a slice of the time line by providing both a
     * start AND end time token. Start is "exclusive" – that is, the
     * first item returned will be the one immediately after the start
     * Timetoken value. Default is NULL.
     */
    char const* start;
    /** If provided (not NULL), lets you select an "end date", in
     * Timetoken format. If not provided, it will provide up to the
     * number of messages defined in the "count" parameter. Page
     * through results by providing a start OR end time
     * token. Retrieve a slice of the time line by providing both a
     * start AND end time token. End is "inclusive" – that is, if a
     * message is associated exactly with the end Timetoken, it will
     * be included in the result. Default is NULL.
     */
    char const* end;
    /** If true to recieve metadata with each history
     * message if any. If false, no metadata per message. Defaults to
     * false.
     */
    bool include_meta;
    /** If true to recieve message type with each history
     * message. If false, no message type per message. Defaults to
     * false.
     */
    bool include_message_type;
    /** If true to receive user_id with each history
     * message. If false, no user_id per message. Defaults to
     * false.
     */
    bool include_user_id;
    /** If true to recieve message actions with each history
     * message. If false, no message actions per message. Defaults to
     * false.
     */
    bool include_message_actions;
};

/** This returns the default options for fetch history transactions.
    It's best to always call it to initialize the
    #pubnub_fetch_history_options, since it has several parameters.
 */
PUBNUB_EXTERN struct pubnub_fetch_history_options pubnub_fetch_history_defopts(void);


/** The fetch history. It is basically the same as the
    pubnub_history(), just adding a few options that will be sent.

    Basic usage:

        struct pubnub_fetch_history_options opt = pubnub_fetch_history_defopts();
        opt.reverse = true;
        pbresult = pubnub_fetch_history(pn, "my_channel1,my_channel2", opt);

    @param pb The Pubnub context. Can't be NULL.
    @param channel The string with the channel name(s) to get history for. Can't be
   NULL.
    @param opt Options for this fetch history transaction
    @return #PNR_STARTED on success, an error otherwise
*/
PUBNUB_EXTERN enum pubnub_res pubnub_fetch_history(pubnub_t*                     pb,
                                  char const*                   channel,
                                  struct pubnub_fetch_history_options opt);

PUBNUB_EXTERN pubnub_chamebl_t pubnub_get_fetch_history(pubnub_t* pb);

#endif /* !defined INC_PUBNUB_FETCH_HISTORY */

#endif /* PUBNUB_USE_FETCH_HISTORY */

