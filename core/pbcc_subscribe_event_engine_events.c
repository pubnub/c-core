/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pbcc_subscribe_event_engine_events.h"

#include <string.h>

#include "core/pubnub_log.h"


// ----------------------------------------------
//              Function prototypes
// ----------------------------------------------

/**
 * @brief Create event for underlying Event Engine.
 *
 * @param ee                      Pointer to the Subscribe Event Engine for
 *                                which will process the event.
 * @param event                   Type of Subscribe Event Engine event.
 * @param [in,out] channels       Pointer to the byte sting of comma-separated
 *                                channels from which PubNub client should
 *                                receive real-time updates.
 * @param [in,out] channel_groups Pointer to the byte string of comma-separated
 *                                channel groups from which PubNub client should
 *                                receive real-time updates.
 * @param cursor                  Time token, which should be used with next
 *                                subscription REST API loop call.
 * @param reason                  Subscription processing result code which
 *                                explain failure reason.
 * @param sent_by_ee              Whether event created per Event Engine request
 *                                or not.
 * @return Pointer to requested `event` type, which will be processed by the
 *         Subscribe Event Engine to get transition instructions from the
 *         current state.
 */
static pbcc_ee_event_t* pbcc_subscribe_ee_event_alloc_(
    pbcc_subscribe_ee_t*             ee,
    pbcc_subscribe_ee_event          event,
    char**                           channels,
    char**                           channel_groups,
    const pubnub_subscribe_cursor_t* cursor,
    const enum pubnub_res*           reason,
    bool                             sent_by_ee);

/**
 * @brief Create Subscribe Event Engine context object.
 *
 * @param pb                      PubNub context, which should be used for
 *                                effects execution by Subscribe Event Engine
 *                                effects dispatcher.
 * @param [in,out] channels       Pointer to the byte string with
 *                                comma-separated of channels which should be
 *                                passed between states in context object.
 * @param [in,out] channel_groups Pointer to the byte string with
 *                                comma-separated of channel groups which should
 *                                be passed between states in context object.
 * @return Pointer to the ready to use Subscribe Event Engine context object, or
 *         `NULL` in case of insufficient memory error.
 *         The returned pointer must be passed to the
 *         `pbcc_subscribe_ee_context_free_` to avoid a memory leak.
 */
pbcc_subscribe_ee_context_t* pbcc_subscribe_ee_context_alloc_(
    pubnub_t* pb,
    char**    channels,
    char**    channel_groups);

/**
 * @brief Copy source context with `channels` and `channel_groups` override.
*
 * @param pb                      PubNub context, which should be used for
 *                                effects execution by Subscribe Event Engine
 *                                effects dispatcher.
 * @param ctx                     Pointer to the context, from which data will
 *                                be copied to the next Subscribe Event Engine
 *                                context.
 * @param [in,out] channels       Pointer to the byte string with
 *                                comma-separated channels which should be used
 *                                instead of `channels` from provided context.
 * @param [in,out] channel_groups Pointer to the byte string with
 *                                comma-separated channel groups which should be
 *                                used instead of `channel_groups` from provided
 *                                context.
 * @param copy_subscribables      Whether subscribables should be copied from
 *                                the context or not.
 * @return Pointer to the ready to use Subscribe Event Engine context created
 *         from the data of source context, or `NULL` in case of insufficient
 *         memory error.
 *         The returned pointer must be passed to the
 *         `pbcc_subscribe_ee_context_free_` to avoid a memory leak.
 */
pbcc_subscribe_ee_context_t* pbcc_subscribe_ee_context_copy_(
    pubnub_t*                          pb,
    const pbcc_subscribe_ee_context_t* ctx,
    char**                             channels,
    char**                             channel_groups,
    bool                               copy_subscribables);

/**
 * @brief Clean up resources used by subscription context.
 *
 * @param ctx Pointer to the context, which should free up resources.
 */
void pbcc_subscribe_ee_context_free_(pbcc_subscribe_ee_context_t* ctx);


// ----------------------------------------------
//                   Functions
// ----------------------------------------------

pbcc_ee_event_t* pbcc_subscription_changed_event_alloc(
    pbcc_subscribe_ee_t* ee,
    char**               channels,
    char**               channel_groups,
    const bool           sent_by_ee)
{
    if (NULL != channels && NULL != *channels && 0 == strlen(*channels)) {
        free(*channels);
        *channels = NULL;
    }
    if (NULL != channel_groups && NULL != *channel_groups &&
        0 == strlen(*channel_groups)) {
        free(*channel_groups);
        *channel_groups = NULL;
    }

    const pbcc_subscribe_ee_event type =
        SUBSCRIBE_EE_EVENT_SUBSCRIPTION_CHANGED;
    return pbcc_subscribe_ee_event_alloc_(ee,
                                          type,
                                          channels,
                                          channel_groups,
                                          NULL,
                                          NULL,
                                          sent_by_ee);
}

pbcc_ee_event_t* pbcc_subscription_restored_event_alloc(
    pbcc_subscribe_ee_t*            ee,
    char**                          channels,
    char**                          channel_groups,
    const pubnub_subscribe_cursor_t cursor,
    const bool                      sent_by_ee)
{
    if (NULL != channels && NULL != *channels && 0 == strlen(*channels)) {
        free(*channels);
        *channels = NULL;
    }
    if (NULL != channel_groups && NULL != *channel_groups &&
        0 == strlen(*channel_groups)) {
        free(*channel_groups);
        *channel_groups = NULL;
    }

    const pbcc_subscribe_ee_event type =
        SUBSCRIBE_EE_EVENT_SUBSCRIPTION_RESTORED;
    return pbcc_subscribe_ee_event_alloc_(ee,
                                          type,
                                          channels,
                                          channel_groups,
                                          &cursor,
                                          NULL,
                                          sent_by_ee);
}

pbcc_ee_event_t* pbcc_handshake_success_event_alloc(
    pbcc_subscribe_ee_t*            ee,
    const pubnub_subscribe_cursor_t cursor)
{
    const pbcc_subscribe_ee_event type = SUBSCRIBE_EE_EVENT_HANDSHAKE_SUCCESS;
    return pbcc_subscribe_ee_event_alloc_(ee,
                                          type,
                                          NULL,
                                          NULL,
                                          &cursor,
                                          NULL,
                                          true);
}

pbcc_ee_event_t* pbcc_handshake_failure_event_alloc(
    pbcc_subscribe_ee_t*  ee,
    const enum pubnub_res reason)
{
    const pbcc_subscribe_ee_event type = SUBSCRIBE_EE_EVENT_HANDSHAKE_FAILURE;
    return pbcc_subscribe_ee_event_alloc_(ee,
                                          type,
                                          NULL,
                                          NULL,
                                          NULL,
                                          &reason,
                                          true);
}

pbcc_ee_event_t* pbcc_receive_success_event_alloc(
    pbcc_subscribe_ee_t*            ee,
    const pubnub_subscribe_cursor_t cursor)
{
    const pbcc_subscribe_ee_event type = SUBSCRIBE_EE_EVENT_RECEIVE_SUCCESS;
    return pbcc_subscribe_ee_event_alloc_(ee,
                                          type,
                                          NULL,
                                          NULL,
                                          &cursor,
                                          NULL,
                                          true);
}

pbcc_ee_event_t* pbcc_receive_failure_event_alloc(
    pbcc_subscribe_ee_t*  ee,
    const enum pubnub_res reason)
{
    const pbcc_subscribe_ee_event type = SUBSCRIBE_EE_EVENT_RECEIVE_FAILURE;
    return pbcc_subscribe_ee_event_alloc_(ee,
                                          type,
                                          NULL,
                                          NULL,
                                          NULL,
                                          &reason,
                                          true);
}

pbcc_ee_event_t* pbcc_disconnect_event_alloc(pbcc_subscribe_ee_t* ee)
{
    const pbcc_subscribe_ee_event type = SUBSCRIBE_EE_EVENT_DISCONNECT;
    return pbcc_subscribe_ee_event_alloc_(ee,
                                          type,
                                          NULL,
                                          NULL,
                                          NULL,
                                          NULL,
                                          true);
}

pbcc_ee_event_t* pbcc_reconnect_event_alloc(
    pbcc_subscribe_ee_t*            ee,
    const pubnub_subscribe_cursor_t cursor)
{
    const pbcc_subscribe_ee_event type = SUBSCRIBE_EE_EVENT_RECONNECT;
    return pbcc_subscribe_ee_event_alloc_(ee,
                                          type,
                                          NULL,
                                          NULL,
                                          &cursor,
                                          NULL,
                                          true);
}

pbcc_ee_event_t* pbcc_unsubscribe_all_event_alloc(
    pbcc_subscribe_ee_t* ee,
    char**               channels,
    char**               channel_groups)
{
    if (NULL != channels && NULL != *channels && 0 == strlen(*channels)) {
        free(*channels);
        *channels = NULL;
    }
    if (NULL != channel_groups && NULL != *channel_groups &&
        0 == strlen(*channel_groups)) {
        free(*channel_groups);
        *channel_groups = NULL;
    }

    const pbcc_subscribe_ee_event type = SUBSCRIBE_EE_EVENT_UNSUBSCRIBE_ALL;
    return pbcc_subscribe_ee_event_alloc_(ee,
                                          type,
                                          channels,
                                          channel_groups,
                                          NULL,
                                          NULL,
                                          false);
}

pbcc_ee_event_t* pbcc_subscribe_ee_event_alloc_(
    pbcc_subscribe_ee_t*             ee,
    const pbcc_subscribe_ee_event    event,
    char**                           channels,
    char**                           channel_groups,
    const pubnub_subscribe_cursor_t* cursor,
    const enum pubnub_res*           reason,
    const bool                       sent_by_ee)
{
    pubnub_mutex_lock(ee->mutw);
    pbcc_ee_data_t* current_state_data =
        pbcc_subscribe_ee_current_state_context(ee);
    const pbcc_subscribe_ee_context_t* current_context =
        pbcc_ee_data_value(current_state_data);
    const bool copy_subscribables =
        event != SUBSCRIBE_EE_EVENT_SUBSCRIPTION_CHANGED &&
        event != SUBSCRIBE_EE_EVENT_SUBSCRIPTION_RESTORED;

    pbcc_subscribe_ee_context_t* ctx = pbcc_subscribe_ee_context_copy_(
        ee->pb,
        current_context,
        channels,
        channel_groups,
        copy_subscribables);
    /** For subscription change / restore we need to send heartbeat. */
    ctx->send_heartbeat = !copy_subscribables && !sent_by_ee;
    pbcc_ee_data_free(current_state_data);

    if (NULL == ctx) {
        pubnub_mutex_unlock(ee->mutw);
        return NULL;
    }
    if (NULL != cursor) { ctx->cursor = *cursor; }
    if (NULL != reason) { ctx->reason = *reason; }

    pbcc_ee_data_t* data = pbcc_ee_data_alloc(
        ctx,
        (pbcc_ee_data_free_function_t)pbcc_subscribe_ee_context_free_);
    if (NULL == data) {
        pubnub_mutex_unlock(ee->mutw);
        pbcc_subscribe_ee_context_free_(ctx);
        return NULL;
    }
    pubnub_mutex_unlock(ee->mutw);

    pbcc_ee_event_t* event_object = pbcc_ee_event_alloc(event, data);
    if (NULL == event_object) {
        pbcc_ee_data_free(data);
        return NULL;
    }

    return event_object;
}

pbcc_subscribe_ee_context_t* pbcc_subscribe_ee_context_alloc_(
    pubnub_t* pb,
    char**    channels,
    char**    channel_groups)
{
    pbcc_subscribe_ee_context_t* context =
        malloc(sizeof(pbcc_subscribe_ee_context_t));
    if (NULL == context) {
        PUBNUB_LOG_ERROR(
            "pbcc_subscribe_ee_context_alloc_: failed to allocate memory\n");
        if (NULL != channels && NULL != *channels) {
            free(*channels);
            *channels = NULL;
        }
        if (NULL != channel_groups && NULL != *channel_groups) {
            free(*channel_groups);
            *channel_groups = NULL;
        }
        return NULL;
    }

    context->pb             = pb;
    context->reason         = PNR_OK;
    context->channels       = NULL;
    context->channel_groups = NULL;
    context->send_heartbeat = false;
    context->cursor = pubnub_subscribe_cursor(NULL);
    if (NULL != channels && NULL != *channels)
        context->channels = pbcc_ee_data_alloc(*channels, free);
    if (NULL != channel_groups && NULL != *channel_groups) {
        context->channel_groups =
            pbcc_ee_data_alloc(*channel_groups, free);
    }

    return context;
}

pbcc_subscribe_ee_context_t* pbcc_subscribe_ee_context_copy_(
    pubnub_t*                          pb,
    const pbcc_subscribe_ee_context_t* ctx,
    char**                             channels,
    char**                             channel_groups,
    const bool                         copy_subscribables)
{
    pbcc_subscribe_ee_context_t* context = pbcc_subscribe_ee_context_alloc_(
        pb,
        channels,
        channel_groups);
    if (NULL == context) { return NULL; }
    if (NULL == ctx) { return context; }

    if (copy_subscribables) {
        if (NULL == context->channels && NULL != ctx->channels)
            context->channels = pbcc_ee_data_copy(ctx->channels);
        if (NULL == context->channel_groups && NULL != ctx->channel_groups)
            context->channel_groups = pbcc_ee_data_copy(ctx->channel_groups);
    }
    context->cursor = ctx->cursor;

    return context;
}

void pbcc_subscribe_ee_context_free_(pbcc_subscribe_ee_context_t* ctx)
{
    if (NULL == ctx) { return; }

    if (NULL != ctx->channels) { pbcc_ee_data_free(ctx->channels); }
    if (NULL != ctx->channel_groups) { pbcc_ee_data_free(ctx->channel_groups); }
    free(ctx);
}