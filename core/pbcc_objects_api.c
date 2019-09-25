/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"
#include "pubnub_version.h"
#include "pubnub_assert.h"
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


enum pubnub_res pbcc_find_objects_id(struct pbcc_context* pb,
                                     char const* obj,
                                     struct pbjson_elem* parsed_id,
                                     char const* file,
                                     int line)
{
    struct pbjson_elem elem;
    enum pbjson_object_name_parse_result json_rslt;

    elem.end = pbjson_find_end_element(obj,
                                       obj + pb_strnlen_s(obj, PUBNUB_MAX_OBJECT_LENGTH));
    if ((*obj != '{') || (*(elem.end++) != '}')) {
        PUBNUB_LOG_ERROR("%s:%d: pbcc_find_objects_id(pbcc=%p) - "
                         "Invalid param: object is not JSON - "
                         "object='%s'\n",
                         file,
                         line,
                         pb,
                         obj);

        return PNR_OBJECTS_API_INVALID_PARAM;
    }
    elem.start = obj;
    json_rslt = pbjson_get_object_value(&elem, "id", parsed_id);
    if (json_rslt != jonmpOK) {
        PUBNUB_LOG_ERROR("%s:%d: pbcc_find_objects_id(pbcc=%p) - Invalid param: "
                         "pbjson_get_object_value(\"id\") reported an error: %s\n",
                         file,
                         line,
                         pb,
                         pbjson_object_name_parse_result_2_string(json_rslt));

        return PNR_OBJECTS_API_INVALID_PARAM;
    }
    if ((*parsed_id->start != '"') || (*(parsed_id->end - 1) != '"')) {
        PUBNUB_LOG_ERROR("%s:%d: pbcc_find_objects_id(pbcc=%p) - Invalid param: "
                         "'id' key value is not a string - id='%.*s'\n",
                         file,
                         line,
                         pb,
                         (int)(parsed_id->end - parsed_id->start),
                         parsed_id->start);

        return PNR_OBJECTS_API_INVALID_PARAM;
    }

    return PNR_OK;
}


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


enum pubnub_res pbcc_get_users_prep(struct pbcc_context* pb,
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

    PUBNUB_ASSERT_OPT(limit < MAX_OBJECTS_LIMIT);

    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;
    pb->http_buf_len = snprintf(pb->http_buf,
                                sizeof pb->http_buf,
                                "/v1/objects/%s/users",
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
    pb->msg_ofs = pb->msg_end = 0;
    pb->http_buf_len = snprintf(pb->http_buf,
                                sizeof pb->http_buf,
                                "/v1/objects/%s/users",
                                pb->subscribe_key);
    APPEND_URL_PARAM_M(pb, "pnsdk", uname, '?');
    APPEND_URL_PARAM_M(pb, "uuid", uuid, '&');
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');

    rslt = append_url_param_include(pb, include, include_count);
    APPEND_MESSAGE_BODY_M(rslt, pb, user_obj);

    return (rslt != PNR_OK) ? rslt : PNR_STARTED;
}


enum pubnub_res pbcc_get_user_prep(struct pbcc_context* pb,
                                   char const** include,
                                   size_t include_count,
                                   char const* user_id)
{
    char const* const uname = pubnub_uname();
    char const*       uuid = pbcc_uuid_get(pb);
    enum pubnub_res   rslt;

    if (NULL == user_id) {
        PUBNUB_LOG_ERROR("pbcc_get_user_prep(pbcc=%p) - Invalid param: "
                         "user_id=NULL\n",
                         pb);

        return PNR_OBJECTS_API_INVALID_PARAM;
    }

    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;
    pb->http_buf_len = snprintf(pb->http_buf,
                                sizeof pb->http_buf,
                                "/v1/objects/%s/users/%s",
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
                                      char const* user_obj,
                                      struct pbjson_elem const* id)
{
    char const* const  uname = pubnub_uname();
    char const*        uuid = pbcc_uuid_get(pb);
    enum pubnub_res    rslt;

    PUBNUB_ASSERT_OPT(user_obj != NULL);

    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;
    pb->http_buf_len = snprintf(pb->http_buf,
                                sizeof pb->http_buf,
                                "/v1/objects/%s/users/%.*s",
                                pb->subscribe_key,
                                (int)(id->end - id->start - 2),
                                id->start + 1);
    APPEND_URL_PARAM_M(pb, "pnsdk", uname, '?');
    APPEND_URL_PARAM_M(pb, "uuid", uuid, '&');
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');

    rslt = append_url_param_include(pb, include, include_count);
    APPEND_MESSAGE_BODY_M(rslt, pb, user_obj);

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
    pb->msg_ofs = pb->msg_end = 0;
    pb->http_buf_len = snprintf(pb->http_buf,
                                sizeof pb->http_buf,
                                "/v1/objects/%s/users/%s",
                                pb->subscribe_key,
                                user_id);
    APPEND_URL_PARAM_M(pb, "pnsdk", uname, '?');
    APPEND_URL_PARAM_M(pb, "uuid", uuid, '&');
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');

    return PNR_STARTED;
}


enum pubnub_res pbcc_get_spaces_prep(struct pbcc_context* pb,
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

    PUBNUB_ASSERT_OPT(limit < MAX_OBJECTS_LIMIT);

    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;
    pb->http_buf_len = snprintf(pb->http_buf,
                                sizeof pb->http_buf,
                                "/v1/objects/%s/spaces",
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
    pb->msg_ofs = pb->msg_end = 0;
    pb->http_buf_len = snprintf(pb->http_buf,
                                sizeof pb->http_buf,
                                "/v1/objects/%s/spaces",
                                pb->subscribe_key);
    APPEND_URL_PARAM_M(pb, "pnsdk", uname, '?');
    APPEND_URL_PARAM_M(pb, "uuid", uuid, '&');
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');

    rslt = append_url_param_include(pb, include, include_count);
    APPEND_MESSAGE_BODY_M(rslt, pb, space_obj);

    return (rslt != PNR_OK) ? rslt : PNR_STARTED;
}


enum pubnub_res pbcc_get_space_prep(struct pbcc_context* pb,
                                    char const** include,
                                    size_t include_count,
                                    char const* space_id)
{
    char const* const uname = pubnub_uname();
    char const*       uuid = pbcc_uuid_get(pb);
    enum pubnub_res   rslt;

    if (NULL == space_id) {
        PUBNUB_LOG_ERROR("pbcc_get_space_prep(pbcc=%p) - Invalid param: "
                         "space_id=NULL\n",
                         pb);

        return PNR_OBJECTS_API_INVALID_PARAM;
    }

    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;
    pb->http_buf_len = snprintf(pb->http_buf,
                                sizeof pb->http_buf,
                                "/v1/objects/%s/spaces/%s",
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
                                       char const* space_obj,
                                       struct pbjson_elem const* id)
{
    char const* const  uname = pubnub_uname();
    char const*        uuid = pbcc_uuid_get(pb);
    enum pubnub_res    rslt;

    PUBNUB_ASSERT_OPT(space_obj != NULL);

    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;
    pb->http_buf_len = snprintf(pb->http_buf,
                                sizeof pb->http_buf,
                                "/v1/objects/%s/spaces/%.*s",
                                pb->subscribe_key,
                                (int)(id->end - id->start - 2),
                                id->start + 1);
    APPEND_URL_PARAM_M(pb, "pnsdk", uname, '?');
    APPEND_URL_PARAM_M(pb, "uuid", uuid, '&');
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');

    rslt = append_url_param_include(pb, include, include_count);
    APPEND_MESSAGE_BODY_M(rslt, pb, space_obj);

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
    pb->msg_ofs = pb->msg_end = 0;
    pb->http_buf_len = snprintf(pb->http_buf,
                                sizeof pb->http_buf,
                                "/v1/objects/%s/spaces/%s",
                                pb->subscribe_key,
                                space_id);
    APPEND_URL_PARAM_M(pb, "pnsdk", uname, '?');
    APPEND_URL_PARAM_M(pb, "uuid", uuid, '&');
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');

    return PNR_STARTED;
}


enum pubnub_res pbcc_get_memberships_prep(struct pbcc_context* pb,
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

    PUBNUB_ASSERT_OPT(limit < MAX_OBJECTS_LIMIT);

    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;
    pb->http_buf_len = snprintf(pb->http_buf,
                                sizeof pb->http_buf,
                                "/v1/objects/%s/users/%s/spaces",
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


enum pubnub_res pbcc_update_memberships_prep(struct pbcc_context* pb,
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
    pb->msg_ofs = pb->msg_end = 0;
    pb->http_buf_len = snprintf(pb->http_buf,
                                sizeof pb->http_buf,
                                "/v1/objects/%s/users/%s/spaces",
                                pb->subscribe_key,
                                user_id);
    APPEND_URL_PARAM_M(pb, "pnsdk", uname, '?');
    APPEND_URL_PARAM_M(pb, "uuid", uuid, '&');
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');

    rslt = append_url_param_include(pb, include, include_count);
    APPEND_MESSAGE_BODY_M(rslt, pb, update_obj);

    return (rslt != PNR_OK) ? rslt : PNR_STARTED;
}


enum pubnub_res pbcc_get_members_prep(struct pbcc_context* pb,
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

    PUBNUB_ASSERT_OPT(limit < MAX_OBJECTS_LIMIT);

    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;
    pb->http_buf_len = snprintf(pb->http_buf,
                                sizeof pb->http_buf,
                                "/v1/objects/%s/spaces/%s/users",
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


enum pubnub_res pbcc_update_members_prep(struct pbcc_context* pb,
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
    pb->msg_ofs = pb->msg_end = 0;
    pb->http_buf_len = snprintf(pb->http_buf,
                                sizeof pb->http_buf,
                                "/v1/objects/%s/spaces/%s/users",
                                pb->subscribe_key,
                                space_id);
    APPEND_URL_PARAM_M(pb, "pnsdk", uname, '?');
    APPEND_URL_PARAM_M(pb, "uuid", uuid, '&');
    APPEND_URL_PARAM_M(pb, "auth", pb->auth, '&');

    rslt = append_url_param_include(pb, include, include_count);
    APPEND_MESSAGE_BODY_M(rslt, pb, update_obj);

    return (rslt != PNR_OK) ? rslt : PNR_STARTED;
}


enum pubnub_res pbcc_parse_objects_api_response(struct pbcc_context* pb)
{
    char* reply    = pb->http_reply;
    int   replylen = pb->http_buf_len;
    struct pbjson_elem elem;
    struct pbjson_elem parsed;
    enum pbjson_object_name_parse_result json_rslt;

    if ((replylen < 2) || (reply[0] != '{')) {
        return PNR_FORMAT_ERROR;
    }

    pb->chan_ofs = pb->chan_end = 0;
    pb->msg_ofs = 0;
    pb->msg_end = replylen + 1;

    elem.end = pbjson_find_end_element(reply, reply + replylen);
    /* elem.end has to be just behind end curly brace */
    if ((*reply != '{') || (*(elem.end++) != '}')) {
        PUBNUB_LOG_ERROR("pbcc_parse_objects_api_response(pbcc=%p) - Invalid: "
                         "response from server is not JSON - response='%s'\n",
                         pb,
                         reply);

        return PNR_FORMAT_ERROR;
    }
    elem.start = reply;
    json_rslt = pbjson_get_object_value(&elem, "data", &parsed);
    if (jonmpKeyNotFound == json_rslt) {
        json_rslt = pbjson_get_object_value(&elem, "error", &parsed);
        if (json_rslt != jonmpOK) {
            PUBNUB_LOG_ERROR("pbcc_parse_objects_api_response(pbcc=%p) - Invalid response: "
                             "pbjson_get_object_value(\"error\") reported an error: %s\n",
                             pb,
                             pbjson_object_name_parse_result_2_string(json_rslt));

            return PNR_FORMAT_ERROR;
        }
        PUBNUB_LOG_WARNING("pbcc_parse_objects_api_response(pbcc=%p): \"error\"='%.*s'\n",
                           pb,
                           (int)(parsed.end - parsed.start),
                           parsed.start);

        return PNR_OBJECTS_API_ERROR;
    }
    else if (json_rslt != jonmpOK) {
        PUBNUB_LOG_ERROR("pbcc_parse_objects_api_response(pbcc=%p) - Invalid response: "
                         "pbjson_get_object_value(\"data\") reported an error: %s\n",
                         pb,
                         pbjson_object_name_parse_result_2_string(json_rslt));

        return PNR_FORMAT_ERROR;
    }
    PUBNUB_LOG_TRACE("pbcc_parse_objects_api_response(pbcc=%p): \"data\"='%.*s'\n",
                     pb,
                     (int)(parsed.end - parsed.start),
                     parsed.start);

    return PNR_OK;
}
