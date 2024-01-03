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


enum pubnub_res pubnub_getall_uuidmetadata(pubnub_t* pb, 
                                 char const* include, 
                                 size_t limit,
                                 char const* start,
                                 char const* end,
                                 enum pubnub_tribool count)
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
                               include, 
                               limit,
                               start,
                               end,
                               count, 
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
    rslt = pbcc_set_uuidmetadata_prep(&pb->core, uuid_metadataid, include, uuid_metadata_obj, pb->trans);
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
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }
    pb->trans = PBTT_GETALL_CHANNELMETADATA;
    rslt = pbcc_getall_channelmetadata_prep(&pb->core,
                                include, 
                                limit,
                                start,
                                end,
                                count, 
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
    rslt = pbcc_set_channelmetadata_prep(&pb->core, channel_metadataid, include, channel_metadata_obj, pb->trans);
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


enum pubnub_res pubnub_get_memberships(pubnub_t* pb,
                                       char const* uuid_metadataid,
                                       char const* include,
                                       size_t limit,
                                       char const* start,
                                       char const* end,
                                       enum pubnub_tribool count)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }
    pb->trans = PBTT_GET_MEMBERSHIPS;
    rslt = pbcc_get_memberships_prep(&pb->core,
                                     uuid_metadataid,
                                     include, 
                                     limit,
                                     start,
                                     end,
                                     count, 
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
                    set_obj);

    pb->method = pubnubUsePATCH;
    pb->trans = PBTT_SET_MEMBERSHIPS;
    rslt = pbcc_set_memberships_prep(&pb->core,
                                        uuid_metadataid,
                                        include,
                                        set_obj, 
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
                    "pubnub_remove_memberships",
                    obj_buffer,
                    "{\"delete\":",
                    remove_obj);
    pb->method = pubnubUsePATCH;
    pb->trans = PBTT_REMOVE_MEMBERSHIPS;
    rslt = pbcc_set_memberships_prep(&pb->core,
                                        uuid_metadataid,
                                        include,
                                        remove_obj, 
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


enum pubnub_res pubnub_get_members(pubnub_t* pb,
                                   char const* channel_metadataid,
                                   char const* include,
                                   size_t limit,
                                   char const* start,
                                   char const* end,
                                   enum pubnub_tribool count)
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
                                 channel_metadataid,
                                 include,
                                 limit,
                                 start,
                                 end,
                                 count, 
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
                    set_obj);
    pb->method = pubnubUsePATCH;
    pb->trans = PBTT_SET_MEMBERS;
    rslt = pbcc_set_members_prep(&pb->core,
                                    channel_metadataid,
                                    include,
                                    set_obj, 
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
                    remove_obj);
    pb->method = pubnubUsePATCH;
    pb->trans = PBTT_REMOVE_MEMBERS;
    rslt = pbcc_set_members_prep(&pb->core,
                                    channel_metadataid,
                                    include,
                                    remove_obj, 
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

