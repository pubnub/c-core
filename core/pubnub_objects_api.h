/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_OBJECTS_API
#define INC_PUBNUB_OBJECTS_API


#include "pubnub_api_types.h"

#include <stdbool.h>


/** Returns a paginated list of users associated with the subscription key of the context @p pbp,
    optionally including each record's custom data object.
    @param pb The pubnub context. Can't be NULL
    @param include array of (C) strings with additional/complex user attributes to include in
                   response. Use NULL if you don't want to retrieve additional attributes.
    @param include_count dimension of @p include. Set 0 if you don’t want to retrieve additional
                         attributes
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
enum pubnub_res pubnub_get_users(pubnub_t* pb, 
                                 char const** include, 
                                 size_t include_count,
                                 size_t limit,
                                 char const* start,
                                 char const* end,
                                 enum pubnub_tribool count);

/** Creates a user with the attributes specified in @p user_obj.
    Returns the created user object, optionally including the user's custom data object.
    @note User ID and name are required properties in the @p user_obj
    @param pb The pubnub context. Can't be NULL
    @param include array of (C) strings with additional/complex user attributes to include in
                   response. Use NULL if you don't want to retrieve additional attributes.
    @param include_count dimension of @p include. Set 0 if you don’t want to retrieve additional
                         attributes
    @param user_obj The JSON string with the definition of the User
                    Object to create.
    @return #PNR_STARTED on success, an error otherwise
  */
enum pubnub_res pubnub_create_user(pubnub_t* pb, 
                                   char const** include, 
                                   size_t include_count,
                                   char const* user_obj);


/** Returns the user object specified with @p user_id, optionally including the user's
    custom data object.
    @param pb The pubnub context. Can't be NULL
    @param include array of (C) strings with additional/complex user attributes to include in
                   response. Use NULL if you don't want to retrieve additional attributes.
    @param include_count dimension of @p include. Set 0 if you don’t want to retrieve additional
                         attributes
    @param user_id The User ID for which to retrieve the user object.
                   Cannot be NULL.
    @return #PNR_STARTED on success, an error otherwise
  */
enum pubnub_res pubnub_get_user(pubnub_t* pb,
                                char const** include, 
                                size_t include_count,
                                char const* user_id);


/** Updates the user object specified with the `id` key of the @p user_obj with any new
    information you provide. Returns the updated user object, optionally including
    the user's custom data object. 
    @param pb The pubnub context. Can't be NULL
    @param include array of (C) strings with additional/complex user attributes to include in
                   response. Use NULL if you don't want to retrieve additional attributes.
    @param include_count dimension of @p include. Set 0 if you don’t want to retrieve additional
                         attributes
    @param user_obj The JSON string with the description of the User
                    Object to update.
    @return #PNR_STARTED on success, an error otherwise
  */
enum pubnub_res pubnub_update_user(pubnub_t* pb, 
                                   char const** include,
                                   size_t include_count,
                                   char const* user_obj);


/** Deletes the user specified with @p user_id.
    @param pb The pubnub context. Can't be NULL
    @param user_id The User ID. Cannot be NULL.
    @return #PNR_STARTED on success, an error otherwise
  */
enum pubnub_res pubnub_delete_user(pubnub_t* pb, char const* user_id);


/** Returns the spaces associated with the subscriber key of the context @p pbp, optionally
    including each space's custom data object.
    @param pb The pubnub context. Can't be NULL
    @param include array of (C) strings with additional/complex attributes to include in response.
                   Use NULL if you don't want to retrieve additional attributes.
    @param include_count dimension of @p include. Set 0 if you don’t want to retrieve additional
                         attributes
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
enum pubnub_res pubnub_get_spaces(pubnub_t* pb, 
                                  char const** include, 
                                  size_t include_count,
                                  size_t limit,
                                  char const* start,
                                  char const* end,
                                  enum pubnub_tribool count);


/** Creates a space with the attributes specified in @p space_obj.
    Returns the created space object, optionally including its custom data object.
    @note Space ID and name are required properties of @p space_obj
    @param pb The pubnub context. Can't be NULL
    @param include array of (C) strings with additional/complex attributes to include in response.
                   Use NULL if you don't want to retrieve additional attributes.
    @param include_count dimension of @p include. Set 0 if you don’t want to retrieve additional
                         attributes
    @param space_obj The JSON string with the definition of the Space Object to create.
    @return #PNR_STARTED on success, an error otherwise
  */
enum pubnub_res pubnub_create_space(pubnub_t* pb, 
                                    char const** include, 
                                    size_t include_count,
                                    char const* space_obj);


/** Returns the space object specified with @p space_id, optionally including its custom
    data object.
    @param pb The pubnub context. Can't be NULL
    @param include array of (C) strings with additional/complex attributes to include in response.
                   Use NULL if you don't want to retrieve additional attributes.
    @param include_count dimension of @p include. Set 0 if you don’t want to retrieve additional
                         attributes
    @param space_id The Space ID for which to retrieve the space object. Cannot be NULL.
    @return #PNR_STARTED on success, an error otherwise
  */
enum pubnub_res pubnub_get_space(pubnub_t* pb,
                                 char const** include, 
                                 size_t include_count,
                                 char const* space_id);


/** Updates the space specified by the `id` property of the @p space_obj. Returns the space object,
    optionally including its custom data object. 
    @param pb The pubnub context. Can't be NULL
    @param include array of (C) strings with additional/complex attributes to include in response.
                   Use NULL if you don't want to retrieve additional attributes.
    @param include_count dimension of @p include. Set 0 if you don’t want to retrieve additional
                         attributes
    @param space_obj The JSON string with the description of the Space Object to update.
    @return #PNR_STARTED on success, an error otherwise
  */
enum pubnub_res pubnub_update_space(pubnub_t* pb, 
                                    char const** include,
                                    size_t include_count,
                                    char const* space_obj);


/** Deletes the space specified with @p space_id.
    @param pb The pubnub context. Can't be NULL
    @param space_id The Space ID. Cannot be NULL.
    @return #PNR_STARTED on success, an error otherwise
  */
enum pubnub_res pubnub_delete_space(pubnub_t* pb, char const* space_id);


/** Returns the space memberships of the user specified by @p user_id, optionally including
    the custom data objects for...
    @param pb The pubnub context. Can't be NULL
    @param user_id The User ID for which to retrieve the space memberships for.
                   Cannot be NULL.
    @param include array of (C) strings with additional/complex attributes to include in response.
                   Use NULL if you don't want to retrieve additional attributes.
    @param include_count dimension of @p include. Set 0 if you don’t want to retrieve additional
                         attributes
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
enum pubnub_res pubnub_get_memberships(pubnub_t* pb,
                                       char const* user_id,
                                       char const** include,
                                       size_t include_count,
                                       size_t limit,
                                       char const* start,
                                       char const* end,
                                       enum pubnub_tribool count);


/** Adds the space memberships of the user specified by @p user_id. Uses the `add` property
    to perform those operations on one, or more memberships.
    An example for @update_obj:
       [
         {
           "id": "main-space-id"
         },
         {
           "id": "space-0"
         }
       ]

    @param pb The pubnub context. Can't be NULL
    @param include array of (C) strings with additional/complex attributes to include in response.
                   Use NULL if you don't want to retrieve additional attributes.
    @param include_count dimension of @p include. Set 0 if you don’t want to retrieve additional
                         attributes
    @param update_obj The JSON object that defines the updates to perform.
                      Cannot be NULL.
    @return #PNR_STARTED on success, an error otherwise
  */
enum pubnub_res pubnub_join_spaces(pubnub_t* pb, 
                                   char const* user_id,
                                   char const** include,
                                   size_t include_count,
                                   char const* update_obj);


/** Updates the space memberships of the user specified by @p user_id. Uses the `update` property
    to perform those operations on one, or more memberships.
    An example for @update_obj:
       [
         {
           "id": "main-space-id",
           "custom": {
             "starred": true
           }
         },
         {
           "id": "space-0",
            "some_key": {
              "other_key": "other_value"
            }
         }
       ]

    @param pb The pubnub context. Can't be NULL
    @param include array of (C) strings with additional/complex attributes to include in response.
                   Use NULL if you don't want to retrieve additional attributes.
    @param include_count dimension of @p include. Set 0 if you don’t want to retrieve additional
                         attributes
    @param update_obj The JSON object that defines the updates to perform.
                      Cannot be NULL.
    @return #PNR_STARTED on success, an error otherwise
  */
enum pubnub_res pubnub_update_memberships(pubnub_t* pb, 
                                          char const* user_id,
                                          char const** include,
                                          size_t include_count,
                                          char const* update_obj);


/** Removes the space memberships of the user specified by @p user_id. Uses the `remove` property
    to perform those operations on one, or more memberships.
    An example for @update_obj:
      [
        {
          "id": "main-space-id",
          "custom": {
            "starred": true
          }
        },
        {
          "id": "space-0"
        }
      ]

    @param pb The pubnub context. Can't be NULL
    @param include array of (C) strings with additional/complex attributes to include in response.
                   Use NULL if you don't want to retrieve additional attributes.
    @param include_count dimension of @p include. Set 0 if you don’t want to retrieve additional
                         attributes
    @param update_obj The JSON object that defines the updates to perform.
                      Cannot be NULL.
    @return #PNR_STARTED on success, an error otherwise
  */
enum pubnub_res pubnub_leave_spaces(pubnub_t* pb, 
                                    char const* user_id,
                                    char const** include,
                                    size_t include_count,
                                    char const* update_obj);


/** Returns all users in the space specified with @p space_id, optionally including
    the custom data objects for...
    @param pb The pubnub context. Can't be NULL
    @param space_id The Space ID for which to retrieve the user object.
    @param include array of (C) strings with additional/complex attributes to include in response.
                   Use NULL if you don't want to retrieve additional attributes.
    @param include_count dimension of @p include. Set 0 if you don’t want to retrieve additional
                         attributes
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
enum pubnub_res pubnub_get_members(pubnub_t* pb,
                                   char const* space_id,
                                   char const** include,
                                   size_t include_count,
                                   size_t limit,
                                   char const* start,
                                   char const* end,
                                   enum pubnub_tribool count);


/** Adds the list of members of the space specified with @p space_id. Uses the `add`
    property to perform the operation on one or more members.
    An example for @update_obj:
       [
         {
           "id": "some-user-id"
         },
         {
           "id": "user-0-id"
         }
       ]

    @param pb The pubnub context. Can't be NULL
    @param include array of (C) strings with additional/complex attributes to include in response.
                   Use NULL if you don't want to retrieve additional attributes.
    @param include_count dimension of @p include. Set 0 if you don’t want to retrieve additional
                         attributes
    @param update_obj The JSON object that defines the updates to perform. Cannot be NULL.
    @return #PNR_STARTED on success, an error otherwise
  */
enum pubnub_res pubnub_add_members(pubnub_t* pb, 
                                   char const* space_id,
                                   char const** include,
                                   size_t include_count,
                                   char const* update_obj);


/** Updates the list of members of the space specified with @p space_id. Uses the `update`
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
           "id": "user-0-id",
            "some_key": {
              "other_key": "other_value"
            }
         }
       ]

    @param pb The pubnub context. Can't be NULL
    @param include array of (C) strings with additional/complex attributes to include in response.
                   Use NULL if you don't want to retrieve additional attributes.
    @param include_count dimension of @p include. Set 0 if you don’t want to retrieve additional
                         attributes
    @param update_obj The JSON object that defines the updates to perform. Cannot be NULL.
    @return #PNR_STARTED on success, an error otherwise
  */
enum pubnub_res pubnub_update_members(pubnub_t* pb, 
                                      char const* space_id,
                                      char const** include,
                                      size_t include_count,
                                      char const* update_obj);


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
    @param include array of (C) strings with additional/complex attributes to include in response.
                   Use NULL if you don't want to retrieve additional attributes.
    @param include_count dimension of @p include. Set 0 if you don’t want to retrieve additional
                         attributes
    @param update_obj The JSON object that defines the updates to perform. Cannot be NULL.
    @return #PNR_STARTED on success, an error otherwise
  */
enum pubnub_res pubnub_remove_members(pubnub_t* pb, 
                                      char const* space_id,
                                      char const** include,
                                      size_t include_count,
                                      char const* update_obj);


#endif /* !defined INC_PUBNUB_OBJECTS_API */
