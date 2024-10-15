/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pbcc_subscribe_event_engine_effects.h"

#include <stdio.h>

#include "core/pbcc_subscribe_event_engine_types.h"
#include "core/pubnub_subscribe_v2.h"
#include "core/pubnub_pubsubapi.h"
#include "core/pubnub_coreapi.h"
#include "core/pubnub_assert.h"
#include "pubnub_internal.h"


// ----------------------------------------------
//                  Constants
// ----------------------------------------------

/** Maximum channel name length */
#define PBCC_SUBSCRIBE_EE_CHANNEL_MAXIMUM_LENGTH 2100


// ----------------------------------------------
//               Function prototypes
// ----------------------------------------------

/**
 * @brief Make actual call to the Subscribe v2.
 *
 * @param invocation Pointer to the invocation which has been used to execute
 *                   effect.
 * @param context    Pointer to the Subscription context with all required
 *                   information to perform Subscribe v2 call.
 * @param cb         Pointer to the effect execution completion callback
 *                   function.
 */
static void make_subscribe_request_(
    pbcc_ee_invocation_t*                invocation,
    pbcc_ee_data_t*                      context,
    pbcc_ee_effect_completion_function_t cb);


// ----------------------------------------------
//                   Functions
// ----------------------------------------------

void pbcc_subscribe_ee_handshake_effect(
    pbcc_ee_invocation_t*                      invocation,
    pbcc_ee_data_t*                            context,
    const pbcc_ee_effect_completion_function_t cb)
{
    make_subscribe_request_(invocation, context, cb);
}

void pbcc_subscribe_ee_receive_effect(
    pbcc_ee_invocation_t*                      invocation,
    pbcc_ee_data_t*                            context,
    const pbcc_ee_effect_completion_function_t cb)
{
    make_subscribe_request_(invocation, context, cb);
}

void pbcc_subscribe_ee_emit_status_effect(
    pbcc_ee_invocation_t*                      invocation,
    pbcc_ee_data_t*                            context,
    const pbcc_ee_effect_completion_function_t cb)
{
    pbcc_ee_data_t* context_copy = pbcc_ee_data_copy(context);
    const pbcc_subscribe_ee_context_t* ctx = pbcc_ee_data_value(context_copy);
    pbcc_subscribe_ee_t* subscribe_ee = ctx->pb->core.subscribe_ee;

    pubnub_mutex_lock(subscribe_ee->mutw);
    const pubnub_subscription_status status = subscribe_ee->status;
    pubnub_mutex_unlock(subscribe_ee->mutw);

    pbcc_event_listener_emit_status(subscribe_ee->event_listener,
                                    status,
                                    ctx->reason,
                                    pbcc_ee_data_value(ctx->channels),
                                    pbcc_ee_data_value(ctx->channel_groups));
    cb(subscribe_ee->ee, invocation, false);
    pbcc_ee_data_free(context_copy);
}

void pbcc_subscribe_ee_emit_messages_effect(
    pbcc_ee_invocation_t*                      invocation,
    pbcc_ee_data_t*                            context,
    const pbcc_ee_effect_completion_function_t cb)
{
    char subscribable_name[PBCC_SUBSCRIBE_EE_CHANNEL_MAXIMUM_LENGTH];
    pbcc_ee_data_t* context_copy = pbcc_ee_data_copy(context);
    const pbcc_subscribe_ee_context_t* ctx = pbcc_ee_data_value(context_copy);
    const pbcc_subscribe_ee_t* subscribe_ee = ctx->pb->core.subscribe_ee;
    pubnub_t* pb = ctx->pb;

    for (struct pubnub_v2_message msg = pubnub_get_v2(pb); msg.payload.size > 0;
         msg                          = pubnub_get_v2(pb)) {
        struct pubnub_char_mem_block subscribable;
        if (msg.match_or_group.size) { subscribable = msg.match_or_group; }
        else {subscribable = msg.channel;}
        memcpy(subscribable_name, subscribable.ptr, subscribable.size);
        subscribable_name[subscribable.size] = '\0';
        pbcc_event_listener_emit_message(subscribe_ee->event_listener, subscribable_name, msg);
    }

    cb(subscribe_ee->ee, invocation, false);
    pbcc_ee_data_free(context_copy);
}

void pbcc_subscribe_ee_cancel_effect(
    pbcc_ee_invocation_t*                      invocation,
    pbcc_ee_data_t*                            context,
    const pbcc_ee_effect_completion_function_t cb)
{
    PUBNUB_ASSERT_OPT(NULL != context);

    pbcc_ee_data_t* context_copy = pbcc_ee_data_copy(context);
    const pbcc_subscribe_ee_context_t* ctx = pbcc_ee_data_value(context_copy);
    pbcc_subscribe_ee_t* subscribe_ee = ctx->pb->core.subscribe_ee;
    PUBNUB_ASSERT(pb_valid_ctx_ptr(ctx->pb));

    /**
     * Check whether there is any request is in progress and don't let it to be
     * cancelled.
     */
    pubnub_mutex_lock(subscribe_ee->mutw);
    const enum pubnub_trans transaction = subscribe_ee->current_transaction;
    if (PBTT_SUBSCRIBE_V2 != transaction && PBTT_NONE != transaction) {
        pubnub_mutex_unlock(subscribe_ee->mutw);
        pbcc_ee_data_free(context_copy);
        cb(subscribe_ee->ee, invocation, true);

        return;
    }
    pubnub_mutex_unlock(subscribe_ee->mutw);

    if (PN_CANCEL_FINISHED == pubnub_cancel(ctx->pb))
        cb(subscribe_ee->ee, invocation, false);
    else
        subscribe_ee->cancel_invocation = invocation;
    pbcc_ee_data_free(context_copy);
}

void make_subscribe_request_(
    pbcc_ee_invocation_t*                      invocation,
    pbcc_ee_data_t*                            context,
    const pbcc_ee_effect_completion_function_t cb)
{
    PUBNUB_ASSERT_OPT(NULL != context);

    pbcc_ee_data_t* context_copy = pbcc_ee_data_copy(context);
    pbcc_subscribe_ee_context_t* ctx = pbcc_ee_data_value(context_copy);
    pbcc_subscribe_ee_t* subscribe_ee = ctx->pb->core.subscribe_ee;
    pubnub_t* pb = ctx->pb;

    /**
     * Check whether there any request is in progress and postpone subscribe
     * effect execution untill it will be completed.
     */
    pubnub_mutex_lock(subscribe_ee->mutw);
    if (PBTT_NONE != subscribe_ee->current_transaction) {
        pubnub_mutex_unlock(subscribe_ee->mutw);
        cb(subscribe_ee->ee, invocation, true);
        pbcc_ee_data_free(context_copy);

        return;
    }

    if (ctx->send_heartbeat) {
        pubnub_mutex_lock(pb->monitor);
        if (!pbnc_can_start_transaction(pb)) {
            pubnub_mutex_unlock(pb->monitor);

            /** Postpone invocation because there is ongoing request. */
            cb(subscribe_ee->ee, invocation, true);
            pbcc_ee_data_free(context_copy);

            return;
        }

        pubnub_mutex_unlock(pb->monitor);
        ctx->send_heartbeat               = false;
        subscribe_ee->current_transaction = PBTT_HEARTBEAT;
        pubnub_heartbeat(subscribe_ee->pb,
            pbcc_ee_data_value(ctx->channels),
            pbcc_ee_data_value(ctx->channel_groups));
        pubnub_mutex_unlock(subscribe_ee->mutw);

        /** Postpone invocation because there is ongoing heratbeat request. */
        cb(subscribe_ee->ee, invocation, true);
        pbcc_ee_data_free(context_copy);

        return;
    }
    pubnub_mutex_unlock(subscribe_ee->mutw);

    /** Override timetoken stored for next subscription loop. */
    pubnub_mutex_lock(pb->monitor);
    size_t token_len = strlen(ctx->cursor.timetoken);
    memcpy(pb->core.timetoken, ctx->cursor.timetoken, token_len);
    pb->core.timetoken[token_len] = '\0';
    pb->core.region = ctx->cursor.region;
    pbpal_mutex_unlock(pb->monitor);

    struct pubnub_subscribe_v2_options opts = pubnub_subscribe_v2_defopts();
    opts.filter_expr = subscribe_ee->filter_expr;
    opts.channel_group = pbcc_ee_data_value(ctx->channel_groups);
    opts.heartbeat = subscribe_ee->heartbeat;

    const enum pubnub_res rslt = pubnub_subscribe_v2(
        pb,
        pbcc_ee_data_value(ctx->channels),
        opts);

    /**
     * Report effect invocation called or should be paused if not started.
     * If other request in progress, after registered callback call it will
     * trigger next invocation which will call paused once more.
     */
    cb(subscribe_ee->ee, invocation, PNR_IN_PROGRESS == rslt);

    if (PNR_OK != rslt && PNR_STARTED != rslt && PNR_IN_PROGRESS != rslt)
        pbcc_subscribe_ee_handle_subscribe_error(subscribe_ee, rslt);

    pbcc_ee_data_free(context_copy);
}