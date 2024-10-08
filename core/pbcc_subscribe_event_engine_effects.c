/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pbcc_subscribe_event_engine_effects.h"

#include "core/pbcc_subscribe_event_engine_types.h"
#include "core/pubnub_subscribe_v2.h"
#include "core/pubnub_pubsubapi.h"
#include "core/pubnub_assert.h"
#include "pubnub_internal.h"


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
 * @return Subscribe v2 call operation result.
 */
static enum pubnub_res make_subscribe_request_(
    pbcc_ee_invocation_t*                invocation,
    pbcc_ee_data_t*                      context,
    pbcc_ee_effect_completion_function_t cb);


// ----------------------------------------------
//                   Functions
// ----------------------------------------------

enum pubnub_res pbcc_subscribe_ee_handshake_effect(
    pbcc_ee_invocation_t*                      invocation,
    pbcc_ee_data_t*                            context,
    const pbcc_ee_effect_completion_function_t cb)
{
    return make_subscribe_request_(invocation, context, cb);
}

enum pubnub_res pbcc_subscribe_ee_receive_effect(
    pbcc_ee_invocation_t*                      invocation,
    pbcc_ee_data_t*                            context,
    const pbcc_ee_effect_completion_function_t cb)
{
    return make_subscribe_request_(invocation, context, cb);
}

enum pubnub_res pbcc_subscribe_ee_emit_status_effect(
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
    cb(subscribe_ee->ee, invocation);
    pbcc_ee_data_free(context_copy);

    return PNR_OK;
}

enum pubnub_res pbcc_subscribe_ee_emit_messages_effect(
    pbcc_ee_invocation_t*                      invocation,
    pbcc_ee_data_t*                            context,
    const pbcc_ee_effect_completion_function_t cb)
{
    pbcc_ee_data_t* context_copy = pbcc_ee_data_copy(context);
    const pbcc_subscribe_ee_context_t* ctx = pbcc_ee_data_value(context_copy);
    const pbcc_subscribe_ee_t* subscribe_ee = ctx->pb->core.subscribe_ee;
    pubnub_t* pb = ctx->pb;

    for (struct pubnub_v2_message msg = pubnub_get_v2(pb); msg.payload.size > 0;
         msg                          = pubnub_get_v2(pb)) {
        pbcc_event_listener_emit_message(subscribe_ee->event_listener, msg);
    }

    cb(subscribe_ee->ee, invocation);
    pbcc_ee_data_free(context_copy);

    return PNR_OK;
}

void pbcc_subscribe_ee_cancel_effect(
    pbcc_ee_invocation_t*                      invocation,
    pbcc_ee_data_t*                            context,
    const pbcc_ee_effect_completion_function_t cb)
{
    PUBNUB_ASSERT_OPT(NULL != context);

    pbcc_ee_data_t* context_copy = pbcc_ee_data_copy(context);
    const pbcc_subscribe_ee_context_t* ctx = pbcc_ee_data_value(context_copy);
    const pbcc_subscribe_ee_t* subscribe_ee = ctx->pb->core.subscribe_ee;
    PUBNUB_ASSERT(pb_valid_ctx_ptr(ctx->pb));

    if (PN_CANCEL_FINISHED == pubnub_cancel(ctx->pb))
        cb(subscribe_ee->ee, invocation);
    pbcc_ee_data_free(context_copy);
}

enum pubnub_res make_subscribe_request_(
    pbcc_ee_invocation_t*                      invocation,
    pbcc_ee_data_t*                            context,
    const pbcc_ee_effect_completion_function_t cb)
{
    PUBNUB_ASSERT_OPT(NULL != context);

    pbcc_ee_data_t* context_copy = pbcc_ee_data_copy(context);
    const pbcc_subscribe_ee_context_t* ctx = pbcc_ee_data_value(context_copy);
    pubnub_t* pb = ctx->pb;
    /** Override timetoken stored for next subscription loop. */
    pubnub_mutex_lock(pb->monitor);
    size_t token_len = strlen(pb->core.timetoken);
    memcpy(pb->core.timetoken, ctx->cursor.timetoken, token_len);
    pb->core.timetoken[token_len] = '\0';
    if (ctx->cursor.region > 0) { pb->core.region = ctx->cursor.region; }
    pbpal_mutex_unlock(pb->monitor);

    struct pubnub_subscribe_v2_options opts = pubnub_subscribe_v2_defopts();
    opts.filter_expr = pb->core.subscribe_ee->filter_expr;
    opts.channel_group = pbcc_ee_data_value(ctx->channel_groups);
    opts.heartbeat = pb->core.subscribe_ee->heartbeat;

    const enum pubnub_res rslt = pubnub_subscribe_v2(
        pb,
        pbcc_ee_data_value(ctx->channels),
        opts);
    if (PNR_STARTED == rslt) { cb(pb->core.subscribe_ee->ee, invocation); }
    pbcc_ee_data_free(context_copy);

    return rslt;
}