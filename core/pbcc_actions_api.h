/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#if PUBNUB_USE_ACTIONS_API

#if !defined INC_PBCC_ACTIONS_API
#define INC_PBCC_ACTIONS_API

#include "pubnub_api_types.h"
#include "pubnub_memory_block.h"
#include "lib/pb_deprecated.h"

struct pbcc_context;


/** @file pbcc_actions_api.h

    This has the functions for formating and parsing the
    requests and responses for 'Actions API' transactions
  */

/** Possible action types */
enum pubnub_action_type {
    pbactypReaction,
    pbactypReceipt,
    pbactypCustom
};

/** Forms the action object to be sent in 'pubnub_add_action' request body.
    
    @deprecated This function is deprecated. Use pbcc_form_the_action_object_str() instead.
                The present declaration will be changed to the string version in the future.
    @see pbcc_form_the_action_object_str

    @return #PNR_OK on success, an error otherwise
  */
PUBNUB_DEPRECATED enum pubnub_res pbcc_form_the_action_object(struct pbcc_context* pb,
                                            char* obj_buffer,
                                            size_t buffer_size,
                                            enum pubnub_action_type actype,
                                            char const** json);


/** Forms the action object to be sent in 'pubnub_add_action' request body.
    @return #PNR_OK on success, an error otherwise
  */
enum pubnub_res pbcc_form_the_action_object_str(struct pbcc_context* pb,
                                            char* obj_buffer,
                                            size_t buffer_size,
                                            const char* action_type,
                                            char const** json);


/** Prepares the 'add_action' transaction, mostly by
    formatting the URI of the HTTP request.
  */
enum pubnub_res pbcc_add_action_prep(struct pbcc_context* pb,
                                     char const* channel, 
                                     char const* message_timetoken, 
                                     char const* value);

/** Retrives message timetoken from the existing transaction response
  */
pubnub_chamebl_t pbcc_get_message_timetoken(struct pbcc_context* pb);

/** Retrives action timetoken from the existing transaction response
  */
pubnub_chamebl_t pbcc_get_action_timetoken(struct pbcc_context* pb);

/** Prepares the 'remove_action' transaction, mostly by
    formatting the URI of the HTTP request.
  */
enum pubnub_res pbcc_remove_action_prep(struct pbcc_context* pb,
                                        char const* channel,
                                        pubnub_chamebl_t message_timetoken,
                                        pubnub_chamebl_t action_timetoken);

/** Prepares the 'get_actions' transaction, mostly by
    formatting the URI of the HTTP request.
  */
enum pubnub_res pbcc_get_actions_prep(struct pbcc_context* pb,
                                      char const* channel,
                                      char const* start,
                                      char const* end,
                                      size_t limit);

/** Prepares the 'get_actions_more' transaction, mostly by
    formatting the URI of the HTTP request.
  */
enum pubnub_res pbcc_get_actions_more_prep(struct pbcc_context* pb);

/** Prepares the 'history_with_actions' transaction, mostly by
    formatting the URI of the HTTP request.
  */
enum pubnub_res pbcc_history_with_actions_prep(struct pbcc_context* pb,
                                               char const* channel,
                                               char const* start,
                                               char const* end,
                                               size_t limit);

/** Parses server response simply on most 'Actions API' transaction requests.
    If transaction is successful, the response(a JSON object) will have key
    "data" with corresponding value. If not, there should be "error" key 'holding'
    error description. If there is neither of the two keys mentioned function
    returns response format error.
    Complete answer will be available via pubnub_get().

    @retval PNR_OK on success
    @retval PNR_ACTIONS_API_ERROR on error
    @retval PNR_FORMAT_ERROR no "data", nor "error" key present in response
  */
enum pubnub_res pbcc_parse_actions_api_response(struct pbcc_context* pb);

/** Parses server response simply on 'pubnub_history_with_actions' transaction request.
    If transaction is successful, the response(a JSON object) will have key
    "channels" with corresponding value. If not, there should be "error_message" key
    'holding' error description. If there is neither of the two keys mentioned function
    returns response format error.
    Complete answer will be available via pubnub_get().

    @retval PNR_OK on success
    @retval PNR_ACTIONS_API_ERROR on error
    @retval PNR_FORMAT_ERROR no "channels", nor "error_mesage" key present in response
  */
enum pubnub_res pbcc_parse_history_with_actions_response(struct pbcc_context* pb);

#endif /* !defined INC_PBCC_ACTIONS_API */

#endif /* PUBNUB_USE_ACTIONS_API */

