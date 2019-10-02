/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
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


enum pubnub_res pubnub_get_users(pubnub_t* pb, 
                                 char const** include, 
                                 size_t include_count,
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
    
    rslt = pbcc_get_users_prep(&pb->core,
                               include, 
                               include_count,
                               limit,
                               start,
                               end,
                               count);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_GET_USERS;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}


enum pubnub_res pubnub_create_user(pubnub_t* pb, 
                                   char const** include, 
                                   size_t include_count,
                                   char const* user_obj)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }
    
#if PUBNUB_USE_GZIP_COMPRESSION
    user_obj = (pbgzip_compress(pb, user_obj) == PNR_OK) ? pb->core.gzip_msg_buf : user_obj;
#endif
    rslt = pbcc_create_user_prep(&pb->core, include, include_count, user_obj);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_CREATE_USER;
        pb->core.last_result = PNR_STARTED;
        pb->method           = pubnubSendViaPOST;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}


enum pubnub_res pubnub_get_user(pubnub_t* pb,
                                char const** include, 
                                size_t include_count,
                                char const* user_id)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }
    
    rslt = pbcc_get_user_prep(&pb->core, include, include_count, user_id);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_GET_USER;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}


enum pubnub_res pubnub_update_user(pubnub_t* pb, 
                                   char const** include,
                                   size_t include_count,
                                   char const* user_obj)
{
    enum pubnub_res rslt;
    struct pbjson_elem parsed_id;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }
    rslt = pbcc_find_objects_id(&pb->core, user_obj, &parsed_id, __FILE__, __LINE__);
    if (rslt != PNR_OK) {
        pubnub_mutex_unlock(pb->monitor);
        return rslt;
    }

#if PUBNUB_USE_GZIP_COMPRESSION
    user_obj = (pbgzip_compress(pb, user_obj) == PNR_OK) ? pb->core.gzip_msg_buf : user_obj;
#endif
    rslt = pbcc_update_user_prep(&pb->core, include, include_count, user_obj, &parsed_id);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_UPDATE_USER;
        pb->core.last_result = PNR_STARTED;
        pb->method           = pubnubUsePATCH;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}


enum pubnub_res pubnub_delete_user(pubnub_t* pb, char const* user_id)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }
    
    rslt = pbcc_delete_user_prep(&pb->core, user_id);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_DELETE_USER;
        pb->core.last_result = PNR_STARTED;
        pb->method           = pubnubUseDELETE;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}


enum pubnub_res pubnub_get_spaces(pubnub_t* pb, 
                                  char const** include, 
                                  size_t include_count,
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
    
    rslt = pbcc_get_spaces_prep(&pb->core,
                                include, 
                                include_count,
                                limit,
                                start,
                                end,
                                count);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_GET_SPACES;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}


enum pubnub_res pubnub_create_space(pubnub_t* pb, 
                                    char const** include, 
                                    size_t include_count,
                                    char const* space_obj)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }

#if PUBNUB_USE_GZIP_COMPRESSION
    space_obj = (pbgzip_compress(pb, space_obj) == PNR_OK) ? pb->core.gzip_msg_buf : space_obj;
#endif
    rslt = pbcc_create_space_prep(&pb->core, include, include_count, space_obj);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_CREATE_SPACE;
        pb->core.last_result = PNR_STARTED;
        pb->method           = pubnubSendViaPOST;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}


enum pubnub_res pubnub_get_space(pubnub_t* pb,
                                 char const** include, 
                                 size_t include_count,
                                 char const* space_id)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }
    
    rslt = pbcc_get_space_prep(&pb->core, include, include_count, space_id);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_GET_SPACE;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}


enum pubnub_res pubnub_update_space(pubnub_t* pb, 
                                    char const** include,
                                    size_t include_count,
                                    char const* space_obj)
{
    enum pubnub_res rslt;
    struct pbjson_elem parsed_id;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }
    rslt = pbcc_find_objects_id(&pb->core, space_obj, &parsed_id, __FILE__, __LINE__);
    if (rslt != PNR_OK) {
        pubnub_mutex_unlock(pb->monitor);
        return rslt;
    }
    
#if PUBNUB_USE_GZIP_COMPRESSION
    space_obj = (pbgzip_compress(pb, space_obj) == PNR_OK) ? pb->core.gzip_msg_buf : space_obj;
#endif
    rslt = pbcc_update_space_prep(&pb->core, include, include_count, space_obj, &parsed_id);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_UPDATE_SPACE;
        pb->core.last_result = PNR_STARTED;
        pb->method           = pubnubUsePATCH;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}


enum pubnub_res pubnub_delete_space(pubnub_t* pb, char const* space_id)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }
    
    rslt = pbcc_delete_space_prep(&pb->core, space_id);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_DELETE_SPACE;
        pb->core.last_result = PNR_STARTED;
        pb->method           = pubnubUseDELETE;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}


enum pubnub_res pubnub_get_memberships(pubnub_t* pb,
                                       char const* user_id,
                                       char const** include,
                                       size_t include_count,
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

    rslt = pbcc_get_memberships_prep(&pb->core,
                                     user_id,
                                     include, 
                                     include_count,
                                     limit,
                                     start,
                                     end,
                                     count);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_GET_MEMBERSHIPS;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}


enum pubnub_res pubnub_join_spaces(pubnub_t* pb, 
                                   char const* user_id,
                                   char const** include,
                                   size_t include_count,
                                   char const* update_obj)
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
                    "pubnub_join_spaces",
                    obj_buffer,
                    "{\"add\":",
                    update_obj);
#if PUBNUB_USE_GZIP_COMPRESSION
    update_obj = (pbgzip_compress(pb, update_obj) == PNR_OK) ? pb->core.gzip_msg_buf : update_obj;
#endif
    rslt = pbcc_update_memberships_prep(&pb->core,
                                        user_id,
                                        include,
                                        include_count,
                                        update_obj);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_JOIN_SPACES;
        pb->core.last_result = PNR_STARTED;
        pb->method           = pubnubUsePATCH;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}


enum pubnub_res pubnub_update_memberships(pubnub_t* pb, 
                                          char const* user_id,
                                          char const** include,
                                          size_t include_count,
                                          char const* update_obj)
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
                    "pubnub_update_memberships",
                    obj_buffer,
                    "{\"update\":",
                    update_obj);
#if PUBNUB_USE_GZIP_COMPRESSION
    update_obj = (pbgzip_compress(pb, update_obj) == PNR_OK) ? pb->core.gzip_msg_buf : update_obj;
#endif
    rslt = pbcc_update_memberships_prep(&pb->core,
                                        user_id,
                                        include,
                                        include_count,
                                        update_obj);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_UPDATE_MEMBERSHIPS;
        pb->core.last_result = PNR_STARTED;
        pb->method           = pubnubUsePATCH;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}


enum pubnub_res pubnub_leave_spaces(pubnub_t* pb, 
                                    char const* user_id,
                                    char const** include,
                                    size_t include_count,
                                    char const* update_obj)
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
                    "pubnub_leave_spaces",
                    obj_buffer,
                    "{\"remove\":",
                    update_obj);
#if PUBNUB_USE_GZIP_COMPRESSION
    update_obj = (pbgzip_compress(pb, update_obj) == PNR_OK) ? pb->core.gzip_msg_buf : update_obj;
#endif
    rslt = pbcc_update_memberships_prep(&pb->core,
                                        user_id,
                                        include,
                                        include_count,
                                        update_obj);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_LEAVE_SPACES;
        pb->core.last_result = PNR_STARTED;
        pb->method           = pubnubUsePATCH;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}


enum pubnub_res pubnub_get_members(pubnub_t* pb,
                                   char const* space_id,
                                   char const** include,
                                   size_t include_count,
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
    
    rslt = pbcc_get_members_prep(&pb->core,
                                 space_id,
                                 include,
                                 include_count,
                                 limit,
                                 start,
                                 end,
                                 count);
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
                                   char const* space_id,
                                   char const** include,
                                   size_t include_count,
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
#if PUBNUB_USE_GZIP_COMPRESSION
    update_obj = (pbgzip_compress(pb, update_obj) == PNR_OK) ? pb->core.gzip_msg_buf : update_obj;
#endif
    rslt = pbcc_update_members_prep(&pb->core,
                                    space_id,
                                    include,
                                    include_count,
                                    update_obj);
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


enum pubnub_res pubnub_update_members(pubnub_t* pb, 
                                      char const* space_id,
                                      char const** include,
                                      size_t include_count,
                                      char const* update_obj)
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
                    "pubnub_update_members",
                    obj_buffer,
                    "{\"update\":",
                    update_obj);
#if PUBNUB_USE_GZIP_COMPRESSION
    update_obj = (pbgzip_compress(pb, update_obj) == PNR_OK) ? pb->core.gzip_msg_buf : update_obj;
#endif
    rslt = pbcc_update_members_prep(&pb->core,
                                    space_id,
                                    include,
                                    include_count,
                                    update_obj);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_UPDATE_MEMBERS;
        pb->core.last_result = PNR_STARTED;
        pb->method           = pubnubUsePATCH;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}


enum pubnub_res pubnub_remove_members(pubnub_t* pb, 
                                      char const* space_id,
                                      char const** include,
                                      size_t include_count,
                                      char const* update_obj)
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
                    "{\"remove\":",
                    update_obj);
#if PUBNUB_USE_GZIP_COMPRESSION
    update_obj = (pbgzip_compress(pb, update_obj) == PNR_OK) ? pb->core.gzip_msg_buf : update_obj;
#endif
    rslt = pbcc_update_members_prep(&pb->core,
                                    space_id,
                                    include,
                                    include_count,
                                    update_obj);
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
