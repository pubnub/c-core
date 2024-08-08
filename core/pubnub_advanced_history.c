/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"

#if PUBNUB_USE_ADVANCED_HISTORY
#include "pubnub_memory_block.h"
#include "pubnub_advanced_history.h"
#include "pubnub_json_parse.h"

#include "pubnub_assert.h"
#include "pubnub_log.h"


/** Should be called only if server reported an error */
int pubnub_get_error_message(pubnub_t* pb, pubnub_chamebl_t* o_msg)
{
    int rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return -1;
    }
    rslt = pbcc_get_error_message(&(pb->core), o_msg);

    pubnub_mutex_unlock(pb->monitor);
    return rslt;
}


int pubnub_get_chan_msg_counts_size(pubnub_t* pb)
{
    /* Number of '"channel":msg_count' pairs */
    int rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return -1;
    }
    rslt = pbcc_get_chan_msg_counts_size(&(pb->core));

    pubnub_mutex_unlock(pb->monitor);
    return rslt;
}


enum pubnub_res pubnub_message_counts(pubnub_t*   pb,
                                      char const* channel,
                                      char const* timetoken)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_ASSERT_OPT(channel != NULL);
    PUBNUB_ASSERT_OPT(timetoken != NULL);

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }

    if (strchr(timetoken, ',') == NULL) {
        rslt = pbcc_message_counts_prep(
            PBTT_MESSAGE_COUNTS, &(pb->core), channel, timetoken, NULL);
    }
    else {
        rslt = pbcc_message_counts_prep(
            PBTT_MESSAGE_COUNTS, &(pb->core), channel, NULL, timetoken);
    }
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_MESSAGE_COUNTS;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }

    pubnub_mutex_unlock(pb->monitor);
    return rslt;
}


int pubnub_get_chan_msg_counts(pubnub_t*                     pb,
                               size_t*                       io_count,
                               struct pubnub_chan_msg_count* chan_msg_counters)
{
    int rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_ASSERT_OPT(io_count != NULL);
    PUBNUB_ASSERT_OPT(chan_msg_counters != NULL);

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return -1;
    }
    rslt = pbcc_get_chan_msg_counts(&(pb->core), io_count, chan_msg_counters);

    pubnub_mutex_unlock(pb->monitor);
    return rslt;
}


int pubnub_get_message_counts(pubnub_t* pb, char const* channel, int* o_count)
{
    int rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_ASSERT_OPT(channel != NULL);
    PUBNUB_ASSERT_OPT(o_count != NULL);

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return -1;
    }
    rslt = pbcc_get_message_counts(&(pb->core), channel, o_count);

    pubnub_mutex_unlock(pb->monitor);
    return rslt;
}

struct pubnub_delete_messages_options pubnub_delete_messages_defopts(void)
{
    struct pubnub_delete_messages_options options;

    options.start = NULL;
    options.end   = NULL;

    return options;
}

enum pubnub_res pubnub_delete_messages(pubnub_t*   pb,
                                       char const* channel,
                                       struct pubnub_delete_messages_options options)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_ASSERT_OPT(NULL != channel);

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }

    enum pubnub_res rslt =
        pbcc_delete_messages_prep(&pb->core, channel, options.start, options.end);

    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_DELETE_MESSAGES;
        pb->method           = pubnubUseDELETE;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}

pubnub_chamebl_t pubnub_get_delete_messages_response(pubnub_t* pb)
{
    pubnub_chamebl_t resp;
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (PBTT_DELETE_MESSAGES != pb->trans) {
        PUBNUB_LOG_ERROR(
            "pubnub_get_delete_messages_response(pb=%p) can be "
            "called only if previous transaction is "
            "PBTT_DELETE_MESSAGES(%d). Previous transaction was: %d\n",
            pb,
            PBTT_DELETE_MESSAGES,
            pb->trans);
        pubnub_mutex_unlock(pb->monitor);
        resp.ptr  = NULL;
        resp.size = 0;
        return resp;
    }

    resp = pbcc_get_delete_messages_response(&pb->core);
    pubnub_mutex_unlock(pb->monitor);

    return resp;
}

#endif /* PUBNUB_USE_ADVANCED_HISTORY */
