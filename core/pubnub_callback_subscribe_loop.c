/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"

#include "pubnub_callback_subscribe_loop.h"

#include "pubnub_pubsubapi.h"
#include "pubnub_mutex.h"
#include "pubnub_assert.h"
#include "pubnub_log.h"


/** Subscribe loop descriptor */
struct pubnub_subloop_descriptor {
    /** The context to use */
    pubnub_guarded_by(monitor) pubnub_t* pbp;
    /** Channel to subscribe to */
    pubnub_guarded_by(monitor) char const* channel;
    /** Extended subscribe options */
    pubnub_guarded_by(monitor) struct pubnub_subscribe_options options;
    /** Callback to call */
    pubnub_guarded_by(monitor) pubnub_subloop_callback_t cb;
    /** Saved callback from the #pbp context */
    pubnub_guarded_by(monitor) pubnub_callback_t saved_context_cb;
    /** Saved user data for callback from the #pbp context */
    pubnub_guarded_by(monitor) void* saved_context_user_data;

#if PUBNUB_THREADSAFE
    pubnub_mutex_t monitor;
#endif
};


static void sublup_context_callback(pubnub_t*         pb,
                                    enum pubnub_trans trans,
                                    enum pubnub_res   result,
                                    void*             user_data)
{
    pubnub_subloop_t* pbsld = (pubnub_subloop_t*)user_data;

    PUBNUB_ASSERT_OPT(pbsld != NULL);

    pubnub_mutex_lock(pbsld->monitor);
    if (PBTT_SUBSCRIBE == trans) {
        if (PNR_OK == result) {
            char const* msg;
            for (msg = pubnub_get(pb); msg != NULL; msg = pubnub_get(pb)) {
                pbsld->cb(pb, msg, PNR_OK);
            }
        }
        else {
            pbsld->cb(pb, NULL, result);
        }
        result = pubnub_subscribe_ex(pbsld->pbp, pbsld->channel, pbsld->options);
        if (result != PNR_STARTED) {
            PUBNUB_LOG_ERROR("Failed to re-subscribe in the subscribe loop, "
                             "error code = %d\n",
                             result);
        }
    }
    pubnub_mutex_unlock(pbsld->monitor);
}


pubnub_subloop_t* pubnub_subloop_define(pubnub_t*                       p,
                                        char const*                     channel,
                                        struct pubnub_subscribe_options options,
                                        pubnub_subloop_callback_t       cb)
{
    pubnub_subloop_t* rslt = (pubnub_subloop_t*)malloc(sizeof(pubnub_subloop_t));
    if (NULL == rslt) {
        return NULL;
    }

    rslt->pbp              = p;
    rslt->channel          = channel;
    rslt->options          = options;
    rslt->cb               = cb;
    rslt->saved_context_cb = NULL;
    pubnub_mutex_init(rslt->monitor);

    return rslt;
}


enum pubnub_res pubnub_subloop_start(pubnub_subloop_t* pbsld)
{
    enum pubnub_res   rslt;
    pubnub_callback_t saved_ctx_cb;
    void*             saved_ctx_data;

    PUBNUB_ASSERT_OPT(NULL != pbsld);

    pubnub_mutex_lock(pbsld->monitor);
    PUBNUB_ASSERT_OPT(NULL != pbsld->pbp);
    saved_ctx_cb   = pubnub_get_callback(pbsld->pbp);
    saved_ctx_data = pubnub_get_user_data(pbsld->pbp);
    rslt = pubnub_register_callback(pbsld->pbp, sublup_context_callback, pbsld);
    if (PNR_OK == rslt) {
        pbsld->saved_context_cb        = saved_ctx_cb;
        pbsld->saved_context_user_data = saved_ctx_data;
        rslt = pubnub_subscribe_ex(pbsld->pbp, pbsld->channel, pbsld->options);
    }
    pubnub_mutex_unlock(pbsld->monitor);

    return rslt;
}

void pubnub_subloop_stop(pubnub_subloop_t* pbsld)
{
    PUBNUB_ASSERT_OPT(NULL != pbsld);

    pubnub_mutex_lock(pbsld->monitor);
    PUBNUB_ASSERT_OPT(NULL != pbsld->pbp);
    pubnub_register_callback(
        pbsld->pbp, pbsld->saved_context_cb, pbsld->saved_context_user_data);
    pubnub_cancel(pbsld->pbp);
    pbsld->saved_context_cb        = NULL;
    pbsld->saved_context_user_data = NULL;
    pubnub_mutex_unlock(pbsld->monitor);
}


void pubnub_subloop_undef(pubnub_subloop_t* pbsld)
{
    PUBNUB_ASSERT_OPT(NULL != pbsld);

    pubnub_mutex_lock(pbsld->monitor);
    if (sublup_context_callback == pubnub_get_callback(pbsld->pbp)) {
        pubnub_register_callback(
            pbsld->pbp, pbsld->saved_context_cb, pbsld->saved_context_user_data);
        pubnub_cancel(pbsld->pbp);
    }
    pbsld->pbp = NULL;
    pubnub_mutex_unlock(pbsld->monitor);
    pubnub_mutex_destroy(pbsld->monitor);

    free(pbsld);
}
