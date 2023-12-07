/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#if PUBNUB_USE_ACTIONS_API

#include "pubnub_internal.h"

#include "core/pubnub_ccore.h"
#include "core/pubnub_netcore.h"
#include "core/pubnub_assert.h"
#include "core/pubnub_timers.h"
#include "core/pubnub_log.h"
#include "core/pubnub_actions_api.h"

#include "core/pbpal.h"

#include <ctype.h>
#include <string.h>


enum pubnub_res pubnub_add_message_action(pubnub_t* pb,
                                  char const* channel,
                                  char const* message_timetoken,
                                  enum pubnub_action_type actype,
                                  char const* value)
{
    enum pubnub_res rslt;
    char obj_buffer[PUBNUB_BUF_MAXLEN];

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_ASSERT(value != NULL);

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }
    rslt = pbcc_form_the_action_object(&pb->core,
                                       obj_buffer,
                                       sizeof obj_buffer,
                                       actype,
                                       &value);
    if (rslt != PNR_OK) {
        pubnub_mutex_unlock(pb->monitor);
        return rslt;
    }
#if PUBNUB_USE_GZIP_COMPRESSION
    value = (pbgzip_compress(pb, value) == PNR_OK) ? pb->core.gzip_msg_buf : value;
#endif
    pb->method = pubnubSendViaPOST;
    rslt = pbcc_add_action_prep(&pb->core,
                                channel,
                                message_timetoken,
                                value);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_ADD_ACTION;
        pb->core.last_result = PNR_STARTED;
        pb->method           = pubnubSendViaPOST;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}


pubnub_chamebl_t pubnub_get_message_timetoken(pubnub_t* pb)
{
    pubnub_chamebl_t result;
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (pb->trans != PBTT_ADD_ACTION) {
        PUBNUB_LOG_ERROR("pubnub_get_message_timetoken(pb=%p) can be called only if "
                         "previous transaction is PBTT_ADD_ACTION(%d). "
                         "Previous transaction was: %d\n",
                         pb,
                         PBTT_ADD_ACTION,
                         pb->trans);
        pubnub_mutex_unlock(pb->monitor);
        result.ptr = NULL;
        result.size = 0;
        return result;
    }
    result = pbcc_get_message_timetoken(&pb->core);
    pubnub_mutex_unlock(pb->monitor);

    return result;
}


pubnub_chamebl_t pubnub_get_message_action_timetoken(pubnub_t* pb)
{
    pubnub_chamebl_t result;
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (pb->trans != PBTT_ADD_ACTION) {
        PUBNUB_LOG_ERROR("pubnub_get_action_timetoken(pb=%p) can be called only if "
                         "previous transaction is PBTT_ADD_ACTION(%d). "
                         "Previous transaction was: %d\n",
                         pb,
                         PBTT_ADD_ACTION,
                         pb->trans);
        pubnub_mutex_unlock(pb->monitor);
        result.ptr = NULL;
        result.size = 0;
        return result;
    }
    result = pbcc_get_action_timetoken(&pb->core);
    pubnub_mutex_unlock(pb->monitor);

    return result;
}


enum pubnub_res pubnub_remove_message_action(pubnub_t* pb,
                                     char const* channel,
                                     pubnub_chamebl_t message_timetoken,
                                     pubnub_chamebl_t action_timetoken)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }
    pb->method = pubnubUseDELETE;
    rslt = pbcc_remove_action_prep(&pb->core,
                                   channel,
                                   message_timetoken,
                                   action_timetoken);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_REMOVE_ACTION;
        pb->core.last_result = PNR_STARTED;
        pb->method           = pubnubUseDELETE;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}


enum pubnub_res pubnub_get_message_actions(pubnub_t* pb,
                                   char const* channel,
                                   char const* start,
                                   char const* end,
                                   size_t limit)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }

    rslt = pbcc_get_actions_prep(&pb->core, channel, start, end, limit);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_GET_ACTIONS;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}


enum pubnub_res pubnub_get_message_actions_more(pubnub_t* pb)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }
    if (pb->trans != PBTT_GET_ACTIONS) {
        PUBNUB_LOG_ERROR("pubnub_get_actions_more(pb=%p) can be called only if "
                         "previous transaction is PBTT_GET_ACTIONS(%d). "
                         "Previous transaction was: %d\n",
                         pb,
                         PBTT_GET_ACTIONS,
                         pb->trans);
        pubnub_mutex_unlock(pb->monitor);
        return PNR_INTERNAL_ERROR;
    }

    rslt = pbcc_get_actions_more_prep(&pb->core);
    if (PNR_STARTED == rslt) {
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}


enum pubnub_res pubnub_history_with_message_actions(pubnub_t* pb,
                                            char const* channel,
                                            char const* start,
                                            char const* end,
                                            size_t limit)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }

    rslt = pbcc_history_with_actions_prep(&pb->core, channel, start, end, limit);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_HISTORY_WITH_ACTIONS;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}


enum pubnub_res pubnub_history_with_message_actions_more(pubnub_t* pb)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }
    if (pb->trans != PBTT_HISTORY_WITH_ACTIONS) {
        PUBNUB_LOG_ERROR("pubnub_history_with_actions_more(pb=%p) can be called only if "
                         "previous transaction is PBTT_HISTORY_WITH_ACTIONS(%d). "
                         "Previous transaction was: %d\n",
                         pb,
                         PBTT_HISTORY_WITH_ACTIONS,
                         pb->trans);
        pubnub_mutex_unlock(pb->monitor);
        return PNR_INTERNAL_ERROR;
    }

    rslt = pbcc_get_actions_more_prep(&pb->core);
    if (PNR_STARTED == rslt) {
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}

#endif /* PUBNUB_USE_ACTIONS_API */

