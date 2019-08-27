/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"
#include "pubnub_version.h"
#include "pubnub_assert.h"
#include "pubnub_json_parse.h"
#include "pubnub_log.h"
#include "pubnub_url_encode.h"
#include "lib/pb_strnlen_s.h"
#include "pbcc_objects_api.h"

#include <stdio.h>
#include <stdlib.h>

/* Maximum number of objects to return in response */
#define MAX_OBJECTS_LIMIT 100
/** Maximum include string element length */ 
#define MAX_INCLUDE_ELEM_LENGTH 30

enum pubnub_res append_url_param_include(struct pbcc_context* pb,
                                         char const** include, 
                                         size_t include_count)
{
    unsigned i;
    if ((NULL == include) && (include_count != 0)) {
        PUBNUB_LOG_ERROR("append_url_param_include(pbcc=%p) - Invalid params: "
                         "include=NULL, include_count=%lu\n",
                         pb,
                         (unsigned long)include_count);

        return PNR_OBJECTS_API_INVALID_PARAM;
    }
    
    for (i = 0; i < include_count; i++) {
        size_t param_val_len;
        if (NULL == include[i]) {
            PUBNUB_LOG_ERROR("append_url_param_include(pbcc=%p) - Invalid params: "
                             "include[%u]=NULL, include_count=%lu\n",
                             pb,
                             i,
                             (unsigned long)include_count);
        
            return PNR_OBJECTS_API_INVALID_PARAM;
        }
        param_val_len = pb_strnlen_s(include[i], MAX_INCLUDE_ELEM_LENGTH);
        if ((pb->http_buf_len + 1 + param_val_len + 1) > sizeof pb->http_buf) {
            PUBNUB_LOG_ERROR("append_url_param_include(pbcc=%p) - Ran out of buffer while appending "
                             "include params : "
                             "include[%u]='%s', include_count=%lu\n",
                             pb,
                             i,
                             include[i],
                             (unsigned long)include_count);

            return PNR_TX_BUFF_TOO_SMALL;
        }
        if (0 == i) {
            APPEND_URL_PARAM_M(pb, "include", include[0], '&');
        }
        else {
            pb->http_buf_len += snprintf(pb->http_buf + pb->http_buf_len,
                                         sizeof pb->http_buf - pb->http_buf_len,
                                         "​,%s",
                                         include[i]);
        }
    }
    
    return PNR_OK;
}


enum pubnub_res pbcc_fetch_all_users_prep(struct pbcc_context* pb,
                                          char const** include, 
                                          size_t include_count,
                                          size_t limit,
                                          char const* start,
                                          char const* end,
                                          enum pubnub_tribool count)
{
    char const* const uname = pubnub_uname();
    char const*       uuid = pbcc_uuid_get(pb);
    enum pubnub_res   rslt;

    PUBNUB_ASSERT_OPT(limit > MAX_OBJECTS_LIMIT);

    pb->http_content_len = 0;
    pb->http_buf_len = snprintf(pb->http_buf,
                                sizeof pb->http_buf,
                                "​/v1​/objects​/%s/users",
                                pb->subscribe_key);
    APPEND_URL_PARAM_M(pb, "pnsdk", uname, '?');
    if (limit > 0) {
        APPEND_URL_PARAM_UNSIGNED_M(pb, "limit", (unsigned)limit, '&');
    }
    APPEND_URL_PARAM_M(pb, "start", start, '&');
    if (NULL == start) {
        APPEND_URL_PARAM_M(pb, "end", end, '&');
    }
    APPEND_URL_PARAM_TRIBOOL_M(pb, "count", count, '&');
    APPEND_URL_PARAM_M(pb, "uuid", uuid, '&');
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');
    rslt = append_url_param_include(pb, include, include_count);

    return (rslt != PNR_OK) ? rslt : PNR_STARTED;
}


enum pubnub_res pbcc_create_user_prep(struct pbcc_context* pb,
                                      char const** include, 
                                      size_t include_count,
                                      const char* user_obj)
{
    char const* const uname = pubnub_uname();
    char const*       uuid = pbcc_uuid_get(pb);
    enum pubnub_res   rslt;

    PUBNUB_ASSERT_OPT(user_obj != NULL);

    pb->http_content_len = 0;
    pb->http_buf_len = snprintf(pb->http_buf,
                                sizeof pb->http_buf,
                                "/v1​/objects​/%s/users",
                                pb->subscribe_key);
    APPEND_URL_PARAM_M(pb, "pnsdk", uname, '?');
    APPEND_URL_PARAM_M(pb, "uuid", uuid, '&');
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');

    rslt = append_url_param_include(pb, include, include_count);
    if (PNR_OK == rslt) {
        APPEND_MESSAGE_BODY_M(pb, user_obj);
    }

    return (rslt != PNR_OK) ? rslt : PNR_STARTED;
}


enum pubnub_res pbcc_fetch_user_prep(struct pbcc_context* pb,
                                     char const** include, 
                                     size_t include_count,
                                     char const* user_id)
{
    char const* const uname = pubnub_uname();
    char const*       uuid = pbcc_uuid_get(pb);
    enum pubnub_res   rslt;
    
    if (NULL == user_id) {
        PUBNUB_LOG_ERROR("pbcc_fetch_user_prep(pbcc=%p) - Invalid param: "
                         "user_id=NULL\n",
                         pb);

        return PNR_OBJECTS_API_INVALID_PARAM;
    }

    pb->http_content_len = 0;
    pb->http_buf_len = snprintf(pb->http_buf,
                                sizeof pb->http_buf,
                                "​/v1​/objects​/%s/users/%s",
                                pb->subscribe_key,
                                user_id);
    APPEND_URL_PARAM_M(pb, "pnsdk", uname, '?');
    APPEND_URL_PARAM_M(pb, "uuid", uuid, '&');
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');
    rslt = append_url_param_include(pb, include, include_count);

    return (rslt != PNR_OK) ? rslt : PNR_STARTED;
}


enum pubnub_res pbcc_update_user_prep(struct pbcc_context* pb,
                                      char const** include, 
                                      size_t include_count,
                                      char const* user_obj)
{
    char const* const  uname = pubnub_uname();
    char const*        uuid = pbcc_uuid_get(pb);
    enum pubnub_res    rslt;
    struct pbjson_elem elem;
    struct pbjson_elem parsed_id;
    enum pbjson_object_name_parse_result json_rslt;
    
    PUBNUB_ASSERT_OPT(user_obj != NULL);

    elem.end = pbjson_find_end_element(user_obj,
                                       user_obj + pb_strnlen_s(user_obj, PUBNUB_MAX_OBJECT_LENGTH));
    if ((*user_obj != '{') || (*elem.end != '}')) {
        PUBNUB_LOG_ERROR("pbcc_update_user_prep(pbcc=%p) - Invalid param: User object is not JSON - "
                         "user_obj='%s'\n",
                         pb,
                         user_obj);

        return PNR_OBJECTS_API_INVALID_PARAM;
    }
    elem.start = user_obj;
    json_rslt = pbjson_get_object_value(&elem, "id", &parsed_id);
    if (json_rslt != jonmpOK) {
        PUBNUB_LOG_ERROR("pbcc_update_user_prep(pbcc=%p) - Invalid param: "
                         "pbjson_get_object_value(\"id\") reported an error: %s\n",
                         pb,
                         pbjson_object_name_parse_result_2_string(json_rslt));

        return PNR_OBJECTS_API_INVALID_PARAM;
    }
    if ((*parsed_id.start != '"') || (*parsed_id.end != '"')) {
        PUBNUB_LOG_ERROR("pbcc_update_user_prep(pbcc=%p) - Invalid param: "
                         "'id' key value is not a string - id=%.*s\n",
                         pb,
                         (int)(parsed_id.end - parsed_id.start + 1),
                         parsed_id.start);

        return PNR_OBJECTS_API_INVALID_PARAM;
    }

    pb->http_content_len = 0;
    pb->http_buf_len = snprintf(pb->http_buf,
                                sizeof pb->http_buf,
                                "​/v1​/objects​/%s/users/%.*s",
                                pb->subscribe_key,
                                (int)(parsed_id.end - parsed_id.start - 2),
                                parsed_id.start + 1);
    APPEND_URL_PARAM_M(pb, "pnsdk", uname, '?');
    APPEND_URL_PARAM_M(pb, "uuid", uuid, '&');
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');

    rslt = append_url_param_include(pb, include, include_count);
    if (PNR_OK == rslt) {
        APPEND_MESSAGE_BODY_M(pb, user_obj);
    }

    return (rslt != PNR_OK) ? rslt : PNR_STARTED;
}


enum pubnub_res pbcc_delete_user_prep(struct pbcc_context* pb, char const* user_id)
{
    char const* const uname = pubnub_uname();
    char const*       uuid = pbcc_uuid_get(pb);
    
    if (NULL == user_id) {
        PUBNUB_LOG_ERROR("pbcc_delete_user_prep(pbcc=%p) - Invalid param: "
                         "user_id=NULL\n",
                         pb);

        return PNR_OBJECTS_API_INVALID_PARAM;
    }

    pb->http_content_len = 0;
    pb->http_buf_len = snprintf(pb->http_buf,
                                sizeof pb->http_buf,
                                "​/v1​/objects​/%s/users/%s",
                                pb->subscribe_key,
                                user_id);
    APPEND_URL_PARAM_M(pb, "pnsdk", uname, '?');
    APPEND_URL_PARAM_M(pb, "uuid", uuid, '&');
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');

    return PNR_STARTED;
}


enum pubnub_res pbcc_fetch_all_spaces_prep(struct pbcc_context* pb,
                                           char const** include, 
                                           size_t include_count,
                                           size_t limit,
                                           char const* start,
                                           char const* end,
                                           enum pubnub_tribool count)
{
    char const* const uname = pubnub_uname();
    char const*       uuid = pbcc_uuid_get(pb);
    enum pubnub_res   rslt;

    PUBNUB_ASSERT_OPT(limit > MAX_OBJECTS_LIMIT);

    pb->http_content_len = 0;
    pb->http_buf_len = snprintf(pb->http_buf,
                                sizeof pb->http_buf,
                                "​/v1​/objects​/%s/spaces",
                                pb->subscribe_key);
    APPEND_URL_PARAM_M(pb, "pnsdk", uname, '?');
    if (limit > 0) {
        APPEND_URL_PARAM_UNSIGNED_M(pb, "limit", (unsigned)limit, '&');
    }
    APPEND_URL_PARAM_M(pb, "start", start, '&');
    if (NULL == start) {
        APPEND_URL_PARAM_M(pb, "end", end, '&');
    }
    APPEND_URL_PARAM_TRIBOOL_M(pb, "count", count, '&');
    APPEND_URL_PARAM_M(pb, "uuid", uuid, '&');
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');
    rslt = append_url_param_include(pb, include, include_count);

    return (rslt != PNR_OK) ? rslt : PNR_STARTED;
}


enum pubnub_res pbcc_create_space_prep(struct pbcc_context* pb,
                                       char const** include, 
                                       size_t include_count,
                                       const char* space_obj)
{
    char const* const uname = pubnub_uname();
    char const*       uuid = pbcc_uuid_get(pb);
    enum pubnub_res   rslt;

    PUBNUB_ASSERT_OPT(space_obj != NULL);

    pb->http_content_len = 0;
    pb->http_buf_len = snprintf(pb->http_buf,
                                sizeof pb->http_buf,
                                "/v1​/objects​/%s/spaces",
                                pb->subscribe_key);
    APPEND_URL_PARAM_M(pb, "pnsdk", uname, '?');
    APPEND_URL_PARAM_M(pb, "uuid", uuid, '&');
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');

    rslt = append_url_param_include(pb, include, include_count);
    if (PNR_OK == rslt) {
        APPEND_MESSAGE_BODY_M(pb, space_obj);
    }

    return (rslt != PNR_OK) ? rslt : PNR_STARTED;
}
    

enum pubnub_res pbcc_fetch_space_prep(struct pbcc_context* pb,
                                      char const** include, 
                                      size_t include_count,
                                      char const* space_id)
{
    char const* const uname = pubnub_uname();
    char const*       uuid = pbcc_uuid_get(pb);
    enum pubnub_res   rslt;
    
    if (NULL == space_id) {
        PUBNUB_LOG_ERROR("pbcc_fetch_space_prep(pbcc=%p) - Invalid param: "
                         "space_id=NULL\n",
                         pb);

        return PNR_OBJECTS_API_INVALID_PARAM;
    }

    pb->http_content_len = 0;
    pb->http_buf_len = snprintf(pb->http_buf,
                                sizeof pb->http_buf,
                                "​/v1​/objects​/%s/spaces/%s",
                                pb->subscribe_key,
                                space_id);
    APPEND_URL_PARAM_M(pb, "pnsdk", uname, '?');
    APPEND_URL_PARAM_M(pb, "uuid", uuid, '&');
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');
    rslt = append_url_param_include(pb, include, include_count);

    return (rslt != PNR_OK) ? rslt : PNR_STARTED;
}


enum pubnub_res pbcc_update_space_prep(struct pbcc_context* pb,
                                       char const** include, 
                                       size_t include_count,
                                       char const* space_obj)
{
    char const* const  uname = pubnub_uname();
    char const*        uuid = pbcc_uuid_get(pb);
    enum pubnub_res    rslt;
    struct pbjson_elem elem;
    struct pbjson_elem parsed_id;
    enum pbjson_object_name_parse_result json_rslt;
    
    PUBNUB_ASSERT_OPT(space_obj != NULL);

    elem.end = pbjson_find_end_element(space_obj,
                                       space_obj + pb_strnlen_s(space_obj, PUBNUB_MAX_OBJECT_LENGTH));
    if ((*space_obj != '{') || (*elem.end != '}')) {
        PUBNUB_LOG_ERROR("pbcc_update_space_prep(pbcc=%p) - "
                         "Invalid param: Space object is not JSON - "
                         "space_obj='%s'\n",
                         pb,
                         space_obj);

        return PNR_OBJECTS_API_INVALID_PARAM;
    }
    elem.start = space_obj;
    json_rslt = pbjson_get_object_value(&elem, "id", &parsed_id);
    if (json_rslt != jonmpOK) {
        PUBNUB_LOG_ERROR("pbcc_update_space_prep(pbcc=%p) - Invalid param: "
                         "pbjson_get_object_value(\"id\") reported an error: %s\n",
                         pb,
                         pbjson_object_name_parse_result_2_string(json_rslt));

        return PNR_OBJECTS_API_INVALID_PARAM;
    }
    if ((*parsed_id.start != '"') || (*parsed_id.end != '"')) {
        PUBNUB_LOG_ERROR("pbcc_update_space_prep(pbcc=%p) - Invalid param: "
                         "'id' key value is not a string - id=%.*s\n",
                         pb,
                         (int)(parsed_id.end - parsed_id.start + 1),
                         parsed_id.start);

        return PNR_OBJECTS_API_INVALID_PARAM;
    }

    pb->http_content_len = 0;
    pb->http_buf_len = snprintf(pb->http_buf,
                                sizeof pb->http_buf,
                                "​/v1​/objects​/%s/spaces/%.*s",
                                pb->subscribe_key,
                                (int)(parsed_id.end - parsed_id.start - 2),
                                parsed_id.start + 1);
    APPEND_URL_PARAM_M(pb, "pnsdk", uname, '?');
    APPEND_URL_PARAM_M(pb, "uuid", uuid, '&');
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');

    rslt = append_url_param_include(pb, include, include_count);
    if (PNR_OK == rslt) {
        APPEND_MESSAGE_BODY_M(pb, space_obj);
    }

    return (rslt != PNR_OK) ? rslt : PNR_STARTED;
}


enum pubnub_res pbcc_delete_space_prep(struct pbcc_context* pb, char const* space_id)
{
    char const* const uname = pubnub_uname();
    char const*       uuid = pbcc_uuid_get(pb);
    
    if (NULL == space_id) {
        PUBNUB_LOG_ERROR("pbcc_delete_space_prep(pbcc=%p) - Invalid param: "
                         "space_id=NULL\n",
                         pb);

        return PNR_OBJECTS_API_INVALID_PARAM;
    }

    pb->http_content_len = 0;
    pb->http_buf_len = snprintf(pb->http_buf,
                                sizeof pb->http_buf,
                                "​/v1​/objects​/%s/spaces/%s",
                                pb->subscribe_key,
                                space_id);
    APPEND_URL_PARAM_M(pb, "pnsdk", uname, '?');
    APPEND_URL_PARAM_M(pb, "uuid", uuid, '&');
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');

    return PNR_STARTED;
}


enum pubnub_res pbcc_fetch_users_space_memberships_prep(struct pbcc_context* pb,
                                                        char const* user_id,
                                                        char const** include, 
                                                        size_t include_count,
                                                        size_t limit,
                                                        char const* start,
                                                        char const* end,
                                                        enum pubnub_tribool count)
{
    char const* const uname = pubnub_uname();
    char const*       uuid = pbcc_uuid_get(pb);
    enum pubnub_res   rslt;

    PUBNUB_ASSERT_OPT(limit > MAX_OBJECTS_LIMIT);

    pb->http_content_len = 0;
    pb->http_buf_len = snprintf(pb->http_buf,
                                sizeof pb->http_buf,
                                "​/v1​/objects​/%s/users/%s/spaces",
                                pb->subscribe_key,
                                user_id);
    APPEND_URL_PARAM_M(pb, "pnsdk", uname, '?');
    if (limit > 0) {
        APPEND_URL_PARAM_UNSIGNED_M(pb, "limit", (unsigned)limit, '&');
    }
    APPEND_URL_PARAM_M(pb, "start", start, '&');
    if (NULL == start) {
        APPEND_URL_PARAM_M(pb, "end", end, '&');
    }
    APPEND_URL_PARAM_TRIBOOL_M(pb, "count", count, '&');
    APPEND_URL_PARAM_M(pb, "uuid", uuid, '&');
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');
    rslt = append_url_param_include(pb, include, include_count);

    return (rslt != PNR_OK) ? rslt : PNR_STARTED;
}


enum pubnub_res pbcc_add_users_space_memberships_prep(struct pbcc_context* pb,
                                                      char const* user_id,
                                                      char const** include, 
                                                      size_t include_count,
                                                      char const* update_obj)
{
    char const* const  uname = pubnub_uname();
    char const*        uuid = pbcc_uuid_get(pb);
    enum pubnub_res    rslt;
    
    PUBNUB_ASSERT_OPT(update_obj != NULL);

    pb->http_content_len = 0;
    pb->http_buf_len = snprintf(pb->http_buf,
                                sizeof pb->http_buf,
                                "​/v1​/objects​/%s/users/%s/spaces",
                                pb->subscribe_key,
                                user_id);
    APPEND_URL_PARAM_M(pb, "pnsdk", uname, '?');
    APPEND_URL_PARAM_M(pb, "uuid", uuid, '&');
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');

    rslt = append_url_param_include(pb, include, include_count);
    if (PNR_OK == rslt) {
        APPEND_MESSAGE_BODY_M(pb, update_obj);
    }

    return (rslt != PNR_OK) ? rslt : PNR_STARTED;
}


enum pubnub_res pbcc_update_users_space_memberships_prep(struct pbcc_context* pb,
                                                         char const* user_id,
                                                         char const** include, 
                                                         size_t include_count,
                                                         char const* update_obj)
{
    char const* const  uname = pubnub_uname();
    char const*        uuid = pbcc_uuid_get(pb);
    enum pubnub_res    rslt;
    
    PUBNUB_ASSERT_OPT(update_obj != NULL);

    pb->http_content_len = 0;
    pb->http_buf_len = snprintf(pb->http_buf,
                                sizeof pb->http_buf,
                                "​/v1​/objects​/%s/users/%s/spaces",
                                pb->subscribe_key,
                                user_id);
    APPEND_URL_PARAM_M(pb, "pnsdk", uname, '?');
    APPEND_URL_PARAM_M(pb, "uuid", uuid, '&');
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');

    rslt = append_url_param_include(pb, include, include_count);
    if (PNR_OK == rslt) {
        APPEND_MESSAGE_BODY_M(pb, update_obj);
    }

    return (rslt != PNR_OK) ? rslt : PNR_STARTED;
}


enum pubnub_res pbcc_remove_users_space_memberships_prep(struct pbcc_context* pb,
                                                         char const* user_id,
                                                         char const** include, 
                                                         size_t include_count,
                                                         char const* update_obj)
{
    char const* const  uname = pubnub_uname();
    char const*        uuid = pbcc_uuid_get(pb);
    enum pubnub_res    rslt;
    
    PUBNUB_ASSERT_OPT(update_obj != NULL);

    pb->http_content_len = 0;
    pb->http_buf_len = snprintf(pb->http_buf,
                                sizeof pb->http_buf,
                                "​/v1​/objects​/%s/users/%s/spaces",
                                pb->subscribe_key,
                                user_id);
    APPEND_URL_PARAM_M(pb, "pnsdk", uname, '?');
    APPEND_URL_PARAM_M(pb, "uuid", uuid, '&');
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');

    rslt = append_url_param_include(pb, include, include_count);
    if (PNR_OK == rslt) {
        APPEND_MESSAGE_BODY_M(pb, update_obj);
    }

    return (rslt != PNR_OK) ? rslt : PNR_STARTED;
}


enum pubnub_res pbcc_fetch_members_in_space_prep(struct pbcc_context* pb,
                                                 char const* space_id,
                                                 char const** include, 
                                                 size_t include_count,
                                                 size_t limit,
                                                 char const* start,
                                                 char const* end,
                                                 enum pubnub_tribool count)
{
    char const* const uname = pubnub_uname();
    char const*       uuid = pbcc_uuid_get(pb);
    enum pubnub_res   rslt;

    PUBNUB_ASSERT_OPT(limit > MAX_OBJECTS_LIMIT);

    pb->http_content_len = 0;
    pb->http_buf_len = snprintf(pb->http_buf,
                                sizeof pb->http_buf,
                                "​/v1​/objects​/%s/spaces/%s/users",
                                pb->subscribe_key,
                                space_id);
    APPEND_URL_PARAM_M(pb, "pnsdk", uname, '?');
    if (limit > 0) {
        APPEND_URL_PARAM_UNSIGNED_M(pb, "limit", (unsigned)limit, '&');
    }
    APPEND_URL_PARAM_M(pb, "start", start, '&');
    if (NULL == start) {
        APPEND_URL_PARAM_M(pb, "end", end, '&');
    }
    APPEND_URL_PARAM_TRIBOOL_M(pb, "count", count, '&');
    APPEND_URL_PARAM_M(pb, "uuid", uuid, '&');
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');
    rslt = append_url_param_include(pb, include, include_count);

    return (rslt != PNR_OK) ? rslt : PNR_STARTED;
}


enum pubnub_res pbcc_update_members_in_space_prep(struct pbcc_context* pb,
                                                  char const* space_id,
                                                  char const** include, 
                                                  size_t include_count,
                                                  char const* update_obj)
{
    char const* const  uname = pubnub_uname();
    char const*        uuid = pbcc_uuid_get(pb);
    enum pubnub_res    rslt;
    
    PUBNUB_ASSERT_OPT(update_obj != NULL);

    pb->http_content_len = 0;
    pb->http_buf_len = snprintf(pb->http_buf,
                                sizeof pb->http_buf,
                                "​/v1​/objects​/%s/spaces/%s/users",
                                pb->subscribe_key,
                                space_id);
    APPEND_URL_PARAM_M(pb, "pnsdk", uname, '?');
    APPEND_URL_PARAM_M(pb, "uuid", uuid, '&');
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');

    rslt = append_url_param_include(pb, include, include_count);
    if (PNR_OK == rslt) {
        APPEND_MESSAGE_BODY_M(pb, update_obj);
    }

    return (rslt != PNR_OK) ? rslt : PNR_STARTED;
}


enum pubnub_res pbcc_parse_objects_api_response(struct pbcc_context* pb)
{
    char* reply    = pb->http_reply;
    int   replylen = pb->http_buf_len;
    struct pbjson_elem elem;
    struct pbjson_elem parsed_status;
    enum pbjson_object_name_parse_result json_rslt;

    if ((replylen < 2) || (reply[0] != '{')) {
        return PNR_FORMAT_ERROR;
    }

    pb->chan_ofs = pb->chan_end = 0;
    pb->msg_ofs = pb->msg_end = 0;

    elem.end = pbjson_find_end_element(reply, reply + replylen);
    if ((*reply != '{') || (*elem.end != '}')) {
        PUBNUB_LOG_ERROR("pbcc_parse_objects_api_response(pbcc=%p) - Invalid: "
                         "response from server is not JSON - response='%s'\n",
                         pb,
                         reply);

        return PNR_FORMAT_ERROR;
    }
    elem.start = reply;
    json_rslt = pbjson_get_object_value(&elem, "status", &parsed_status);
    if (json_rslt != jonmpOK) {
        PUBNUB_LOG_ERROR("pbcc_parse_objects_api_response(pbcc=%p) - Invalid response: "
                         "pbjson_get_object_value(\"status\") reported an error: %s\n",
                         pb,
                         pbjson_object_name_parse_result_2_string(json_rslt));

        return PNR_FORMAT_ERROR;
    }
    if (strncmp("\"ok\"", parsed_status.start, parsed_status.end - parsed_status.start) != 0) {
        return PNR_OBJECTS_API_ERROR;
    }

    return PNR_OBJECTS_API_OK;
}
