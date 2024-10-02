/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#if PUBNUB_USE_OBJECTS_API

#if !defined INC_PUBNUB_OBJECTS_API
#define INC_PUBNUB_OBJECTS_API


#include "pubnub_api_types.h"
#include "lib/pb_extern.h"

/** Data pagination object. This object helps to paginate through the data.
   It is more readable form of `start` and `end` parameters in objects API.
 */
struct pubnub_page_object {
    /** The URL for the next page of results. If there is no next page, this field is null. */
    char const* next;

    /** The URL for the previous page of results. If there is no previous page, this field is null. */
    char const* prev;
};


/** User data object. This object represents the user data. */
struct pubnub_user_data {

    /** Name Display name for the user. */
    char const* name;

    /** User's identifier in an external system */
    char const* external_id;

    /** The URL of the user's profile picture */
    char const* profile_url;

    /** The user's email. */
    char const* email;

    /** JSON providing custom data about the user. Values must be scalar only; arrays or objects are not supported.
        Filtering App Context data through the custom property is not recommended in SDKs. */
    char const* custom;
};


/** Channel data object. This object represents the channel data. */
struct pubnub_channel_data {
    /** Display name for the channel. */
    char const* name;

    /** The channel's description. */
    char const* description;

    /** JSON providing custom data about the channel. Values must be scalar only; arrays or objects are not supported.
        Filtering App Context data through the custom property is not recommended in SDKs. */
    char const* custom;
};


/** The "null" page object, which is used to indicate that there is no next or previous page.
    Also it can be used when there is no will to pass page as a parameter 
    */
PUBNUB_EXTERN const struct pubnub_page_object pubnub_null_page;


/** The "null" user data object, which is used to indicate that there is no user data.
    Also it can be used when there is no will to pass user data as a parameter 
    */
PUBNUB_EXTERN const struct pubnub_user_data pubnub_null_user_data;


/** The "null" channel data object, which is used to indicate that there is no channel data.
    Also it can be used when there is no will to pass channel data as a parameter 
    */
PUBNUB_EXTERN const struct pubnub_channel_data pubnub_null_channel_data;


/** Options for the getall_*metadata functions */
struct pubnub_getall_metadata_opts {
    /** The comma delimited (C) string with additional/complex attributes to include in response.
        Use NULL if you don't want to retrieve additional attributes. */
    char const* include;

    /** Expression used to filter the results. Only objects whose properties satisfy the given expression are returned. */
    char const* filter;

    /** Key-value pair of a property to sort by, and a sort direction. Available options are id, name, and updated.
        Use asc or desc to specify sort direction, or specify null to take the default sort direction (ascending).
        For example: {name: 'asc'} */
    char const* sort;

    /** Number of entities to return in response. Regular values 1 - 100. If you set `0`,
        that means “use the default”. At the time of this writing, default was 100. */
    size_t limit;

    /** Pagination object.
        @see pubnub_page_object */
    struct pubnub_page_object page;

    /** Request totalCount to be included in paginated response. By default, totalCount is omitted. */
    enum pubnub_tribool count;
};


/** Default options for the set_*metadata functions 
    Values are set as follows:
    - include: NULL 
    - filter: NULL 
    - sort: NULL 
    - limit: 100
    - page: pubnub_null_page
    - count: pbccNotSet

    @see pubnub_getall_metadata_opts 
    @see pubnub_null_page
    
    @return Default options for the set_*metadata functions
  */
PUBNUB_EXTERN struct pubnub_getall_metadata_opts pubnub_getall_metadata_defopts();


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


/** Returns a paginated list of metadata objects for users associated with the subscription key of the context @p pbp,
    optionally including each record's custom data object.
    
    Use `pubnub_getall_metadata_defopts()` to get the default options.

    @see pubnub_getall_metadata_defopts

    @param pb The pubnub context. Can't be NULL
    @param opts The options for the getall_uuidmetadata function.
    @return #PNR_STARTED on success, an error otherwise
  */
PUBNUB_EXTERN enum pubnub_res pubnub_getall_uuidmetadata_ex(pubnub_t* pb, struct pubnub_getall_metadata_opts opts);


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


/** Options for the `pubnub_set_uuidmetadata_ex` function */
struct pubnub_set_uuidmetadata_opts {
    /** Uuid of the user. */
    char const* uuid; 

    /** The comma delimited (C) string with additional/complex attributes to include in response.
        Use NULL if you don't want to retrieve additional attributes. */
    char const* include;

    /** The user data object. */
    struct pubnub_user_data data;
};

/** Default options for the set_uuidmetadata functions 
    Values are set as follows:
    - uuid: NULL (`pubnub_set_uuidmetadata_ex` will take the user_id from the context on NULL)
    - include: NULL 
    - data: pubnub_null_user_data
    
    @see pubnub_null_user_data
    
    @return Default options for the set_uuidmetadata functions
  */
PUBNUB_EXTERN struct pubnub_set_uuidmetadata_opts pubnub_set_uuidmetadata_defopts();


/** Creates a metadata for a uuid with the attributes specified in @p opts.
    Returns the created metadata uuid object, optionally including the user's custom data object.
    
    Use `pubnub_set_uuidmetadata_defopts()` to get the default options.

    @see pubnub_set_uuidmetadata_defopts

    @param pb The pubnub context. Can't be NULL
    @param uuid The UUID to create the metadata for. If NULL, the user_id will be taken from the context.
    @param opts The options for the set_uuidmetadata function.
    @return #PNR_STARTED on success, an error otherwise
  */
PUBNUB_EXTERN enum pubnub_res pubnub_set_uuidmetadata_ex(pubnub_t* pb, struct pubnub_set_uuidmetadata_opts opts);


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


/** Returns the spaces associated with the subscriber key of the context @p pbp, optionally
    including each space's custom data object.
    
    Use `pubnub_getall_metadata_defopts()` to get the default options.

    @see pubnub_getall_metadata_defopts

    @param pb The pubnub context. Can't be NULL 
    @param opts The options for the getall_channelmetadata function.
    @return #PNR_STARTED on success, an error otherwise
    */
PUBNUB_EXTERN enum pubnub_res pubnub_getall_channelmetadata_ex(pubnub_t* pb, struct pubnub_getall_metadata_opts opts);


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


/** Options for the `pubnub_set_channelmetadata_ex` function */
struct pubnub_set_channelmetadata_opts {
    /** The comma delimited (C) string with additional/complex attributes to include in response.
        Use NULL if you don't want to retrieve additional attributes. */
    char const* include;

    /** The channel data object. */
    struct pubnub_channel_data data;
};


/** Default options for the set_channelmetadata functions. 
    Values are set as follows:
    - include: NULL 
    - data: pubnub_null_channel_data
    
    @see pubnub_null_channel_data
    
    @return Default options for the set_channelmetadata_ex function.
  */
PUBNUB_EXTERN struct pubnub_set_channelmetadata_opts pubnub_set_channelmetadata_defopts();


/** Creates a metadata for the specified channel with the attributes specified in @p opts.
    Returns the created space object, optionally including its custom data object.
    
    Use `pubnub_set_channelmetadata_defopts()` to get the default options.

    @see pubnub_set_channelmetadata_defopts

    @param pb The pubnub context. Can't be NULL
    @param channel The channel to create the metadata for. If NULL, the channel will be taken from the context.
    @param opts The options for the set_channelmetadata function.
    @return #PNR_STARTED on success, an error otherwise
  */
PUBNUB_EXTERN enum pubnub_res pubnub_set_channelmetadata_ex(pubnub_t* pb, char const* channel, struct pubnub_set_channelmetadata_opts opts);


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


/** Options for the pubnub_get_memberships function */
struct pubnub_membership_opts {
    /** The UUID to retrieve the memberships. */
    char const* uuid;

    /** The comma delimited (C) string with additional/complex attributes to include in response.
        Use NULL if you don't want to retrieve additional attributes. */
    char const* include;

    /** Expression used to filter the results. Only objects whose properties satisfy the given expression are returned. */
    char const* filter;

    /** Key-value pair of a property to sort by, and a sort direction. Available options are id, name, and updated.
        Use asc or desc to specify sort direction, or specify null to take the default sort direction (ascending).
        For example: {name: 'asc'} */
    char const* sort;

    /** Number of entities to return in response. Regular values 1 - 100. If you set `0`,
        that means “use the default”. At the time of this writing, default was 100. */
    size_t limit;

    /** Pagination object.
        @see pubnub_page_object */
    struct pubnub_page_object page;

    /** Request totalCount to be included in paginated response. By default, totalCount is omitted. */
    enum pubnub_tribool count;
};


/** Default options for the pubnub_get_memberships function 
    Values are set as follows:
    - uuid: NULL (`pubnub_*_memberships_ex` will take the user_id from the context on NULL)
    - include: NULL 
    - filter: NULL 
    - sort: NULL 
    - limit: 100
    - page: pubnub_null_page
    
    @see pubnub_null_page
    
    @return Default options for the get_memberships function
  */
PUBNUB_EXTERN struct pubnub_membership_opts pubnub_memberships_defopts();


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


/** Returns the channel memberships of the user specified by @p uuid_metadataid, optionally including
    the custom data objects for...
    
    Use `pubnub_memberships_defopts()` to get the default options.

    @see pubnub_memberships_defopts

    @param pb The pubnub context. Can't be NULL
    @param opts The options for the get_memberships function.
    @return #PNR_STARTED on success, an error otherwise
  */
PUBNUB_EXTERN enum pubnub_res pubnub_get_memberships_ex(pubnub_t* pb, struct pubnub_membership_opts opts);


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


/** Add/Update the channel memberships of the UUID specified by @p metadata_uuid. Uses the `set` property
    to perform those operations on one, or more memberships.
    An example for @channels:
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
    
    Use `pubnub_memberships_defopts()` to get the default options.

    @see pubnub_memberships_defopts

    @param pb The pubnub context. Can't be NULL
    @param channels The JSON object that defines the add/update to perform.
                      Cannot be NULL.
    @param opts The options for the set_memberships function.
    @return #PNR_STARTED on success, an error otherwise
  */
PUBNUB_EXTERN enum pubnub_res pubnub_set_memberships_ex(pubnub_t* pb, char const* channels, struct pubnub_membership_opts opts);


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


/** Removes the memberships of the user specified by @p uuid_metadataid. Uses the `delete` property
    to perform those operations on one, or more memberships.
    An example for @channels:
      [
         {
           "channel":{ "id": "main-channel-id" }
         },
         {
           "channel":{ "id": "channel-0" }
         }
       ]

    Use `pubnub_memberships_defopts()` to get the default options.

    @see pubnub_memberships_defopts

    @param pb The pubnub context. Can't be null
    @param channels The JSON object that defines the remove to perform.
                      Cannot be NULL.
    @param opts The options for the remove_memberships function.

    @return #PNR_STARTED on success, an error otherwise
    */
PUBNUB_EXTERN enum pubnub_res pubnub_remove_memberships_ex(pubnub_t* pb, char const* channels, struct pubnub_membership_opts opts);


/** Options for the pubnub_*_members functions */
struct pubnub_members_opts {
    /** The comma delimited (C) string with additional/complex attributes to include in response.
        Use NULL if you don't want to retrieve additional attributes. */
    char const* include;

    /** Expression used to filter the results. Only objects whose properties satisfy the given expression are returned. */
    char const* filter;

    /** Key-value pair of a property to sort by, and a sort direction. Available options are id, name, and updated.
        Use asc or desc to specify sort direction, or specify null to take the default sort direction (ascending).
        For example: {name: 'asc'} */
    char const* sort;

    /** Number of entities to return in response. Regular values 1 - 100. If you set `0`,
        that means “use the default”. At the time of this writing, default was 100. */
    size_t limit;

    /** Pagination object.
        @see pubnub_page_object */
    struct pubnub_page_object page;

    /** Request totalCount to be included in paginated response. By default, totalCount is omitted. */
    enum pubnub_tribool count;
};


/** Default options for the pubnub_get_members functions 
    Values are set as follows:
    - include: NULL 
    - filter: NULL 
    - sort: NULL 
    - limit: 100
    - page: pubnub_null_page
    - count: pbNotSet
    
    @see pubnub_null_page
    
    @return Default options for the get_members function
  */
PUBNUB_EXTERN struct pubnub_members_opts pubnub_members_defopts();


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


/** Returns all users in the channel specified with @p channel_metadataid, optionally including
    the custom data objects for...
    
    Use `pubnub_get_members_defopts()` to get the default options.

    @see pubnub_get_members_defopts

    @param pb The pubnub context. Can't be NULL
    @param channel The channel to retrieve the members for.
    @param opts The options for the get_members function.
    @return #PNR_STARTED on success, an error otherwise
  */
PUBNUB_EXTERN enum pubnub_res pubnub_get_members_ex(pubnub_t* pb, char const* channel, struct pubnub_members_opts opts);


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


/** Updates the list of members of the space specified with @p space_id. Uses the `update`
    property to perform the operation on one or more members.
    An example for @uuids:
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
    @param channel The channel to add/update the members.
    @param uuids The JSON object that defines the add/update to perform. Cannot be NULL.
    @param opts The options for the set_members function.
    @return #PNR_STARTED on success, an error otherwise
  */
PUBNUB_EXTERN enum pubnub_res pubnub_set_members_ex(pubnub_t* pb,
                                      char const* channel,
                                      char const* uuids,
                                      struct pubnub_members_opts opts);


/** Removes the list of members of the space specified with @p space_id. Uses the `remove`
    property to perform the operation on one or more members.
    An example for @update_obj:
      [
         {
           "uuid":{ "id": "main-user-id" }
         },
         {
           "uuid":{ "id": "user-0" }
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


/** Removes the list of members of the space specified with @p space_id. Uses the `remove`
    property to perform the operation on one or more members.
    An example for @uuids:
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
    @param channel The channel to remove the members.
    @param uuids The JSON object that defines the remove to perform. Cannot be NULL.
    @param opts The options for the remove_members function.
    @return #PNR_STARTED on success, an error otherwise
    */
PUBNUB_EXTERN enum pubnub_res pubnub_remove_members_ex(pubnub_t* pb,
                                      char const* channel,
                                      char const* uuids,
                                      struct pubnub_members_opts opts);


#endif /* !defined INC_PUBNUB_OBJECTS_API */

#endif /* PUBNUB_USE_OBJECTS_API */

