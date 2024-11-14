/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#ifndef INC_PBCC_FETCH_HISTORY
#define INC_PBCC_FETCH_HISTORY
#if PUBNUB_USE_FETCH_HISTORY

struct pbcc_context;

enum pubnub_res pbcc_fetch_history_prep(struct pbcc_context* pb,
                                  const char*          channel,
                                  unsigned int         max_per_channel,
                                  enum pubnub_tribool  include_meta,
                                  enum pubnub_tribool  include_custom_message_type,
                                  enum pubnub_tribool  include_message_type,
                                  enum pubnub_tribool  include_user_id,
                                  enum pubnub_tribool  include_message_actions,
                                  enum pubnub_tribool  reverse,
                                  char const*          start,
                                  char const*          end);


/** Parses server response simply on fetch history transaction request.
    If transaction is successful, the response(a JSON object) will have key
    "data" with corresponding value. If not, there should be "error" key 'holding'
    error description. If there is neither of the two keys mentioned function
    returns response format error.
    Complete answer will be available via pbcc_get_fetch_history().

    @retval PNR_OK on success
    @retval PNR_FETCH_HISTORY_ERROR on error
    @retval PNR_FORMAT_ERROR no "data", nor "error" key present in response
  */
enum pubnub_res pbcc_parse_fetch_history_response(struct pbcc_context* pb);

pubnub_chamebl_t pbcc_get_fetch_history(struct pbcc_context* pb);

#endif /* PUBNUB_USE_FETCH_HISTORY */
#endif /* #ifndef INC_PBCC_FETCH_HISTORY */

