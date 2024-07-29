/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#if PUBNUB_USE_OBJECTS_API

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

// TODO: maybe we should decrease amount of parameters in these functions

/** Prepares the 'get_users' transaction, mostly by
    formatting the URI of the HTTP request.
  */
enum pubnub_res pbcc_getall_uuidmetadata_prep(struct pbcc_context* pb,
                                    char const* include,
                                    size_t limit,
                                    char const* start,
                                    char const* end,
                                    enum pubnub_tribool count,
                                    char const* filter,
                                    char const* sort,
                                    enum pubnub_trans pt);

/** Prepares the 'set_uuidmetadata' transaction, mostly by
    formatting the URI of the HTTP request.
  */
enum pubnub_res pbcc_set_uuidmetadata_prep(struct pbcc_context* pb,
                                      char const* uuid_metadataid,
                                      char const* include,
                                      const char* uuid_metadata_obj,
                                      enum pubnub_trans pt);

/** Prepares the 'get_uuidmetadata' transaction, mostly by
    formatting the URI of the HTTP request.
  */
enum pubnub_res pbcc_get_uuidmetadata_prep(struct pbcc_context* pb,
                                   char const* include,
                                   char const* uuid_metadataid,
                                   enum pubnub_trans pt);


/** Prepares the 'remove_uuidmetadata' transaction, mostly by
    formatting the URI of the HTTP request.
  */
enum pubnub_res pbcc_remove_uuidmetadata_prep(struct pbcc_context* pb, char const* uuid_metadataid, enum pubnub_trans pt);


/** Prepares the 'getall_channelmetadata' transaction, mostly by
    formatting the URI of the HTTP request.
  */
enum pubnub_res pbcc_getall_channelmetadata_prep(struct pbcc_context* pb,
                                     char const* include,
                                     size_t limit,
                                     char const* start,
                                     char const* end,
                                     enum pubnub_tribool count,
                                     char const* filter,
                                     char const* sort,
                                     enum pubnub_trans pt);


/** Prepares the 'set_channelmetatdata' transaction, mostly by
    formatting the URI of the HTTP request.
  */
enum pubnub_res pbcc_set_channelmetadata_prep(struct pbcc_context* pb,
                                       char const* channel_metadataid,
                                       char const* include,
                                       const char* channel_metadata_obj,
                                       enum pubnub_trans pt);


/** Prepares the 'get_channelmetadata' transaction, mostly by
    formatting the URI of the HTTP request.
  */
enum pubnub_res pbcc_get_channelmetadata_prep(struct pbcc_context* pb,
                                    char const* include,
                                    char const* channel_metadataid,
                                    enum pubnub_trans pt);


/** Prepares the 'remove_channelmetadata' transaction, mostly by
    formatting the URI of the HTTP request.
  */
enum pubnub_res pbcc_remove_channelmetadata_prep(struct pbcc_context* pb, char const* channel_metadataid, enum pubnub_trans pt);


/** Prepares the 'get_memberships' transaction, mostly by
    formatting the URI of the HTTP request.
  */
enum pubnub_res pbcc_get_memberships_prep(struct pbcc_context* pb,
                                          char const* uuid_metadataid,
                                          char const* include,
                                          size_t limit,
                                          char const* start,
                                          char const* end,
                                          enum pubnub_tribool count,
                                          char const* filter,
                                          char const* sort,
                                          enum pubnub_trans pt);


/** Prepares the 'add', 'update' or 'remove' 'set_memberships' transactions
    depending on @p update_obj formed, mostly by formatting the URI of the HTTP request.
  */
enum pubnub_res pbcc_set_memberships_prep(struct pbcc_context* pb,
                                             char const* uuid_metadataid,
                                             char const* include,
                                             char const* update_obj,
                                             char const* filter,
                                             char const* sort,
                                             size_t limit,
                                             char const* start,
                                             char const* end,
                                             enum pubnub_tribool count,
                                             enum pubnub_trans pt);

/** Prepares the 'get_members' transaction, mostly by
    formatting the URI of the HTTP request.
  */
enum pubnub_res pbcc_get_members_prep(struct pbcc_context* pb,
                                      char const* channel_metadataid,
                                      char const* include,
                                      size_t limit,
                                      char const* start,
                                      char const* end,
                                      char const* filter,
                                      char const* sort,
                                      enum pubnub_tribool count,
                                      enum pubnub_trans pt);

/** Prepares the 'set_members' transaction, mostly by
    formatting the URI of the HTTP request.
  */
enum pubnub_res pbcc_set_members_prep(struct pbcc_context* pb,
                                         char const* channel_metadataid,
                                         char const* include,
                                         char const* set_obj, 
                                         char const* filter,
                                         char const* sort,
                                         size_t limit,
                                         char const* start,
                                         char const* end,
                                         enum pubnub_tribool count,
                                         enum pubnub_trans pt);


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

#endif /* PUBNUB_USE_OBJECTS_API */

