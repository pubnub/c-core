/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PBCC_OBJECTS_API
#define INC_PBCC_OBJECTS_API

#include "pubnub_api_types.h"
#include "pubnub_json_parse.h"

struct pbcc_context;


/** @file pbcc_objects_api.h

    This has the functions for formating and parsing the
    requests and responses for 'Objects API' transactions
  */

/** Serches for 'id' key value within @p obj and if it finds it valid,
    saves its position in @p parsed_id. If not returns corresponding error.
  */
enum pubnub_res pbcc_find_objects_id(struct pbcc_context* pb,
                                     char const* obj,
                                     struct pbjson_elem* parsed_id,
                                     char const* file,
                                     int line);

/** Prepares the 'get_users' transaction, mostly by
    formatting the URI of the HTTP request.
  */
enum pubnub_res pbcc_get_users_prep(struct pbcc_context* pb,
                                    char const** include,
                                    size_t include_count,
                                    size_t limit,
                                    char const* start,
                                    char const* end,
                                    enum pubnub_tribool count);

/** Prepares the 'create_user' transaction, mostly by
    formatting the URI of the HTTP request.
  */
enum pubnub_res pbcc_create_user_prep(struct pbcc_context* pb,
                                      char const** include,
                                      size_t include_count,
                                      const char* user_obj);

/** Prepares the 'get_user' transaction, mostly by
    formatting the URI of the HTTP request.
  */
enum pubnub_res pbcc_get_user_prep(struct pbcc_context* pb,
                                   char const** include,
                                   size_t include_count,
                                   char const* user_id);

/** Prepares the 'update_user' transaction, mostly by
    formatting the URI of the HTTP request.
  */
enum pubnub_res pbcc_update_user_prep(struct pbcc_context* pb,
                                      char const** include,
                                      size_t include_count,
                                      char const* user_obj,
                                      struct pbjson_elem const* id);



/** Prepares the 'delete_user' transaction, mostly by
    formatting the URI of the HTTP request.
  */
enum pubnub_res pbcc_delete_user_prep(struct pbcc_context* pb, char const* user_id);


/** Prepares the 'get_spaces' transaction, mostly by
    formatting the URI of the HTTP request.
  */
enum pubnub_res pbcc_get_spaces_prep(struct pbcc_context* pb,
                                     char const** include,
                                     size_t include_count,
                                     size_t limit,
                                     char const* start,
                                     char const* end,
                                     enum pubnub_tribool count);


/** Prepares the 'create_space' transaction, mostly by
    formatting the URI of the HTTP request.
  */
enum pubnub_res pbcc_create_space_prep(struct pbcc_context* pb,
                                       char const** include,
                                       size_t include_count,
                                       const char* space_obj);


/** Prepares the 'get_space' transaction, mostly by
    formatting the URI of the HTTP request.
  */
enum pubnub_res pbcc_get_space_prep(struct pbcc_context* pb,
                                    char const** include,
                                    size_t include_count,
                                    char const* space_id);


/** Prepares the 'update_space' transaction, mostly by
    formatting the URI of the HTTP request.
  */
enum pubnub_res pbcc_update_space_prep(struct pbcc_context* pb,
                                       char const** include,
                                       size_t include_count,
                                       char const* space_obj,
                                       struct pbjson_elem const* id);


/** Prepares the 'delete_space' transaction, mostly by
    formatting the URI of the HTTP request.
  */
enum pubnub_res pbcc_delete_space_prep(struct pbcc_context* pb, char const* space_id);


/** Prepares the 'get_memberships' transaction, mostly by
    formatting the URI of the HTTP request.
  */
enum pubnub_res pbcc_get_memberships_prep(struct pbcc_context* pb,
                                          char const* user_id,
                                          char const** include,
                                          size_t include_count,
                                          size_t limit,
                                          char const* start,
                                          char const* end,
                                          enum pubnub_tribool count);


/** Prepares the 'add', 'update' or 'remove' 'users space memberships' transactions
    depending on @p update_obj formed, mostly by formatting the URI of the HTTP request.
  */
enum pubnub_res pbcc_update_memberships_prep(struct pbcc_context* pb,
                                             char const* user_id,
                                             char const** include,
                                             size_t include_count,
                                             char const* update_obj);

/** Prepares the 'get_members' transaction, mostly by
    formatting the URI of the HTTP request.
  */
enum pubnub_res pbcc_get_members_prep(struct pbcc_context* pb,
                                      char const* space_id,
                                      char const** include,
                                      size_t include_count,
                                      size_t limit,
                                      char const* start,
                                      char const* end,
                                      enum pubnub_tribool count);

/** Prepares the 'update_members' transaction, mostly by
    formatting the URI of the HTTP request.
  */
enum pubnub_res pbcc_update_members_prep(struct pbcc_context* pb,
                                         char const* space_id,
                                         char const** include,
                                         size_t include_count,
                                         char const* update_obj);


/** Parses server response simply on any 'Objects API' transaction request.
    If transaction is successful, the response(a JSON object) will have key
    "data" with corresponding value. If not, there should be "error" key 'holding'
    error description. If there is neither of the two keys mentioned function
    returns response format error.
    Complete answer will be available via pubnub_get().

    @retval PNR_OK on success
    @retval PNR_OBJECTS_API_ERROR on error
    @retval PNR_FORMAT_ERROR no "data", nor "error" key present in response
  */
enum pubnub_res pbcc_parse_objects_api_response(struct pbcc_context* pb);


#endif /* !defined INC_PBCC_OBJECTS_API */
