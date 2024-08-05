/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#if PUBNUB_USE_OBJECTS_API

#include "pubnub_internal.h"

#include "core/pubnub_ccore.h"
#include "core/pubnub_netcore.h"
#include "core/pubnub_assert.h"
#include "core/pubnub_timers.h"
#include "core/pubnub_log.h"
#include "lib/pb_strnlen_s.h"
#include "core/pbcc_objects_api.h"
#include "core/pubnub_objects_api.h"

#include "core/pbpal.h"

#include <ctype.h>
#include <string.h>


#define FORM_THE_OBJECT(pbcc, monitor, function_name_literal, obj_buffer, key_literal, json) \
do {                                                                              \
    if (sizeof(obj_buffer) <                                                      \
        sizeof(key_literal) + pb_strnlen_s(json, PUBNUB_MAX_OBJECT_LENGTH) + 1) { \
        PUBNUB_LOG_ERROR(function_name_literal "(pbcc=%p) - "                     \
                         "buffer size is too small: "                             \
                         "current_buffer_size = %lu\n"                            \
                         "required_buffer_size = %lu\n",                          \
                         (pbcc),                                                  \
                         (unsigned long)sizeof(obj_buffer),                       \
                         (unsigned long)(sizeof(key_literal) +                    \
                                         pb_strnlen_s(json, PUBNUB_MAX_OBJECT_LENGTH))); \
        pubnub_mutex_unlock(monitor);                                             \
        return PNR_TX_BUFF_TOO_SMALL;                                             \
    }                                                                             \
    snprintf(obj_buffer, sizeof(obj_buffer), "%s%s%c", key_literal, json, '}');   \
    json = (obj_buffer);                                                          \
} while(0)

const struct pubnub_page_object pubnub_null_page = {NULL, NULL};

const struct pubnub_user_data pubnub_null_user_data = {NULL, NULL, NULL, NULL, NULL};

const struct pubnub_channel_data pubnub_null_channel_data = {NULL, NULL, NULL};


struct pubnub_getall_metadata_opts pubnub_getall_metadata_defopts(void)
{
    struct pubnub_getall_metadata_opts opts;
    opts.include = NULL;
    opts.filter = NULL;
    opts.sort = NULL;
    opts.limit = 100;
    opts.count = pbccNotSet;
    opts.page = pubnub_null_page;

    return opts;
}


enum pubnub_res pubnub_getall_uuidmetadata(pubnub_t* pb, 
                                 char const* include, 
                                 size_t limit,
                                 char const* start,
                                 char const* end,
                                 enum pubnub_tribool count)
{
    struct pubnub_getall_metadata_opts opts = pubnub_getall_metadata_defopts();
    opts.include = include;
    opts.limit = limit;
    opts.page.next = start;
    opts.page.prev = end;
    opts.count = count;

    return pubnub_getall_uuidmetadata_ex(pb, opts);
}


enum pubnub_res pubnub_getall_uuidmetadata_ex(pubnub_t* pb, struct pubnub_getall_metadata_opts opts)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }
    pb->trans = PBTT_GETALL_UUIDMETADATA;
    rslt = pbcc_getall_uuidmetadata_prep(&pb->core,
                               opts.include, 
                               opts.limit,
                               opts.page.next,
                               opts.page.prev,
                               opts.count, 
                               opts.filter,
                               opts.sort,
                               pb->trans);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_GETALL_UUIDMETADATA;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}


struct pubnub_set_uuidmetadata_opts pubnub_set_uuidmetadata_defopts(void)
{
    struct pubnub_set_uuidmetadata_opts opts;
    opts.uuid = NULL;
    opts.include = NULL;
    opts.data = pubnub_null_user_data;

    return opts;
}


enum pubnub_res pubnub_set_uuidmetadata(pubnub_t* pb,
    char const* uuid_metadataid,
    char const* include,
    char const* uuid_metadata_obj)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }
    
    pb->method = pubnubUsePATCH;
    pb->trans = PBTT_SET_UUIDMETADATA;
    rslt = pbcc_set_uuidmetadata_prep(&pb->core,
            uuid_metadataid,
            include,
            uuid_metadata_obj,
            pb->trans);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_SET_UUIDMETADATA;
        pb->core.last_result = PNR_STARTED;
        pb->method           = pubnubUsePATCH;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}


enum pubnub_res pubnub_set_uuidmetadata_ex(pubnub_t* pb,
                                           struct pubnub_set_uuidmetadata_opts opts)
{
    char*           obj_buffer = NULL;
    enum pubnub_res result;
    size_t          obj_len = 0;

    obj_len = NULL != opts.data.custom ? strlen(opts.data.custom) : 0;
    obj_len += NULL != opts.data.email ? strlen(opts.data.email) : 0;
    obj_len += NULL != opts.data.name ? strlen(opts.data.name) : 0;
    obj_len += NULL != opts.data.external_id ? strlen(opts.data.external_id) : 0;
    obj_len += NULL != opts.data.profile_url ? strlen(opts.data.profile_url) : 0;

    obj_buffer = (char*)malloc(obj_len + 64); // 64 is for the JSON object structure with small buffer
    int offset = snprintf(obj_buffer, obj_len, "{");

    // TODO: maybe it will be a good idea to add serialization at some point to this SDK
    if (NULL != opts.data.custom) {
        offset += snprintf(obj_buffer + offset,
                           obj_len - offset,
                           "\"custom\":%s",
                           opts.data.custom);
    }

    if (NULL != opts.data.email) {
        offset += snprintf(obj_buffer + offset,
                           obj_len - offset,
                           "%s\"email\":\"%s\"",
                           offset > 1 ? "," : "",
                           opts.data.email);
    }

    if (NULL != opts.data.name) {
        offset += snprintf(obj_buffer + offset,
                           obj_len - offset,
                           "%s\"name\":\"%s\"",
                           offset > 1 ? "," : "",
                           opts.data.name);
    }

    if (NULL != opts.data.external_id) {
        offset += snprintf(obj_buffer + offset,
                           obj_len - offset,
                           "%s\"externalId\":\"%s\"",
                           offset > 1 ? "," : "",
                           opts.data.external_id);
    }

    if (NULL != opts.data.profile_url) {
        offset += snprintf(obj_buffer + offset,
                           obj_len - offset,
                           "%s\"profileUrl\":\"%s\"",
                           offset > 1 ? "," : "",
                           opts.data.profile_url);
    }
    snprintf(obj_buffer + offset, obj_len - offset, "}");

    result = pubnub_set_uuidmetadata(pb, opts.uuid, opts.include, obj_buffer);
    free(obj_buffer);

    return result;
}



enum pubnub_res pubnub_get_uuidmetadata(pubnub_t* pb,
                                char const* include, 
                                char const* uuid_metadataid)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }
    pb->trans = PBTT_GET_UUIDMETADATA;
    rslt = pbcc_get_uuidmetadata_prep(&pb->core, include, uuid_metadataid, pb->trans);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_GET_UUIDMETADATA;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}


enum pubnub_res pubnub_remove_uuidmetadata(pubnub_t* pb, char const* uuid_metadataid)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }
    pb->method = pubnubUseDELETE;
    pb->trans = PBTT_DELETE_UUIDMETADATA;
    rslt = pbcc_remove_uuidmetadata_prep(&pb->core, uuid_metadataid, pb->trans);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_DELETE_UUIDMETADATA;
        pb->core.last_result = PNR_STARTED;
        pb->method           = pubnubUseDELETE;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}


enum pubnub_res pubnub_getall_channelmetadata(pubnub_t* pb, 
                                  char const* include, 
                                  size_t limit,
                                  char const* start,
                                  char const* end,
                                  enum pubnub_tribool count)
{
    struct pubnub_getall_metadata_opts opts = pubnub_getall_metadata_defopts();
    opts.include = include;
    opts.limit = limit;
    opts.page.next = start;
    opts.page.prev = end;
    opts.count = count;

    return pubnub_getall_channelmetadata_ex(pb, opts);
}


enum pubnub_res pubnub_getall_channelmetadata_ex(pubnub_t* pb, 
        struct pubnub_getall_metadata_opts opts)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }
    pb->trans = PBTT_GETALL_CHANNELMETADATA;
    rslt = pbcc_getall_channelmetadata_prep(&pb->core,
                                opts.include, 
                                opts.limit,
                                opts.page.next,
                                opts.page.prev,
                                opts.count, 
                                opts.filter,
                                opts.sort,
                                pb->trans);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_GETALL_CHANNELMETADATA;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}


struct pubnub_set_channelmetadata_opts pubnub_set_channelmetadata_defopts(void)
{
    struct pubnub_set_channelmetadata_opts opts;
    opts.include = NULL;
    opts.data = pubnub_null_channel_data;

    return opts;
}


enum pubnub_res pubnub_set_channelmetadata(pubnub_t* pb, 
                                    char const* channel_metadataid,
                                    char const* include, 
                                    char const* channel_metadata_obj)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }

    pb->method = pubnubUsePATCH;
    pb->trans = PBTT_SET_CHANNELMETADATA;
    rslt = pbcc_set_channelmetadata_prep(&pb->core,
                                  channel_metadataid,
                                  include,
                                  channel_metadata_obj,
                                  pb->trans);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_SET_CHANNELMETADATA;
        pb->core.last_result = PNR_STARTED;
        pb->method           = pubnubUsePATCH;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}


enum pubnub_res pubnub_set_channelmetadata_ex(pubnub_t*   pb,
                                              char const* channel,
                                              struct pubnub_set_channelmetadata_opts opts)
{
    char*           obj_buffer = NULL;
    enum pubnub_res result;
    size_t          obj_len = 0;

    obj_len = NULL != opts.data.custom ? strlen(opts.data.custom) : 0;
    obj_len += NULL != opts.data.description ? strlen(opts.data.description) : 0;
    obj_len += NULL != opts.data.name ? strlen(opts.data.name) : 0;

    obj_buffer = (char*)malloc(obj_len + 64); // 64 is for the JSON object structure with small buffer
    int offset = snprintf(obj_buffer, obj_len, "{");

    if (NULL != opts.data.custom) {
        offset += snprintf(obj_buffer, obj_len, "\"custom\":%s", opts.data.custom);
    }

    if (NULL != opts.data.description) {
        offset += snprintf(obj_buffer + offset,
                           obj_len - offset,
                           "%s\"description\":\"%s\"",
                           offset > 1 ? "," : "",
                           opts.data.description);
    }

    if (NULL != opts.data.name) {
        offset += snprintf(obj_buffer + offset,
                           obj_len - offset,
                           "%s\"name\":\"%s\"",
                           offset > 1 ? "," : "",
                           opts.data.name);
    }

    offset += snprintf(obj_buffer + offset, obj_len - offset, "}\0");

    result = pubnub_set_channelmetadata(pb, channel, opts.include, obj_buffer);
    free(obj_buffer);

    return result;
}


enum pubnub_res pubnub_get_channelmetadata(pubnub_t* pb,
                                 char const* include, 
                                 char const* channel_metadataid)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }
    pb->trans = PBTT_GET_CHANNELMETADATA;
    rslt = pbcc_get_channelmetadata_prep(&pb->core, include, channel_metadataid, pb->trans);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_GET_CHANNELMETADATA;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}


enum pubnub_res pubnub_remove_channelmetadata(pubnub_t* pb, char const* channel_metadataid)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }
    
    pb->method = pubnubUseDELETE;
    pb->trans = PBTT_REMOVE_CHANNELMETADATA;
    rslt = pbcc_remove_channelmetadata_prep(&pb->core, channel_metadataid, pb->trans);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_REMOVE_CHANNELMETADATA;
        pb->core.last_result = PNR_STARTED;
        pb->method           = pubnubUseDELETE;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}


struct pubnub_membership_opts pubnub_memberships_defopts(void)
{
    struct pubnub_membership_opts opts;
    opts.uuid = NULL;
    opts.include = NULL;
    opts.page = pubnub_null_page;
    opts.filter = NULL;
    opts.sort = NULL;
    opts.limit = 100;

    return opts;
}


enum pubnub_res pubnub_get_memberships(pubnub_t* pb,
                                       char const* uuid_metadataid,
                                       char const* include,
                                       size_t limit,
                                       char const* start,
                                       char const* end,
                                       enum pubnub_tribool count)
{
    struct pubnub_membership_opts opts = pubnub_memberships_defopts();
    opts.uuid = uuid_metadataid;
    opts.include = include;
    opts.limit = limit;
    opts.page.next = start;
    opts.page.prev = end;
    opts.count = count;

    return pubnub_get_memberships_ex(pb, opts);
}


enum pubnub_res pubnub_get_memberships_ex(pubnub_t* pb,
        struct pubnub_membership_opts opts)
{
    enum pubnub_res rslt;

    if (NULL == opts.uuid) {
        opts.uuid = pb->core.user_id;
    }

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }
    pb->trans = PBTT_GET_MEMBERSHIPS;
    rslt = pbcc_get_memberships_prep(&pb->core,
                                     opts.uuid,
                                     opts.include, 
                                     opts.limit,
                                     opts.page.next,
                                     opts.page.prev,
                                     opts.count, 
                                     opts.filter,
                                     opts.sort,
                                     pb->trans);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_GET_MEMBERSHIPS;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}


enum pubnub_res pubnub_set_memberships(pubnub_t* pb, 
                                          char const* uuid_metadataid,
                                          char const* include,
                                          char const* set_obj)
{
    struct pubnub_membership_opts opts = pubnub_memberships_defopts();
    opts.uuid = uuid_metadataid;
    opts.include = include;

    return pubnub_set_memberships_ex(pb, set_obj, opts);
}


enum pubnub_res pubnub_set_memberships_ex(pubnub_t* pb, 
        char const* channels,
        struct pubnub_membership_opts opts)
{
    enum pubnub_res rslt;
    char obj_buffer[PUBNUB_BUF_MAXLEN];
    
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }
    
    FORM_THE_OBJECT(&pb->core,
                    pb->monitor,
                    "pubnub_set_memberships",
                    obj_buffer,
                    "{\"set\":",
                    channels);

    pb->method = pubnubUsePATCH;
    pb->trans = PBTT_SET_MEMBERSHIPS;
    rslt = pbcc_set_memberships_prep(&pb->core,
                                        opts.uuid,
                                        opts.include,
                                        channels, 
                                        opts.filter,
                                        opts.sort,
                                        opts.limit,
                                        opts.page.next,
                                        opts.page.prev,
                                        opts.count,
                                        pb->trans);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_SET_MEMBERSHIPS;
        pb->core.last_result = PNR_STARTED;
        pb->method           = pubnubUsePATCH;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}


enum pubnub_res pubnub_remove_memberships(pubnub_t* pb, 
                                    char const* uuid_metadataid,
                                    char const* include,
                                    char const* remove_obj)
{
    struct pubnub_membership_opts opts = pubnub_memberships_defopts();
    opts.uuid = uuid_metadataid;
    opts.include = include;

    return pubnub_remove_memberships_ex(pb, remove_obj, opts);
}


enum pubnub_res pubnub_remove_memberships_ex(pubnub_t* pb, 
        char const* channels,
        struct pubnub_membership_opts opts)
{
    enum pubnub_res rslt;
    char obj_buffer[PUBNUB_BUF_MAXLEN];

    if (NULL == opts.uuid) {
        opts.uuid = pb->core.user_id;
    }
    
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }
    
    FORM_THE_OBJECT(&pb->core,
                    pb->monitor,
                    "pubnub_remove_memberships",
                    obj_buffer,
                    "{\"delete\":",
                    channels);
    pb->method = pubnubUsePATCH;
    pb->trans = PBTT_REMOVE_MEMBERSHIPS;
    rslt = pbcc_set_memberships_prep(&pb->core,
                                        opts.uuid,
                                        opts.include,
                                        channels, 
                                        opts.filter,
                                        opts.sort,
                                        opts.limit,
                                        opts.page.next,
                                        opts.page.prev,
                                        opts.count,
                                        pb->trans);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_REMOVE_MEMBERSHIPS;
        pb->core.last_result = PNR_STARTED;
        pb->method           = pubnubUsePATCH;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}


struct pubnub_members_opts pubnub_members_defopts(void)
{
    struct pubnub_members_opts opts;
    opts.include = NULL;
    opts.filter = NULL;
    opts.sort = NULL;
    opts.limit = 100;
    opts.page = pubnub_null_page;
    opts.count = pbccNotSet;

    return opts;
}


enum pubnub_res pubnub_get_members(pubnub_t* pb,
                                   char const* channel_metadataid,
                                   char const* include,
                                   size_t limit,
                                   char const* start,
                                   char const* end,
                                   enum pubnub_tribool count)
{
    struct pubnub_members_opts opts = pubnub_members_defopts();
    opts.include = include;
    opts.limit = limit;
    opts.page.next = start;
    opts.page.prev = end;
    opts.count = count;

    return pubnub_get_members_ex(pb, channel_metadataid, opts);
}


enum pubnub_res pubnub_get_members_ex(pubnub_t* pb,
        char const* channel,
        struct pubnub_members_opts opts)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }
    pb->trans = PBTT_GET_MEMBERS;
    rslt = pbcc_get_members_prep(&pb->core,
                                 channel,
                                 opts.include,
                                 opts.limit,
                                 opts.page.next,
                                 opts.page.prev,
                                 opts.filter,
                                 opts.sort,
                                 opts.count, 
                                 pb->trans);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_GET_MEMBERS;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}


enum pubnub_res pubnub_add_members(pubnub_t* pb, 
                                   char const* channel_metadataid,
                                   char const* include,
                                   char const* update_obj)
{
    char obj_buffer[PUBNUB_BUF_MAXLEN];
    enum pubnub_res rslt;
    struct pubnub_members_opts defaults = pubnub_members_defopts();

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }
    
    FORM_THE_OBJECT(&pb->core,
                    pb->monitor,
                    "pubnub_add_members",
                    obj_buffer,
                    "{\"add\":",
                    update_obj);
    pb->trans = PBTT_ADD_MEMBERS;
    rslt = pbcc_set_members_prep(&pb->core,
                                    channel_metadataid,
                                    include,
                                    update_obj, 
                                    defaults.filter,
                                    defaults.sort,
                                    defaults.limit,
                                    defaults.page.next,
                                    defaults.page.prev,
                                    defaults.count,
                                    pb->trans);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_ADD_MEMBERS;
        pb->core.last_result = PNR_STARTED;
        pb->method           = pubnubUsePATCH;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}


enum pubnub_res pubnub_set_members(pubnub_t* pb, 
                                      char const* channel_metadataid,
                                      char const* include,
                                      char const* set_obj)
{
    struct pubnub_members_opts opts = pubnub_members_defopts();
    opts.include = include;

    return pubnub_set_members_ex(pb, channel_metadataid, set_obj, opts);
}


enum pubnub_res pubnub_set_members_ex(pubnub_t* pb, 
        char const* channel,
        char const* uuids,
        struct pubnub_members_opts opts)
{
    enum pubnub_res rslt;
    char obj_buffer[PUBNUB_BUF_MAXLEN];

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }
    
    FORM_THE_OBJECT(&pb->core,
                    pb->monitor,
                    "pubnub_set_members",
                    obj_buffer,
                    "{\"set\":",
                    uuids);
    pb->method = pubnubUsePATCH;
    pb->trans = PBTT_SET_MEMBERS;
    rslt = pbcc_set_members_prep(&pb->core,
                                    channel,
                                    opts.include,
                                    uuids, 
                                    opts.filter,
                                    opts.sort,
                                    opts.limit,
                                    opts.page.next,
                                    opts.page.prev,
                                    opts.count,
                                    pb->trans);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_SET_MEMBERS;
        pb->core.last_result = PNR_STARTED;
        pb->method           = pubnubUsePATCH;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}



enum pubnub_res pubnub_remove_members(pubnub_t* pb, 
                                      char const* channel_metadataid,
                                      char const* include,
                                      char const* remove_obj)
{
    struct pubnub_members_opts opts = pubnub_members_defopts();
    opts.include = include;

    return pubnub_remove_members_ex(pb, channel_metadataid, remove_obj, opts);
}


enum pubnub_res pubnub_remove_members_ex(pubnub_t* pb, 
        char const* channel,
        char const* uuids,
        struct pubnub_members_opts opts)
{
    enum pubnub_res rslt;
    char obj_buffer[PUBNUB_BUF_MAXLEN];

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }
    
    FORM_THE_OBJECT(&pb->core,
                    pb->monitor,
                    "pubnub_remove_members",
                    obj_buffer,
                    "{\"delete\":",
                    uuids);
    pb->method = pubnubUsePATCH;
    pb->trans = PBTT_REMOVE_MEMBERS;
    rslt = pbcc_set_members_prep(&pb->core,
                                    channel,
                                    opts.include,
                                    uuids, 
                                    opts.filter,
                                    opts.sort,
                                    opts.limit,
                                    opts.page.next,
                                    opts.page.prev,
                                    opts.count,
                                    pb->trans);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_REMOVE_MEMBERS;
        pb->core.last_result = PNR_STARTED;
        pb->method           = pubnubUsePATCH;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}


#endif /* PUBNUB_USE_OBJECTS_API */

