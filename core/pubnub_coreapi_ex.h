/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_COREAPI_EX
#define INC_PUBNUB_COREAPI_EX


#include "pubnub_api_types.h"
#include "lib/pb_extern.h"

#include <stdbool.h>
#include <stdlib.h>


/** @file pubnub_coreapi_ex.h

    This is the "Extended Core" API of the Pubnub client library.  It
    has the "extended" versions of (most of the)
    "transaction-starting" functions from the "Core" API. These
    extended versions accept more parameters. The functions in the
    Core API proper use defaults for these additional parameters and,
    in general, use less bandwith, as those defaults are not
    transmitted to Pubnub network.

    Each extended function has a structure holding (all) the options
    and a helper function which provides defaults for the options.  So,
    if you don't care about all the options, you can get the defaults
    and set only the ones you do care about. This is also a good way
    to be "future proof", as new versions of Pubnub SDK may introduce
    new options, with defaults added to these functions.
*/


/** Options for "extended" publish V1. */
struct pubnub_publish_options {
    /** If true, the message is stored in history. If false,
        the message is _not_ stored in history.
    */
    bool store;
    /** If not NULL, the key used to encrypt the message before
        sending it to Pubnub. Keep in mind that encryption is a CPU
        intensive task. Also, it uses more bandwidth, most of which
        comes from the fact that decrypted data is sent as Base64
        encoded JSON string. This means that the actual amount of data
        that can be sent encrypted in a message is at least 25%
        smaller (than un-encrypted). Another point to be made is that
        it also does some memory management (allocting and
        deallocating).

        @deprecated Use `pubnub_set_crypto_module()` instead.
    */
    char const* cipher_key;
    /** If `true`, the message is replicated, thus will be received by
        all subscribers. If `false`, the message is _not_ replicated
        and will be only delivered to BLOCK event handlers. Setting
        `false` here and `false` on store is sometimes referred to as
        a "fire" (instead of a "publish").
    */
    bool replicate;
    /** An optional JSON object, used to send additional ("meta") data
        about the message, which can be used for stream filtering.
     */
    char const* meta;
    /** Defines the method by which publish transaction will be performed */
    enum pubnub_method method;
    /** For how many hours message should be kept and available with history
        API.
     */
    size_t ttl;
    /** User-specified message type.
        Important: String limited by **3**-**50** case-sensitive alphanumeric
        characters with only `-` and `_` special characters allowed.
     */
    char const* custom_message_type;
};

/** This returns the default options for publish V1 transactions.
    Will set `store = true`, `cipher_key = NULL`, `replicate = true`,
    `meta = NULL` and `method = pubnubPublishViaGet`
 */
PUBNUB_EXTERN struct pubnub_publish_options pubnub_publish_defopts(void);

/** The extended publish V1. Basically the same as pubnub_publish(),
    but with added optional parameters in @p opts.

    Basic usage:

        struct pubnub_publish_options opt = pubnub_publish_defopts();
        opt.store = false;
        pbresult = pubnub_publish_ex(pn, "my_channel", "42", opt);

    @param p The Pubnub context. Can't be NULL.
    @param channel The string with the channel name to publish to. Can't be
   NULL.
    @param opts Publish V1 options
    @return #PNR_STARTED on success, an error otherwise
*/
PUBNUB_EXTERN enum pubnub_res pubnub_publish_ex(pubnub_t*                     p,
                                  const char*                   channel,
                                  const char*                   message,
                                  struct pubnub_publish_options opts);


/** Options for "extended" signal V1. */
struct pubnub_signal_options {
    /** User-specified message type.
        Important: String limited by **3**-**50** case-sensitive alphanumeric
        characters with only `-` and `_` special characters allowed.
     */
    char const* custom_message_type;
};

/** This returns the default options for signal V1 transactions. */
PUBNUB_EXTERN struct pubnub_signal_options pubnub_signal_defopts(void);

/** The extended signal V1. Basically the same as pubnub_signal(),
    but with added optional parameters in @p opts.

    Basic usage:

        struct pubnub_signal_options opt = pubnub_signal_defopts();
        opt.custom_message_type = "test-message-type";
        pbresult = pubnub_signal_ex(pn, "my_channel", "status:active", opt);

    @param pb The pubnub context. Can't be NULL
    @param channel The string with the channel to signal to.
    @param message The signal message to send, expected to be in JSON format
    @param opts Signal V1 options
    @return #PNR_STARTED on success, an error otherwise
*/
PUBNUB_EXTERN enum pubnub_res pubnub_signal_ex(
    pubnub_t* pb,
    const char* channel,
    const char* message,
    struct pubnub_signal_options opts);


/** Options for "extended" subscribe. */
struct pubnub_subscribe_options {
    /** Channel group (a comma-delimited list of channel group names).
        If NULL, will not be used */
    char const* channel_group;
    /** The channel activity timeout, in seconds. If below the minumum
        value supported by Pubnub, the minimum value will be used
        (instead).
     */
    unsigned heartbeat;
};


/** This returns the default options for subscribe transactions.  Will
    set `channel_group = NULL`, `heartbeat` to default heartbeat
    value.
 */
PUBNUB_EXTERN struct pubnub_subscribe_options pubnub_subscribe_defopts(void);

/** The extended subscribe. Basically the same as pubnub_subscribe()
    but with options (except @p channel) given in @p opts.

    When auto heartbeat is enabled at compile time both @p channel
    and channel groups could be passed as NULL which suggests default
    behaviour(unless it is uuid's very first subscription) in which case
    transaction uses channel and channel groups that are already subscribed.

    Basic usage:

        struct pubnub_subscribe_options opt = pubnub_subscribe_defopts();
        opt.heartbeat = 412;
        pbresult = pubnub_subscribe_ex(pn, "my_channel", opt);

    @param p The Pubnub context. Can't be NULL.
    @param channel The string with the channel name (or
    comma-delimited list of channel names) to subscribe for.
    @param opt Subscribe options
    @return #PNR_STARTED on success, an error otherwise
*/
PUBNUB_EXTERN enum pubnub_res pubnub_subscribe_ex(pubnub_t*                       p,
                                    const char*                     channel,
                                    struct pubnub_subscribe_options opts);


/** Options for "extended" here now, global or not, doesn't matter. */
struct pubnub_here_now_options {
    /** Channel group (a comma-delimited list of channel group names).
        If NULL, will not be used */
    char const* channel_group;
    /** If true will not give uuids associated with occupancy */
    bool disable_uuids;
    /** If true (and if `disable_uuds` is false), will give associated
        state alongside uuid info
     */
    bool state;
};

/** This returns the default options for here-now transactions.  Will
    set `channel_group = NULL`, `disable_uuids=true` and `state =
    false`.
 */
PUBNUB_EXTERN struct pubnub_here_now_options pubnub_here_now_defopts(void);

/** The extended "here now". It is basically the same as the
    pubnub_here_now(), just adding a few options that will be sent.
    Also, "channel_group" parameter is moved to options, it is not a
    "regular" function parameter, but, its behavior is otherwise the
    same.

    Basic usage:

        struct pubnub_here_now_options opt = pubnub_here_now_defopts();
        opt.state = 1;
        pbresult = pubnub_here_now_ex(pn, "my_channel", opt);

    @param p The Pubnub context. Can't be NULL.
    @param channel The string with the channel name (or
    comma-delimited list of channel names) to get presence info for.
    @param opt Here-now options for this here-now
    @return #PNR_STARTED on success, an error otherwise
*/
PUBNUB_EXTERN enum pubnub_res pubnub_here_now_ex(pubnub_t*                      p,
                                   const char*                    channel,
                                   struct pubnub_here_now_options opt);

/** This is to pubnub_here_now_ex() as pubnub_global_here_now() is to
    pubnub_here_now().

    @param p The Pubnub context. Can't be NULL.
    @param opt Here-now options for this here-now. Keep in mind that
    `opt.channel_group` has to be NULL here.

    @return #PNR_STARTED on success, an error otherwise
*/
PUBNUB_EXTERN enum pubnub_res pubnub_global_here_now_ex(pubnub_t*                      p,
                                          struct pubnub_here_now_options opt);


/** Options for "extended" history. */
struct pubnub_history_options {
    /** If false, the returned start and end Timetoken values will be
     * returned as long ints. If true, the start and end Timetoken
     * values will be returned as strings. Default is false. */
    bool string_token;
    /** The maximum number of messages to return per response. Has to
     * be between 1 and 100 messages. Default is 100.
     */
    int count;
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
    /** If true to recieve a timetoken with each history
     * message. If false, no timetokens per message. Defaults to
     * false.
     */
    bool include_token;
    /** If true to recieve metadata with each history
     * message if any. If false, no metadata per message. Defaults to
     * false.
     */
    bool include_meta;
};


/** This returns the default options for history transactions.
    It's best to always call it to initialize the
    #pubnub_history_options, since it has several parameters.
 */
PUBNUB_EXTERN struct pubnub_history_options pubnub_history_defopts(void);

/** The extended "history". It is basically the same as the
    pubnub_history(), just adding a few options that will be sent.

    Basic usage:

        struct pubnub_history_options opt = pubnub_history_defopts();
        opt.reverse = true;
        pbresult = pubnub_history_ex(pn, "my_channel", opt);

    @param pb The Pubnub context. Can't be NULL.
    @param channel The string with the channel name to get history for. Can't be
   NULL.
    @param opt Options for this history transaction
    @return #PNR_STARTED on success, an error otherwise
*/
PUBNUB_EXTERN enum pubnub_res pubnub_history_ex(pubnub_t*                     pb,
                                  char const*                   channel,
                                  struct pubnub_history_options opt);

/** Options for "extended" set_state. */
struct pubnub_set_state_options {
    /** Channel group (a comma-delimited list of channel group names).
        If NULL, will not be used */
    char const* channel_group;
  
    /** The identification of the user for which to set state for.
    If NULL, the current user_id of the pubnub context will be used.
     */ 
    char const* user_id;
   
    /** The channel activity timeout, in seconds. If below the minumum
        value supported by Pubnub, the minimum value will be used
        (instead).
     */
    bool heartbeat;
};


/** This returns the default options for set_state transactions.  Will
    set `channel_group = NULL`, `user_id = NULL` and `heartbeat to false`
 */
PUBNUB_EXTERN struct pubnub_set_state_options pubnub_set_state_defopts(void);

/** The extended set_state. Basically the same as pubnub_set_state()
    but with options (except @p channel) given in @p opts.

    When heartbeat is enabled will use `/heartbeat` endpoint instead of 
    the `/data` one. this change will allow user to set state and make 
    a heartbeat call at once.

    Basic usage:

        struct pubnub_set_state_options opt = pubnub_set_state_defopts();
        opt.heartbeat = true;
        pbresult = pubnub_set_state_ex(pn, "my_channel", "{}", opt);

    @param p The Pubnub context. Can't be NULL.
    @param channel The string with the channel name (or
    comma-delimited list of channel names) to subscribe for.
    @param state State that user want's to provide
    @param opt Subscribe options
    @return #PNR_STARTED on success, an error otherwise
*/
PUBNUB_EXTERN enum pubnub_res pubnub_set_state_ex(pubnub_t*                       p,
                                    const char*                     channel,
                                    const char*                     state,
                                    struct pubnub_set_state_options opts);


#endif /* defined INC_PUBNUB_COREAPI_EX */
