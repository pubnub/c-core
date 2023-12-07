/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#if PUBNUB_USE_OBJECTS_API

#if !defined INC_PUBNUB_OBJECTS_API
#define INC_PUBNUB_OBJECTS_API


#include "pubnub_api_types.h"
#include "lib/pb_extern.h"

#include <stdbool.h>


/** Returns a paginated list of metadata objects for users associated with the subscription key of the context @p pbp,
    optionally including each record's custom data object.
    @param pb The pubnub context. Can't be NULL
    @param include The comma delimited (C) string with additional/complex user attributes to include in
                   response. Use NULL if you don't want to retrieve additional attributes.
    @param limit Number of entities to return in response. Regular values 1 - 100. If you set `0`,
                 that means “use the default”. At the time of this writing, default was 100.
    @param start Previously-returned cursor bookmark for fetching the next page. Use NULL if you
                 don’t want to paginate with a start bookmark.
    @param end Previously-returned cursor bookmark for fetching the previous page. Ignored if you
               also supply the start parameter. Use NULL if you don’t want to paginate with an
               end bookmark.
    @param count Request totalCount to be included in paginated response. By default, totalCount
                 is omitted.
    @return #PNR_STARTED on success, an error otherwise
  */
PUBNUB_EXTERN enum pubnub_res pubnub_getall_uuidmetadata(pubnub_t* pb,
                                 char const* include, 
                                 size_t limit,
                                 char const* start,
                                 char const* end,
                                 enum pubnub_tribool count);

/** Creates a metadata for a uuid with the attributes specified in @p uuid_metadata_obj.
    Returns the created metadata uuid object, optionally including the user's custom data object.
    @note User ID and name are required properties in the @p uuid_metadata_obj
    @param pb The pubnub context. Can't be NULL
    @param include The comma delimited (C) string with additional/complex user attributes to include in
                   response. Use NULL if you don't want to retrieve additional attributes.
    @param include_count dimension of @p include. Set 0 if you don’t want to retrieve additional
                         attributes
    @param uuid_metadata_obj The JSON string with the definition of the UUID Metadata
                    Object to create.
    @return #PNR_STARTED on success, an error otherwise
  */
PUBNUB_EXTERN enum pubnub_res pubnub_set_uuidmetadata(pubnub_t* pb, 
    char const* uuid_metadataid,
    char const* include,
    char const* uuid_metadata_obj);


/** Returns the uuid metadata object specified with @p user_id, optionally including the user's
    custom data object.
    @param pb The pubnub context. Can't be NULL
    @param include The comma delimited (C) string with additional/complex user attributes to include in
                   response. Use NULL if you don't want to retrieve additional attributes.
    @param uuid_metadataid The UUID Metadata ID for which to retrieve the user object.
                   Cannot be NULL.
    @return #PNR_STARTED on success, an error otherwise
  */
PUBNUB_EXTERN enum pubnub_res pubnub_get_uuidmetadata(pubnub_t* pb,
                                char const* include, 
                                char const* uuid_metadataid);



/** Deletes the uuid metadata specified with @p uuid_metadataid.
    @param pb The pubnub context. Can't be NULL
    @param uuid_metadataid The UUID Metatdata ID. Cannot be NULL.
    @return #PNR_STARTED on success, an error otherwise
  */
PUBNUB_EXTERN enum pubnub_res pubnub_remove_uuidmetadata(pubnub_t* pb, char const* uuid_metadataid);


/** Returns the spaces associated with the subscriber key of the context @p pbp, optionally
    including each space's custom data object.
    @param pb The pubnub context. Can't be NULL
    @param include array of (C) strings in comma delimited format with additional/complex attributes to include in response.
                   Use NULL if you don't want to retrieve additional attributes.
    @param limit Number of entities to return in response. Regular values 1 - 100. If you set `0`,
                 that means “use the default”. At the time of this writing, default was 100.
    @param start Previously-returned cursor bookmark for fetching the next page. Use NULL if you
                 don’t want to paginate with a start bookmark.
    @param end Previously-returned cursor bookmark for fetching the previous page. Ignored if
               you also supply the start parameter. Use NULL if you don’t want to paginate with
               an end bookmark.
    @param count Request totalCount to be included in paginated response. By default, totalCount
                 is omitted.
    @return #PNR_STARTED on success, an error otherwise
  */
PUBNUB_EXTERN enum pubnub_res pubnub_getall_channelmetadata(pubnub_t* pb, 
                                  char const* include, 
                                  size_t limit,
                                  char const* start,
                                  char const* end,
                                  enum pubnub_tribool count);


/** Creates a metadata for the specified channel with the attributes specified in @p channel_metadata_obj.
    Returns the created space object, optionally including its custom data object.
    @note Channel ID and name are required properties of @p channel_metadata_obj
    @param pb The pubnub context. Can't be NULL
    @param include array of (C) strings in comma delimited format with additional/complex attributes to include in response.
                   Use NULL if you don't want to retrieve additional attributes.
    @param channel_metadata_obj The JSON string with the definition of the channel metadata Object to create.
    @return #PNR_STARTED on success, an error otherwise
  */
PUBNUB_EXTERN enum pubnub_res pubnub_set_channelmetadata(pubnub_t* pb,
                                    char const* channel_metadataid,
                                    char const* include, 
                                    char const* channel_metadata_obj);


/** Returns the channel metadata object specified with @p channel_metadataid, optionally including its custom
    data object.
    @param pb The pubnub context. Can't be NULL
    @param include array of (C) strings in comma delimited format with additional/complex attributes to include in response.
                   Use NULL if you don't want to retrieve additional attributes.
    @param channel_metadataid The Channel ID for which to retrieve the channel metadata object. Cannot be NULL.
    @return #PNR_STARTED on success, an error otherwise
  */
PUBNUB_EXTERN enum pubnub_res pubnub_get_channelmetadata(pubnub_t* pb,
                                 char const* include, 
                                 char const* channel_metadataid);



/** Deletes the channel metadata specified with @p channel_metadataid.
    @param pb The pubnub context. Can't be NULL
    @param channel_metadataid The Channel ID. Cannot be NULL.
    @return #PNR_STARTED on success, an error otherwise
  */
PUBNUB_EXTERN enum pubnub_res pubnub_remove_channelmetadata(pubnub_t* pb, char const* channel_metadataid);


/** Returns the channel memberships of the user specified by @p uuid_metadataid, optionally including
    the custom data objects for...
    @param pb The pubnub context. Can't be NULL
    @param uuid_metadataid The UUID to retrieve the memberships.
                   Cannot be NULL.
    @param include array of (C) strings in comma delimited format with additional/complex attributes to include in response.
                   Use NULL if you don't want to retrieve additional attributes.
    @param limit Number of entities to return in response. Regular values 1 - 100.
                 If you set `0`, that means “use the default”. At the time of this writing,
                 default was 100.
    @param start Previously-returned cursor bookmark for fetching the next page. Use NULL if you
                 don’t want to paginate with a start bookmark.
    @param end Previously-returned cursor bookmark for fetching the previous page. Ignored if
               you also supply the start parameter. Use NULL if you don’t want to paginate with
               an end bookmark.
    @param count Request totalCount to be included in paginated response. By default, totalCount
                 is omitted.
    @return #PNR_STARTED on success, an error otherwise
  */
PUBNUB_EXTERN enum pubnub_res pubnub_get_memberships(pubnub_t* pb,
                                       char const* uuid_metadataid,
                                       char const* include,
                                       size_t limit,
                                       char const* start,
                                       char const* end,
                                       enum pubnub_tribool count);


/** Add/Update the channel memberships of the UUID specified by @p metadata_uuid. Uses the `set` property
    to perform those operations on one, or more memberships.
    An example for @set_obj:
       [
         {
           "channel":{ "id": "main-channel-id" },
           "custom": {
             "starred": true
           }
         },
         {
           "channel":{ "id": "channel-0" },
            "some_key": {
              "other_key": "other_value"
            }
         }
       ]

    @param pb The pubnub context. Can't be NULL
    @param uuid_metadataid The UUID to add/update the memberships.
                   Cannot be NULL.
    @param include array of (C) strings in comma delimited format with additional/complex attributes to include in response.
                   Use NULL if you don't want to retrieve additional attributes.
    @param set_obj The JSON object that defines the add/update to perform.
                      Cannot be NULL.
    @return #PNR_STARTED on success, an error otherwise
  */
PUBNUB_EXTERN enum pubnub_res pubnub_set_memberships(pubnub_t* pb, 
                                          char const* uuid_metadataid,
                                          char const* include,
                                          char const* set_obj);


/** Removes the memberships of the user specified by @p uuid_metadataid. Uses the `delete` property
    to perform those operations on one, or more memberships.
    An example for @remove_obj:
      [
        {
          "id": "main-channel-id"
        },
        {
          "id": "channel-0"
        }
      ]

    @param pb The pubnub context. Can't be NULL
    @param uuid_metadataid The UUID to remove the memberships.
                   Cannot be NULL.
    @param include array of (C) strings in comma delimited format with additional/complex attributes to include in response.
                   Use NULL if you don't want to retrieve additional attributes.
    @param remove_obj The JSON object that defines the remove to perform.
                      Cannot be NULL.
    @return #PNR_STARTED on success, an error otherwise
  */
PUBNUB_EXTERN enum pubnub_res pubnub_remove_memberships(pubnub_t* pb, 
                                    char const* uuid_metadataid,
                                    char const* include,
                                    char const* remove_obj);


/** Returns all users in the channel specified with @p channel_metadataid, optionally including
    the custom data objects for...
    @param pb The pubnub context. Can't be NULL
    @param channel_metadataid The Channel ID for which to retrieve the user metadata object.
    @param include array of (C) strings in comma delimited format with additional/complex attributes to include in response.
                   Use NULL if you don't want to retrieve additional attributes.
    @param limit Number of entities to return in response. Regular values 1 - 100.
                 If you set `0`, that means “use the default”. At the time of this writing,
                 default was 100.
    @param start Previously-returned cursor bookmark for fetching the next page. Use NULL if you
                 don’t want to paginate with a start bookmark.
    @param end Previously-returned cursor bookmark for fetching the previous page. Ignored if
               you also supply the start parameter. Use NULL if you don’t want to paginate with
               an end bookmark.
    @param count Request totalCount to be included in paginated response. By default, totalCount
                 is omitted.
    @return #PNR_STARTED on success, an error otherwise
  */
PUBNUB_EXTERN enum pubnub_res pubnub_get_members(pubnub_t* pb,
                                   char const* channel_metadataid,
                                   char const* include,
                                   size_t limit,
                                   char const* start,
                                   char const* end,
                                   enum pubnub_tribool count);


/** Adds the list of members of the channel specified with @p channel_metadataid. Uses the `add`
    property to perform the operation on one or more members.
    An example for @add_obj:
       [
         {
           "id": "some-user-id"
         },
         {
           "id": "user-0-id"
         }
       ]

    @param pb The pubnub context. Can't be NULL
    @param channel_metadataid The Channel ID.
    @param include array of (C) strings in comma delimited format with additional/complex attributes to include in response.
                   Use NULL if you don't want to retrieve additional attributes.
    @param add_obj The JSON object that defines the add to perform. Cannot be NULL.
    @return #PNR_STARTED on success, an error otherwise
  */
PUBNUB_EXTERN enum pubnub_res pubnub_add_members(pubnub_t* pb, 
                                   char const* channel_metadataid,
                                   char const* include,
                                   char const* add_obj);


/** Updates the list of members of the space specified with @p space_id. Uses the `update`
    property to perform the operation on one or more members.
    An example for @set_obj:
       [
         {
           "id": "some-user-id",
           "custom": {
             "starred": true
           }
         },
         {
           "id": "user-0-id",
            "some_key": {
              "other_key": "other_value"
            }
         }
       ]

    @param pb The pubnub context. Can't be NULL
    @param channel_metadataid The Channel ID for which to add/update the user metadata.
    @param include array of (C) strings in comma delimited format with additional/complex attributes to include in response.
                   Use NULL if you don't want to retrieve additional attributes.
    @param set_obj The JSON object that defines the add/update to perform. Cannot be NULL.
    @return #PNR_STARTED on success, an error otherwise
  */
PUBNUB_EXTERN enum pubnub_res pubnub_set_members(pubnub_t* pb, 
                                      char const* channel_metadataid,
                                      char const* include,
                                      char const* set_obj);


/** Removes the list of members of the space specified with @p space_id. Uses the `remove`
    property to perform the operation on one or more members.
    An example for @update_obj:
      [
        {
          "id": "some-user-id",
          "custom": {
            "starred": true
          }
        },
        {
          "id": "user-0-id"
        }
      ]

    @param pb The pubnub context. Can't be NULL
    @param channel_metadataid The Channel ID.
    @param include array of (C) strings in comma delimited format with additional/complex attributes to include in response.
                   Use NULL if you don't want to retrieve additional attributes.
    @param remove_obj The JSON object that defines the remove to perform. Cannot be NULL.
    @return #PNR_STARTED on success, an error otherwise
  */
PUBNUB_EXTERN enum pubnub_res pubnub_remove_members(pubnub_t* pb, 
                                      char const* channel_metadataid,
                                      char const* include,
                                      char const* remove_obj);


#endif /* !defined INC_PUBNUB_OBJECTS_API */

#endif /* PUBNUB_USE_OBJECTS_API */

