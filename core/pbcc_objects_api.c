/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#if PUBNUB_USE_OBJECTS_API

#include "pubnub_internal.h"
#include "pubnub_version.h"
#include "pubnub_assert.h"
#include "pubnub_url_encode.h"
#include "pubnub_helper.h"
#if PUBNUB_USE_LOGGER
#include "pbcc_logger_manager.h"
#endif // PUBNUB_USE_LOGGER
#include "lib/pb_strnlen_s.h"
#include "pbcc_objects_api.h"

#include "pbpal.h"
#include <stdio.h>
#include <stdlib.h>

/* Maximum number of objects to return in response */
#define MAX_OBJECTS_LIMIT 100
/** Maximum include string element length */
#define MAX_INCLUDE_ELEM_LENGTH 30


enum pubnub_res pbcc_find_objects_id(
    struct pbcc_context* pb,
    char const*          obj,
    struct pbjson_elem*  parsed_id,
    char const*          file,
    int                  line)
{
    struct pbjson_elem                   elem;
    enum pbjson_object_name_parse_result json_rslt;

    elem.end = pbjson_find_end_element(
        obj, obj + pb_strnlen_s(obj, PUBNUB_MAX_OBJECT_LENGTH));
    if ((*obj != '{') || (*(elem.end++) != '}')) {
        PBCC_LOG_ERROR(
            pb->logger_manager,
            "Object passed from %s:%d is not a JSON:\n  - object: %s",
            file,
            line,
            obj);

        return PNR_OBJECTS_API_INVALID_PARAM;
    }
    elem.start = obj;
    json_rslt  = pbjson_get_object_value(&elem, "id", parsed_id);
    if (json_rslt != jonmpOK) {
        PBCC_LOG_ERROR(
            pb->logger_manager,
            "'id' is missing in object passed from %s:%d:\n  - object: %s",
            file,
            line,
            obj);

        return PNR_OBJECTS_API_INVALID_PARAM;
    }
    if ((*parsed_id->start != '"') || (*(parsed_id->end - 1) != '"')) {
        PBCC_LOG_ERROR(
            pb->logger_manager,
            "'id' is not a string in object passed from %s:%d:\n  - object: %s",
            file,
            line,
            obj);

        return PNR_OBJECTS_API_INVALID_PARAM;
    }

    return PNR_OK;
}


enum pubnub_res pbcc_getall_uuidmetadata_prep(
    struct pbcc_context* pb,
    char const*          include,
    size_t               limit,
    char const*          start,
    char const*          end,
    enum pubnub_tribool  count,
    char const*          filter,
    char const*          sort,
    enum pubnub_trans    pt)
{
    char const* const uname   = pubnub_uname();
    char const*       user_id = pbcc_user_id_get(pb);
    enum pubnub_res   rslt    = PNR_OK;

    PUBNUB_ASSERT_OPT(user_id != NULL);
    PUBNUB_ASSERT_OPT(limit <= MAX_OBJECTS_LIMIT);

    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;
    pb->http_buf_len          = snprintf(
        pb->http_buf,
        sizeof pb->http_buf,
        "/v2/objects/%s/uuids",
        pb->subscribe_key);

    URL_PARAMS_INIT(qparam, PUBNUB_MAX_URL_PARAMS);
    if (uname) { ADD_URL_PARAM(qparam, pnsdk, uname); }

    if (limit != 0) { ADD_URL_PARAM_SIZET(pb, qparam, limit, limit); }
    if (start != NULL) { ADD_URL_PARAM(qparam, start, start); }
    if (end != NULL) { ADD_URL_PARAM(qparam, end, end); }

    if (count != pbccNotSet) {
        ADD_URL_PARAM(qparam, count, count == pbccTrue ? "true" : "false");
    }

    if (NULL != filter) { ADD_URL_PARAM(qparam, filter, filter); }
    if (NULL != sort) { ADD_URL_PARAM(qparam, sort, sort); }

    if (user_id) { ADD_URL_PARAM(qparam, uuid, user_id); }
#if PUBNUB_CRYPTO_API
    if (pb->secret_key == NULL) { ADD_URL_AUTH_PARAM(pb, qparam, auth); }
    ADD_TS_TO_URL_PARAM();
#else
    ADD_URL_AUTH_PARAM(pb, qparam, auth);
#endif

    if (include) { ADD_URL_PARAM(qparam, include, include); }

#if PUBNUB_CRYPTO_API
    SORT_URL_PARAMETERS(qparam);
#endif
    ENCODE_URL_PARAMS_TRANS(pt, pb, qparam);
#if PUBNUB_CRYPTO_API
    if (pb->secret_key != NULL) {
        rslt = pbcc_sign_url(pb, "", pubnubSendViaGET, true);
        if (rslt != PNR_OK) {
#if PUBNUB_LOG_ENABLED(ERROR)
            pbcc_logger_manager_log_error(
                pb->logger_manager,
                PUBNUB_LOG_LEVEL_ERROR,
                PUBNUB_LOG_LOCATION,
                rslt,
                "Get all UUID metadata URL signing failed",
                pubnub_res_2_string(rslt));
#endif // PUBNUB_LOG_ENABLED(ERROR)
        }
    }
#endif

    return (rslt != PNR_OK) ? rslt : PNR_STARTED;
}


enum pubnub_res pbcc_set_uuidmetadata_prep(
    struct pbcc_context* pb,
    char const*          uuid_metadataid,
    char const*          include,
    const char*          user_obj,
    enum pubnub_trans    pt)
{
    char const* const uname   = pubnub_uname();
    char const*       user_id = pbcc_user_id_get(pb);
    enum pubnub_res   rslt    = PNR_OK;

    PUBNUB_ASSERT_OPT(user_id != NULL);
    PUBNUB_ASSERT_OPT(user_obj != NULL);
    if (NULL == uuid_metadataid) { uuid_metadataid = user_id; }


    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;
    pb->http_buf_len          = snprintf(
        pb->http_buf,
        sizeof pb->http_buf,
        "/v2/objects/%s/uuids/",
        pb->subscribe_key);
    APPEND_URL_ENCODED_M(pb, uuid_metadataid);

    URL_PARAMS_INIT(qparam, PUBNUB_MAX_URL_PARAMS);
    if (uname) { ADD_URL_PARAM(qparam, pnsdk, uname); }
    if (user_id) { ADD_URL_PARAM(qparam, uuid, user_id); }
#if PUBNUB_CRYPTO_API
    if (pb->secret_key == NULL) { ADD_URL_AUTH_PARAM(pb, qparam, auth); }
    ADD_TS_TO_URL_PARAM();
#else
    ADD_URL_AUTH_PARAM(pb, qparam, auth);
#endif
    if (include) { ADD_URL_PARAM(qparam, include, include); }

#if PUBNUB_CRYPTO_API
    SORT_URL_PARAMETERS(qparam);
#endif
    ENCODE_URL_PARAMS_TRANS(pt, pb, qparam);
#if PUBNUB_CRYPTO_API
    if (pb->secret_key != NULL) {
        rslt = pbcc_sign_url(pb, user_obj, pubnubUsePATCH, true);
        if (rslt != PNR_OK) {
#if PUBNUB_LOG_ENABLED(ERROR)
            pbcc_logger_manager_log_error(
                pb->logger_manager,
                PUBNUB_LOG_LEVEL_ERROR,
                PUBNUB_LOG_LOCATION,
                rslt,
                "Set UUID metadata URL signing failed",
                pubnub_res_2_string(rslt));
#endif // PUBNUB_LOG_ENABLED(ERROR)
        }
    }
#endif

    APPEND_MESSAGE_BODY_M(rslt, pb, user_obj);

    return (rslt != PNR_OK) ? rslt : PNR_STARTED;
}


enum pubnub_res pbcc_get_uuidmetadata_prep(
    struct pbcc_context* pb,
    char const*          include,
    char const*          uuid_metadataid,
    enum pubnub_trans    pt)
{
    char const* const uname   = pubnub_uname();
    char const*       user_id = pbcc_user_id_get(pb);
    enum pubnub_res   rslt    = PNR_OK;

    PUBNUB_ASSERT_OPT(user_id != NULL);
    if (NULL == uuid_metadataid) { uuid_metadataid = user_id; }

    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;
    pb->http_buf_len          = snprintf(
        pb->http_buf,
        sizeof pb->http_buf,
        "/v2/objects/%s/uuids/",
        pb->subscribe_key);
    APPEND_URL_ENCODED_M(pb, uuid_metadataid);

    URL_PARAMS_INIT(qparam, PUBNUB_MAX_URL_PARAMS);
    if (uname) { ADD_URL_PARAM(qparam, pnsdk, uname); }
    if (user_id) { ADD_URL_PARAM(qparam, uuid, user_id); }
#if PUBNUB_CRYPTO_API
    if (pb->secret_key == NULL) { ADD_URL_AUTH_PARAM(pb, qparam, auth); }
    ADD_TS_TO_URL_PARAM();
#else
    ADD_URL_AUTH_PARAM(pb, qparam, auth);
#endif

    if (include) { ADD_URL_PARAM(qparam, include, include); }

#if PUBNUB_CRYPTO_API
    SORT_URL_PARAMETERS(qparam);
#endif
    ENCODE_URL_PARAMS_TRANS(pt, pb, qparam);
#if PUBNUB_CRYPTO_API
    if (pb->secret_key != NULL) {
        rslt = pbcc_sign_url(pb, "", pubnubSendViaGET, true);
        if (rslt != PNR_OK) {
#if PUBNUB_LOG_ENABLED(ERROR)
            pbcc_logger_manager_log_error(
                pb->logger_manager,
                PUBNUB_LOG_LEVEL_ERROR,
                PUBNUB_LOG_LOCATION,
                rslt,
                "Get UUID metadata URL signing failed",
                pubnub_res_2_string(rslt));
#endif // PUBNUB_LOG_ENABLED(ERROR)
        }
    }
#endif

    return (rslt != PNR_OK) ? rslt : PNR_STARTED;
}


enum pubnub_res pbcc_remove_uuidmetadata_prep(
    struct pbcc_context* pb,
    char const*          uuid_metadataid,
    enum pubnub_trans    pt)
{
    char const* const uname   = pubnub_uname();
    char const*       user_id = pbcc_user_id_get(pb);
    enum pubnub_res   rslt    = PNR_OK;

    PUBNUB_ASSERT_OPT(user_id != NULL);
    if (NULL == uuid_metadataid) { uuid_metadataid = user_id; }

    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;
    pb->http_buf_len          = snprintf(
        pb->http_buf,
        sizeof pb->http_buf,
        "/v2/objects/%s/uuids/",
        pb->subscribe_key);
    APPEND_URL_ENCODED_M(pb, uuid_metadataid);

    URL_PARAMS_INIT(qparam, PUBNUB_MAX_URL_PARAMS);
    if (uname) { ADD_URL_PARAM(qparam, pnsdk, uname); }
    if (user_id) { ADD_URL_PARAM(qparam, uuid, user_id); }
#if PUBNUB_CRYPTO_API
    if (pb->secret_key == NULL) { ADD_URL_AUTH_PARAM(pb, qparam, auth); }
    ADD_TS_TO_URL_PARAM();
#else
    ADD_URL_AUTH_PARAM(pb, qparam, auth);
#endif

#if PUBNUB_CRYPTO_API
    SORT_URL_PARAMETERS(qparam);
#endif
    ENCODE_URL_PARAMS_TRANS(pt, pb, qparam);
#if PUBNUB_CRYPTO_API
    if (pb->secret_key != NULL) {
        rslt = pbcc_sign_url(pb, "", pubnubUseDELETE, true);
        if (rslt != PNR_OK) {
#if PUBNUB_LOG_ENABLED(ERROR)
            pbcc_logger_manager_log_error(
                pb->logger_manager,
                PUBNUB_LOG_LEVEL_ERROR,
                PUBNUB_LOG_LOCATION,
                rslt,
                "Remove UUID metadata URL signing failed",
                pubnub_res_2_string(rslt));
#endif // PUBNUB_LOG_ENABLED(ERROR)
        }
    }
#endif

    return (rslt != PNR_OK) ? rslt : PNR_STARTED;
}


enum pubnub_res pbcc_getall_channelmetadata_prep(
    struct pbcc_context* pb,
    char const*          include,
    size_t               limit,
    char const*          start,
    char const*          end,
    enum pubnub_tribool  count,
    char const*          filter,
    char const*          sort,
    enum pubnub_trans    pt)
{
    char const* const uname   = pubnub_uname();
    char const*       user_id = pbcc_user_id_get(pb);
    enum pubnub_res   rslt    = PNR_OK;

    PUBNUB_ASSERT_OPT(user_id != NULL);
    PUBNUB_ASSERT_OPT(limit <= MAX_OBJECTS_LIMIT);

    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;
    pb->http_buf_len          = snprintf(
        pb->http_buf,
        sizeof pb->http_buf,
        "/v2/objects/%s/channels",
        pb->subscribe_key);
    URL_PARAMS_INIT(qparam, PUBNUB_MAX_URL_PARAMS);
    if (uname) { ADD_URL_PARAM(qparam, pnsdk, uname); }

    if (limit != 0) { ADD_URL_PARAM_SIZET(pb, qparam, limit, limit); }

    if (start != NULL) { ADD_URL_PARAM(qparam, start, start); }
    if (end != NULL) { ADD_URL_PARAM(qparam, end, end); }

    if (count != pbccNotSet) {
        ADD_URL_PARAM(qparam, count, count == pbccTrue ? "true" : "false");
    }

    if (NULL != filter) { ADD_URL_PARAM(qparam, filter, filter); }
    if (NULL != sort) { ADD_URL_PARAM(qparam, sort, sort); }

    if (user_id) { ADD_URL_PARAM(qparam, uuid, user_id); }
#if PUBNUB_CRYPTO_API
    if (pb->secret_key == NULL) { ADD_URL_AUTH_PARAM(pb, qparam, auth); }
    ADD_TS_TO_URL_PARAM();
#else
    ADD_URL_AUTH_PARAM(pb, qparam, auth);
#endif
    if (include) { ADD_URL_PARAM(qparam, include, include); }

#if PUBNUB_CRYPTO_API
    SORT_URL_PARAMETERS(qparam);
#endif
    ENCODE_URL_PARAMS_TRANS(pt, pb, qparam);
#if PUBNUB_CRYPTO_API
    if (pb->secret_key != NULL) {
        rslt = pbcc_sign_url(pb, "", pubnubSendViaGET, true);
        if (rslt != PNR_OK) {
#if PUBNUB_LOG_ENABLED(ERROR)
            pbcc_logger_manager_log_error(
                pb->logger_manager,
                PUBNUB_LOG_LEVEL_ERROR,
                PUBNUB_LOG_LOCATION,
                rslt,
                "Get all channel metadata URL signing failed",
                pubnub_res_2_string(rslt));
#endif // PUBNUB_LOG_ENABLED(ERROR)
        }
    }
#endif

    return (rslt != PNR_OK) ? rslt : PNR_STARTED;
}


enum pubnub_res pbcc_set_channelmetadata_prep(
    struct pbcc_context* pb,
    char const*          channel_metadataid,
    char const*          include,
    const char*          channel_metadata_obj,
    enum pubnub_trans    pt)
{
    char const* const uname   = pubnub_uname();
    char const*       user_id = pbcc_user_id_get(pb);
    enum pubnub_res   rslt    = PNR_OK;

    PUBNUB_ASSERT_OPT(user_id != NULL);
    PUBNUB_ASSERT_OPT(channel_metadata_obj != NULL);
    if (NULL == channel_metadataid) {
        PBCC_LOG_ERROR(pb->logger_manager, "'channel_metadataid' is NULL");

        return PNR_OBJECTS_API_INVALID_PARAM;
    }

    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;
    pb->http_buf_len          = snprintf(
        pb->http_buf,
        sizeof pb->http_buf,
        "/v2/objects/%s/channels/",
        pb->subscribe_key);
    APPEND_URL_ENCODED_M(pb, channel_metadataid);

    URL_PARAMS_INIT(qparam, PUBNUB_MAX_URL_PARAMS);
    if (uname) { ADD_URL_PARAM(qparam, pnsdk, uname); }
    if (user_id) { ADD_URL_PARAM(qparam, uuid, user_id); }
#if PUBNUB_CRYPTO_API
    if (pb->secret_key == NULL) { ADD_URL_AUTH_PARAM(pb, qparam, auth); }
    ADD_TS_TO_URL_PARAM();
#else
    ADD_URL_AUTH_PARAM(pb, qparam, auth);
#endif

    if (include) { ADD_URL_PARAM(qparam, include, include); }

#if PUBNUB_CRYPTO_API
    SORT_URL_PARAMETERS(qparam);
#endif
    ENCODE_URL_PARAMS_TRANS(pt, pb, qparam);
#if PUBNUB_CRYPTO_API
    if (pb->secret_key != NULL) {
        rslt = pbcc_sign_url(pb, channel_metadata_obj, pubnubUsePATCH, true);
        if (rslt != PNR_OK) {
#if PUBNUB_LOG_ENABLED(ERROR)
            pbcc_logger_manager_log_error(
                pb->logger_manager,
                PUBNUB_LOG_LEVEL_ERROR,
                PUBNUB_LOG_LOCATION,
                rslt,
                "Set channel metadata URL signing failed",
                pubnub_res_2_string(rslt));
#endif // PUBNUB_LOG_ENABLED(ERROR)
        }
    }
#endif

    APPEND_MESSAGE_BODY_M(rslt, pb, channel_metadata_obj);

    return (rslt != PNR_OK) ? rslt : PNR_STARTED;
}


enum pubnub_res pbcc_get_channelmetadata_prep(
    struct pbcc_context* pb,
    char const*          include,
    char const*          channel_metadataid,
    enum pubnub_trans    pt)
{
    char const* const uname   = pubnub_uname();
    char const*       user_id = pbcc_user_id_get(pb);
    enum pubnub_res   rslt    = PNR_OK;

    PUBNUB_ASSERT_OPT(user_id != NULL);
    if (NULL == channel_metadataid) {
        PBCC_LOG_ERROR(pb->logger_manager, "'channel_metadataid' is NULL");

        return PNR_OBJECTS_API_INVALID_PARAM;
    }

    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;
    pb->http_buf_len          = snprintf(
        pb->http_buf,
        sizeof pb->http_buf,
        "/v2/objects/%s/channels/",
        pb->subscribe_key);
    APPEND_URL_ENCODED_M(pb, channel_metadataid);

    URL_PARAMS_INIT(qparam, PUBNUB_MAX_URL_PARAMS);
    if (uname) { ADD_URL_PARAM(qparam, pnsdk, uname); }
    if (user_id) { ADD_URL_PARAM(qparam, uuid, user_id); }
#if PUBNUB_CRYPTO_API
    if (pb->secret_key == NULL) { ADD_URL_AUTH_PARAM(pb, qparam, auth); }
    ADD_TS_TO_URL_PARAM();
#else
    ADD_URL_AUTH_PARAM(pb, qparam, auth);
#endif

    if (include) { ADD_URL_PARAM(qparam, include, include); }

#if PUBNUB_CRYPTO_API
    SORT_URL_PARAMETERS(qparam);
#endif
    ENCODE_URL_PARAMS_TRANS(pt, pb, qparam);
#if PUBNUB_CRYPTO_API
    if (pb->secret_key != NULL) {
        rslt = pbcc_sign_url(pb, "", pubnubSendViaGET, true);
        if (rslt != PNR_OK) {
#if PUBNUB_LOG_ENABLED(ERROR)
            pbcc_logger_manager_log_error(
                pb->logger_manager,
                PUBNUB_LOG_LEVEL_ERROR,
                PUBNUB_LOG_LOCATION,
                rslt,
                "Get channel metadata URL signing failed",
                pubnub_res_2_string(rslt));
#endif // PUBNUB_LOG_ENABLED(ERROR)
        }
    }
#endif

    return (rslt != PNR_OK) ? rslt : PNR_STARTED;
}


enum pubnub_res pbcc_remove_channelmetadata_prep(
    struct pbcc_context* pb,
    char const*          channel_metadataid,
    enum pubnub_trans    pt)
{
    char const* const uname   = pubnub_uname();
    char const*       user_id = pbcc_user_id_get(pb);
    enum pubnub_res   rslt    = PNR_OK;

    PUBNUB_ASSERT_OPT(user_id != NULL);
    if (NULL == channel_metadataid) {
        PBCC_LOG_ERROR(pb->logger_manager, "'channel_metadataid' is NULL");

        return PNR_OBJECTS_API_INVALID_PARAM;
    }

    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;
    pb->http_buf_len          = snprintf(
        pb->http_buf,
        sizeof pb->http_buf,
        "/v2/objects/%s/channels/",
        pb->subscribe_key);
    APPEND_URL_ENCODED_M(pb, channel_metadataid);

    URL_PARAMS_INIT(qparam, PUBNUB_MAX_URL_PARAMS);
    if (uname) { ADD_URL_PARAM(qparam, pnsdk, uname); }
    if (user_id) { ADD_URL_PARAM(qparam, uuid, user_id); }
#if PUBNUB_CRYPTO_API
    if (pb->secret_key == NULL) { ADD_URL_AUTH_PARAM(pb, qparam, auth); }
    ADD_TS_TO_URL_PARAM();
#else
    ADD_URL_AUTH_PARAM(pb, qparam, auth);
#endif

#if PUBNUB_CRYPTO_API
    SORT_URL_PARAMETERS(qparam);
#endif
    ENCODE_URL_PARAMS_TRANS(pt, pb, qparam);
#if PUBNUB_CRYPTO_API
    if (pb->secret_key != NULL) {
        rslt = pbcc_sign_url(pb, "", pubnubUseDELETE, true);
        if (rslt != PNR_OK) {
#if PUBNUB_LOG_ENABLED(ERROR)
            pbcc_logger_manager_log_error(
                pb->logger_manager,
                PUBNUB_LOG_LEVEL_ERROR,
                PUBNUB_LOG_LOCATION,
                rslt,
                "Remove channel metadata URL signing failed",
                pubnub_res_2_string(rslt));
#endif // PUBNUB_LOG_ENABLED(ERROR)
        }
    }
#endif

    return (rslt != PNR_OK) ? rslt : PNR_STARTED;
}


enum pubnub_res pbcc_get_memberships_prep(
    struct pbcc_context* pb,
    char const*          uuid_metadataid,
    char const*          include,
    size_t               limit,
    char const*          start,
    char const*          end,
    enum pubnub_tribool  count,
    char const*          filter,
    char const*          sort,
    enum pubnub_trans    pt)
{
    char const* const uname   = pubnub_uname();
    char const*       user_id = pbcc_user_id_get(pb);
    enum pubnub_res   rslt    = PNR_OK;

    PUBNUB_ASSERT_OPT(user_id != NULL);
    PUBNUB_ASSERT_OPT(limit <= MAX_OBJECTS_LIMIT);

    if (NULL == uuid_metadataid) { uuid_metadataid = user_id; }

    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;
    pb->http_buf_len          = snprintf(
        pb->http_buf,
        sizeof pb->http_buf,
        "/v2/objects/%s/uuids/",
        pb->subscribe_key);
    APPEND_URL_ENCODED_M(pb, uuid_metadataid);
    APPEND_URL_LITERAL_M(pb, "/channels");

    URL_PARAMS_INIT(qparam, PUBNUB_MAX_URL_PARAMS);
    if (uname) { ADD_URL_PARAM(qparam, pnsdk, uname); }
    if (limit > 0) { ADD_URL_PARAM_SIZET(pb, qparam, limit, limit); }
    if (start) { ADD_URL_PARAM(qparam, start, start); }
    if (end) { ADD_URL_PARAM(qparam, end, end); }
    if (count != pbccNotSet) {
        ADD_URL_PARAM(qparam, count, count == pbccTrue ? "true" : "false");
    }

    if (NULL != filter) { ADD_URL_PARAM(qparam, filter, filter); }
    if (NULL != sort) { ADD_URL_PARAM(qparam, sort, sort); }

    if (user_id) { ADD_URL_PARAM(qparam, uuid, user_id); }
#if PUBNUB_CRYPTO_API
    if (pb->secret_key == NULL) { ADD_URL_AUTH_PARAM(pb, qparam, auth); }
    ADD_TS_TO_URL_PARAM();
#else
    ADD_URL_AUTH_PARAM(pb, qparam, auth);
#endif

    if (include) { ADD_URL_PARAM(qparam, include, include); }

#if PUBNUB_CRYPTO_API
    SORT_URL_PARAMETERS(qparam);
#endif
    ENCODE_URL_PARAMS_TRANS(pt, pb, qparam);
#if PUBNUB_CRYPTO_API
    if (pb->secret_key != NULL) {
        rslt = pbcc_sign_url(pb, "", pubnubSendViaGET, true);
        if (rslt != PNR_OK) {
#if PUBNUB_LOG_ENABLED(ERROR)
            pbcc_logger_manager_log_error(
                pb->logger_manager,
                PUBNUB_LOG_LEVEL_ERROR,
                PUBNUB_LOG_LOCATION,
                rslt,
                "Get memberships URL signing failed",
                pubnub_res_2_string(rslt));
#endif // PUBNUB_LOG_ENABLED(ERROR)
        }
    }
#endif

    return (rslt != PNR_OK) ? rslt : PNR_STARTED;
}


enum pubnub_res pbcc_set_memberships_prep(
    struct pbcc_context* pb,
    char const*          uuid_metadataid,
    char const*          include,
    char const*          set_obj,
    char const*          filter,
    char const*          sort,
    size_t               limit,
    char const*          start,
    char const*          end,
    enum pubnub_tribool  count,
    enum pubnub_trans    pt)
{
    char const* const uname   = pubnub_uname();
    char const*       user_id = pbcc_user_id_get(pb);
    enum pubnub_res   rslt    = PNR_OK;

    PUBNUB_ASSERT_OPT(user_id != NULL);
    PUBNUB_ASSERT_OPT(set_obj != NULL);

    if (NULL == uuid_metadataid) { uuid_metadataid = user_id; }

    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;
    pb->http_buf_len          = snprintf(
        pb->http_buf,
        sizeof pb->http_buf,
        "/v2/objects/%s/uuids/",
        pb->subscribe_key);
    APPEND_URL_ENCODED_M(pb, uuid_metadataid);
    APPEND_URL_LITERAL_M(pb, "/channels");

    URL_PARAMS_INIT(qparam, PUBNUB_MAX_URL_PARAMS);
    if (uname) { ADD_URL_PARAM(qparam, pnsdk, uname); }
    if (user_id) { ADD_URL_PARAM(qparam, uuid, user_id); }

    if (limit > 0) { ADD_URL_PARAM_SIZET(pb, qparam, limit, limit); }
    if (NULL != start) { ADD_URL_PARAM(qparam, start, start); }
    if (NULL != end) { ADD_URL_PARAM(qparam, end, end); }

    if (count != pbccNotSet) {
        ADD_URL_PARAM(qparam, count, count == pbccTrue ? "true" : "false");
    }

    if (NULL != filter) { ADD_URL_PARAM(qparam, filter, filter); }
    if (NULL != sort) { ADD_URL_PARAM(qparam, sort, sort); }

#if PUBNUB_CRYPTO_API
    if (pb->secret_key == NULL) { ADD_URL_AUTH_PARAM(pb, qparam, auth); }
    ADD_TS_TO_URL_PARAM();
#else
    ADD_URL_AUTH_PARAM(pb, qparam, auth);
#endif

    if (include) { ADD_URL_PARAM(qparam, include, include); }

#if PUBNUB_CRYPTO_API
    SORT_URL_PARAMETERS(qparam);
#endif
    ENCODE_URL_PARAMS_TRANS(pt, pb, qparam);
#if PUBNUB_CRYPTO_API
    if (pb->secret_key != NULL) {
        rslt = pbcc_sign_url(pb, set_obj, pubnubUsePATCH, true);
        if (rslt != PNR_OK) {
#if PUBNUB_LOG_ENABLED(ERROR)
            pbcc_logger_manager_log_error(
                pb->logger_manager,
                PUBNUB_LOG_LEVEL_ERROR,
                PUBNUB_LOG_LOCATION,
                rslt,
                "Set memberships URL signing failed",
                pubnub_res_2_string(rslt));
#endif // PUBNUB_LOG_ENABLED(ERROR)
        }
    }
#endif

    APPEND_MESSAGE_BODY_M(rslt, pb, set_obj);

    return (rslt != PNR_OK) ? rslt : PNR_STARTED;
}


enum pubnub_res pbcc_get_members_prep(
    struct pbcc_context* pb,
    char const*          channel_metadataid,
    char const*          include,
    size_t               limit,
    char const*          start,
    char const*          end,
    char const*          filter,
    char const*          sort,
    enum pubnub_tribool  count,
    enum pubnub_trans    pt)
{
    char const* const uname   = pubnub_uname();
    char const*       user_id = pbcc_user_id_get(pb);
    enum pubnub_res   rslt    = PNR_OK;

    PUBNUB_ASSERT_OPT(user_id != NULL);
    PUBNUB_ASSERT_OPT(limit <= MAX_OBJECTS_LIMIT);
    if (NULL == channel_metadataid) {
        PBCC_LOG_ERROR(pb->logger_manager, "'channel_metadataid' is NULL");

        return PNR_OBJECTS_API_INVALID_PARAM;
    }

    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;
    pb->http_buf_len          = snprintf(
        pb->http_buf,
        sizeof pb->http_buf,
        "/v2/objects/%s/channels/",
        pb->subscribe_key);
    APPEND_URL_ENCODED_M(pb, channel_metadataid);
    APPEND_URL_LITERAL_M(pb, "/uuids");

    URL_PARAMS_INIT(qparam, PUBNUB_MAX_URL_PARAMS);
    if (uname) { ADD_URL_PARAM(qparam, pnsdk, uname); }
    if (limit > 0) { ADD_URL_PARAM_SIZET(pb, qparam, limit, limit); }
    if (start) { ADD_URL_PARAM(qparam, start, start); }
    if (end) { ADD_URL_PARAM(qparam, end, end); }
    if (count != pbccNotSet) {
        ADD_URL_PARAM(qparam, count, count == pbccTrue ? "true" : "false");
    }

    if (NULL != filter) { ADD_URL_PARAM(qparam, filter, filter); }
    if (NULL != sort) { ADD_URL_PARAM(qparam, sort, sort); }

    if (user_id) { ADD_URL_PARAM(qparam, uuid, user_id); }
#if PUBNUB_CRYPTO_API
    if (pb->secret_key == NULL) { ADD_URL_AUTH_PARAM(pb, qparam, auth); }
    ADD_TS_TO_URL_PARAM();
#else
    ADD_URL_AUTH_PARAM(pb, qparam, auth);
#endif

    if (include) { ADD_URL_PARAM(qparam, include, include); }

#if PUBNUB_CRYPTO_API
    SORT_URL_PARAMETERS(qparam);
#endif
    ENCODE_URL_PARAMS_TRANS(pt, pb, qparam);
#if PUBNUB_CRYPTO_API
    if (pb->secret_key != NULL) {
        rslt = pbcc_sign_url(pb, "", pubnubSendViaGET, true);
        if (rslt != PNR_OK) {
#if PUBNUB_LOG_ENABLED(ERROR)
            pbcc_logger_manager_log_error(
                pb->logger_manager,
                PUBNUB_LOG_LEVEL_ERROR,
                PUBNUB_LOG_LOCATION,
                rslt,
                "Get members URL signing failed",
                pubnub_res_2_string(rslt));
#endif // PUBNUB_LOG_ENABLED(ERROR)
        }
    }
#endif

    return (rslt != PNR_OK) ? rslt : PNR_STARTED;
}


enum pubnub_res pbcc_set_members_prep(
    struct pbcc_context* pb,
    char const*          channel_metadataid,
    char const*          include,
    char const*          set_obj,
    char const*          filter,
    char const*          sort,
    size_t               limit,
    char const*          start,
    char const*          end,
    enum pubnub_tribool  count,
    enum pubnub_trans    pt)
{
    char const* const uname   = pubnub_uname();
    char const*       user_id = pbcc_user_id_get(pb);
    enum pubnub_res   rslt    = PNR_OK;

    PUBNUB_ASSERT_OPT(user_id != NULL);
    PUBNUB_ASSERT_OPT(set_obj != NULL);
    if (NULL == channel_metadataid) {
        PBCC_LOG_ERROR(pb->logger_manager, "'channel_metadataid' is NULL");

        return PNR_OBJECTS_API_INVALID_PARAM;
    }

    pb->http_content_len = 0;
    pb->msg_ofs = pb->msg_end = 0;
    pb->http_buf_len          = snprintf(
        pb->http_buf,
        sizeof pb->http_buf,
        "/v2/objects/%s/channels/",
        pb->subscribe_key);
    APPEND_URL_ENCODED_M(pb, channel_metadataid);
    APPEND_URL_LITERAL_M(pb, "/uuids");

    URL_PARAMS_INIT(qparam, PUBNUB_MAX_URL_PARAMS);
    if (uname) { ADD_URL_PARAM(qparam, pnsdk, uname); }
    if (user_id) { ADD_URL_PARAM(qparam, uuid, user_id); }

    if (limit > 0) { ADD_URL_PARAM_SIZET(pb, qparam, limit, limit); }
    if (NULL != start) { ADD_URL_PARAM(qparam, start, start); }
    if (NULL != end) { ADD_URL_PARAM(qparam, end, end); }

    if (count != pbccNotSet) {
        ADD_URL_PARAM(qparam, count, count == pbccTrue ? "true" : "false");
    }

    if (NULL != filter) { ADD_URL_PARAM(qparam, filter, filter); }
    if (NULL != sort) { ADD_URL_PARAM(qparam, sort, sort); }
#if PUBNUB_CRYPTO_API
    if (pb->secret_key == NULL) { ADD_URL_AUTH_PARAM(pb, qparam, auth); }
    ADD_TS_TO_URL_PARAM();
#else
    ADD_URL_AUTH_PARAM(pb, qparam, auth);
#endif

    if (include) { ADD_URL_PARAM(qparam, include, include); }

#if PUBNUB_CRYPTO_API
    SORT_URL_PARAMETERS(qparam);
#endif
    ENCODE_URL_PARAMS_TRANS(pt, pb, qparam);
#if PUBNUB_CRYPTO_API
    if (pb->secret_key != NULL) {
        rslt = pbcc_sign_url(pb, set_obj, pubnubUsePATCH, true);
        if (rslt != PNR_OK) {
#if PUBNUB_LOG_ENABLED(ERROR)
            pbcc_logger_manager_log_error(
                pb->logger_manager,
                PUBNUB_LOG_LEVEL_ERROR,
                PUBNUB_LOG_LOCATION,
                rslt,
                "Set members URL signing failed",
                pubnub_res_2_string(rslt));
#endif // PUBNUB_LOG_ENABLED(ERROR)
        }
    }
#endif

    APPEND_MESSAGE_BODY_M(rslt, pb, set_obj);

    return (rslt != PNR_OK) ? rslt : PNR_STARTED;
}


enum pubnub_res pbcc_parse_objects_api_response(struct pbcc_context* pb)
{
    char*                                reply    = pb->http_reply;
    int                                  replylen = pb->http_buf_len;
    struct pbjson_elem                   elem;
    struct pbjson_elem                   parsed;
    enum pbjson_object_name_parse_result json_rslt;

    PBCC_LOG_TRACE(pb->logger_manager, "Parsing objects service response...");

    if ((replylen < 2) || (reply[0] != '{')) { return PNR_FORMAT_ERROR; }

    pb->chan_ofs = pb->chan_end = 0;
    pb->msg_ofs                 = 0;
    pb->msg_end                 = replylen + 1;

    elem.end = pbjson_find_end_element(reply, reply + replylen);
    /* elem.end has to be just behind end curly brace */
    if ((*reply != '{') || (*(elem.end++) != '}')) {
        PBCC_LOG_ERROR(
            pb->logger_manager,
            "Malformed service response:\n  - response: %.*s",
            replylen,
            reply);

        return PNR_FORMAT_ERROR;
    }
    elem.start = reply;

    if (pbjson_value_for_field_found(&elem, "status", "403")) {
        PBCC_LOG_ERROR(
            pb->logger_manager,
            "Objects access denied:\n  - response: %.*s",
            replylen,
            reply);
        return PNR_ACCESS_DENIED;
    }

    json_rslt = pbjson_get_object_value(&elem, "data", &parsed);
    if (jonmpKeyNotFound == json_rslt) {
        json_rslt = pbjson_get_object_value(&elem, "error", &parsed);
        if (json_rslt != jonmpOK) {
            PBCC_LOG_ERROR(
                pb->logger_manager,
                "Malformed service response: missing 'error' field\n  - "
                "response: %.*s",
                replylen,
                reply);

            return PNR_FORMAT_ERROR;
        }
        PBCC_LOG_WARNING(
            pb->logger_manager,
            "Service returned error: %.*s",
            (int)(parsed.end - parsed.start),
            parsed.start);

        return PNR_OBJECTS_API_ERROR;
    }
    else if (json_rslt != jonmpOK) {
        PBCC_LOG_ERROR(
            pb->logger_manager,
            "Malformed service response: missing 'data' field\n  - response: "
            "%.*s",
            replylen,
            reply);

        return PNR_FORMAT_ERROR;
    }
    PBCC_LOG_TRACE(
        pb->logger_manager,
        "Parsed objects data: %.*s",
        (int)(parsed.end - parsed.start),
        parsed.start);

    return PNR_OK;
}

#endif /* PUBNUB_USE_OBJECTS_API */
