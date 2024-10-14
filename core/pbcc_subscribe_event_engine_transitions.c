/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pbcc_subscribe_event_engine_transitions.h"

#include "core/pbcc_subscribe_event_engine_effects.h"
#include "core/pbcc_subscribe_event_engine_states.h"
#include "core/pbcc_subscribe_event_engine_types.h"
#include "core/pubnub_assert.h"
#include "pubnub_internal.h"
#include "core/pubnub_log.h"


// ----------------------------------------------
//              Function prototypes
// ----------------------------------------------

/**
 * @brief Create transition details for Event Engine.
 *
 * @param ee                Pointer to the Event Engine which requested for
 *                          transition details.
 * @param target_state_type Type of target Subscribe Event Engine state.
 * @param context           Subscribe Event Engine data which should be passed
 *                          to the next state and transition invocations.
 * @param invocations       Pointer to the list of invocations which should be
 *                          called before Event Engine will enter new state.
 * @return Pointer to the ready to use transition details or `NULL` in case of
 *         insufficient memory error. The returned pointer must be passed to the
 *         `pbcc_ee_transition_free` to avoid a memory leak.
 */
static pbcc_ee_transition_t* pbcc_transition_alloc_(
    pbcc_event_engine_t*    ee,
    pbcc_subscribe_ee_state target_state_type,
    pbcc_ee_data_t*         context,
    pbarray_t*              invocations);


// ----------------------------------------------
//                   Functions
// ----------------------------------------------

pbcc_ee_transition_t* pbcc_unsubscribed_state_transition_alloc(
    pbcc_event_engine_t*   ee,
    const pbcc_ee_state_t* current_state,
    const pbcc_ee_event_t* event)
{
    if (pbcc_ee_state_type(current_state) != SUBSCRIBE_EE_STATE_UNSUBSCRIBED ||
        (event->type != SUBSCRIBE_EE_EVENT_SUBSCRIPTION_CHANGED &&
         event->type != SUBSCRIBE_EE_EVENT_SUBSCRIPTION_RESTORED)) {
        PUBNUB_LOG_INFO("pbcc_unsubscribed_state_transition_alloc: can't "
                        "handle transition for %d event type\n",
                        event->type);
        return pbcc_transition_alloc_(ee, SUBSCRIBE_EE_STATE_NONE, NULL, NULL);
    }

    return pbcc_transition_alloc_(ee,
                                  SUBSCRIBE_EE_STATE_HANDSHAKING,
                                  event->data,
                                  NULL);
}

pbcc_ee_transition_t* pbcc_handshaking_state_transition_alloc(
    pbcc_event_engine_t*   ee,
    const pbcc_ee_state_t* current_state,
    const pbcc_ee_event_t* event)
{
    if (pbcc_ee_state_type(current_state) != SUBSCRIBE_EE_STATE_HANDSHAKING)
        return pbcc_transition_alloc_(ee, SUBSCRIBE_EE_STATE_NONE, NULL, NULL);

    pbcc_subscribe_ee_state target_state_type = SUBSCRIBE_EE_STATE_NONE;
    pbcc_ee_data_t*         data              = event->data;
    pbarray_t*              invocations       = NULL;

    switch (event->type) {
    case SUBSCRIBE_EE_EVENT_SUBSCRIPTION_CHANGED: {
        PUBNUB_ASSERT_OPT(NULL != data);

        const pbcc_subscribe_ee_context_t* context = pbcc_ee_data_value(data);
        const char*                        channel_groups =
            pbcc_ee_data_value(context->channel_groups);
        const char* channels = pbcc_ee_data_value(context->channels);

        if (NULL != context && 0 == strlen(channels) &&
            0 == strlen(channel_groups)) {
            target_state_type = SUBSCRIBE_EE_STATE_UNSUBSCRIBED;
            data              = NULL;
        }
        else { target_state_type = SUBSCRIBE_EE_STATE_HANDSHAKING; }
    }
    break;
    case SUBSCRIBE_EE_EVENT_SUBSCRIPTION_RESTORED:
        target_state_type = SUBSCRIBE_EE_STATE_HANDSHAKING;
        break;
    case SUBSCRIBE_EE_EVENT_HANDSHAKE_FAILURE:
    case SUBSCRIBE_EE_EVENT_HANDSHAKE_SUCCESS: {
        const pbcc_subscribe_ee_context_t* context = pbcc_ee_data_value(data);
        pbcc_subscribe_ee_t* subscribe_ee = context->pb->core.subscribe_ee;
        target_state_type = event->type ==
                            SUBSCRIBE_EE_EVENT_HANDSHAKE_SUCCESS
                                ? SUBSCRIBE_EE_STATE_RECEIVING
                                : SUBSCRIBE_EE_STATE_HANDSHAKE_FAILED;
        invocations = pbarray_alloc(
            1,
            PBARRAY_RESIZE_NONE,
            PBARRAY_GENERIC_CONTENT_TYPE,
            (pbarray_element_free)pbcc_ee_invocation_free);
        pbcc_ee_invocation_t* invocation = pbcc_ee_invocation_alloc(
            SUBSCRIBE_EE_INVOCATION_EMIT_STATUS,
            (pbcc_ee_effect_function_t)pbcc_subscribe_ee_emit_status_effect,
            data,
            false);

        if (NULL == invocations || NULL == invocation) {
            PUBNUB_LOG_ERROR("pbcc_handshaking_state_transition_alloc: failed "
                "to allocate memory for invocations\n");
            if (NULL != invocations) { pbarray_free(&invocations); }
            if (NULL != invocation) { pbcc_ee_invocation_free(invocation); }
            return NULL;
        }

        if (PBAR_OK != pbarray_add(invocations, invocation)) {
            PUBNUB_LOG_ERROR("pbcc_handshaking_state_transition_alloc: failed "
                "to allocate memory for invocation entry\n");
            pbcc_ee_invocation_free(invocation);
            pbarray_free(&invocations);
            return NULL;
        }

        /** Update latest Subscribe Event Engine subscription status. */
        pubnub_mutex_lock(subscribe_ee->mutw);
        subscribe_ee->status = SUBSCRIBE_EE_STATE_RECEIVING == target_state_type
                                   ? PNSS_SUBSCRIPTION_STATUS_CONNECTED
                                   : PNSS_SUBSCRIPTION_STATUS_CONNECTION_ERROR;
        pubnub_mutex_unlock(subscribe_ee->mutw);
    }
    break;
    case SUBSCRIBE_EE_EVENT_DISCONNECT:
        target_state_type = SUBSCRIBE_EE_STATE_HANDSHAKE_STOPPED;
        break;
    case SUBSCRIBE_EE_EVENT_UNSUBSCRIBE_ALL:
        target_state_type = SUBSCRIBE_EE_STATE_UNSUBSCRIBED;
        break;
    default:
        invocations = NULL;
        data = NULL;
        break;
    }

    if (SUBSCRIBE_EE_STATE_NONE == target_state_type) {
        PUBNUB_LOG_INFO("pbcc_handshaking_state_transition_alloc: can't handle "
                        "transition for %d event type\n",
                        event->type);
    }

    return pbcc_transition_alloc_(ee, target_state_type, data, invocations);
}

pbcc_ee_transition_t* pbcc_handshake_failed_state_transition_alloc(
    pbcc_event_engine_t*   ee,
    const pbcc_ee_state_t* current_state,
    const pbcc_ee_event_t* event)
{
    if (pbcc_ee_state_type(current_state) !=
        SUBSCRIBE_EE_STATE_HANDSHAKE_FAILED) {
        return pbcc_transition_alloc_(ee, SUBSCRIBE_EE_STATE_NONE, NULL, NULL);
    }

    pbcc_subscribe_ee_state target_state_type = SUBSCRIBE_EE_STATE_NONE;
    pbcc_ee_data_t*         data              = event->data;

    switch (event->type) {
    case SUBSCRIBE_EE_EVENT_SUBSCRIPTION_CHANGED:
    case SUBSCRIBE_EE_EVENT_SUBSCRIPTION_RESTORED: {
        PUBNUB_ASSERT_OPT(NULL != data);

        const pbcc_subscribe_ee_context_t* context = pbcc_ee_data_value(data);
        const char*                        channel_groups =
            pbcc_ee_data_value(context->channel_groups);
        const char* channels = pbcc_ee_data_value(context->channels);

        if (NULL != context && (NULL == channels || 0 == strlen(channels)) &&
            (NULL == channel_groups || 0 == strlen(channel_groups))) {
            target_state_type = SUBSCRIBE_EE_STATE_UNSUBSCRIBED;
        }
        else { target_state_type = SUBSCRIBE_EE_STATE_HANDSHAKING; }
    }
    break;
    case SUBSCRIBE_EE_EVENT_RECONNECT:
        target_state_type = SUBSCRIBE_EE_STATE_HANDSHAKING;
        break;
    case SUBSCRIBE_EE_EVENT_UNSUBSCRIBE_ALL:
        target_state_type = SUBSCRIBE_EE_STATE_UNSUBSCRIBED;
        break;
    default:
        data = NULL;
        break;
    }

    if (SUBSCRIBE_EE_STATE_NONE == target_state_type) {
        PUBNUB_LOG_INFO("pbcc_handshake_failed_state_transition_alloc: can't "
                        "handle transition for %d event type\n",
                        event->type);
    }

    return pbcc_transition_alloc_(ee, target_state_type, data, NULL);
}

pbcc_ee_transition_t* pbcc_handshake_stopped_state_transition_alloc(
    pbcc_event_engine_t*   ee,
    const pbcc_ee_state_t* current_state,
    const pbcc_ee_event_t* event)
{
    if (pbcc_ee_state_type(current_state) !=
        SUBSCRIBE_EE_STATE_HANDSHAKE_STOPPED) {
        return pbcc_transition_alloc_(ee, SUBSCRIBE_EE_STATE_NONE, NULL, NULL);
    }

    pbcc_subscribe_ee_state target_state_type = SUBSCRIBE_EE_STATE_NONE;

    switch (event->type) {
    case SUBSCRIBE_EE_EVENT_SUBSCRIPTION_CHANGED:
    case SUBSCRIBE_EE_EVENT_SUBSCRIPTION_RESTORED:
        target_state_type = SUBSCRIBE_EE_STATE_HANDSHAKE_STOPPED;
        break;
    case SUBSCRIBE_EE_EVENT_RECONNECT:
        target_state_type = SUBSCRIBE_EE_STATE_HANDSHAKING;
        break;
    case SUBSCRIBE_EE_EVENT_UNSUBSCRIBE_ALL:
        target_state_type = SUBSCRIBE_EE_STATE_UNSUBSCRIBED;
        break;
    default:
        break;
    }

    if (SUBSCRIBE_EE_STATE_NONE == target_state_type) {
        PUBNUB_LOG_INFO("pbcc_handshake_stopped_state_transition_alloc: can't "
                        "handle transition for %d event type\n",
                        event->type);
    }

    return pbcc_transition_alloc_(ee, target_state_type, event->data, NULL);
}

pbcc_ee_transition_t* pbcc_receiving_state_transition_alloc(
    pbcc_event_engine_t*   ee,
    const pbcc_ee_state_t* current_state,
    const pbcc_ee_event_t* event)
{
    if (pbcc_ee_state_type(current_state) != SUBSCRIBE_EE_STATE_RECEIVING)
        return pbcc_transition_alloc_(ee, SUBSCRIBE_EE_STATE_NONE, NULL, NULL);

    pbcc_subscribe_ee_state    target_state_type = SUBSCRIBE_EE_STATE_NONE;
    pubnub_subscription_status status            = -1;
    pbarray_t*                 invocations       = NULL;
    pbcc_ee_invocation_t*      invocation        = NULL;
    pbcc_ee_data_t*            data              = event->data;

    switch (event->type) {
    case SUBSCRIBE_EE_EVENT_SUBSCRIPTION_CHANGED:
    case SUBSCRIBE_EE_EVENT_SUBSCRIPTION_RESTORED: {
        const pbcc_subscribe_ee_context_t* context = (
                pbcc_subscribe_ee_context_t*)
            pbcc_ee_data_value(data);
        const char* channel_groups =
            pbcc_ee_data_value(context->channel_groups);
        const char* channels = pbcc_ee_data_value(context->channels);

        if (NULL != context && (NULL == channels || 0 == strlen(channels)) &&
            (NULL == channel_groups || 0 == strlen(channel_groups))) {
            target_state_type = SUBSCRIBE_EE_STATE_UNSUBSCRIBED;
            status            = PNSS_SUBSCRIPTION_STATUS_DISCONNECTED;
        }
        else {
            target_state_type = SUBSCRIBE_EE_STATE_RECEIVING;
            status            = PNSS_SUBSCRIPTION_STATUS_SUBSCRIPTION_CHANGED;
        }
    }
    break;
    case SUBSCRIBE_EE_EVENT_RECEIVE_SUCCESS:
        target_state_type = SUBSCRIBE_EE_STATE_RECEIVING;
        break;
    case SUBSCRIBE_EE_EVENT_RECEIVE_FAILURE:
        target_state_type = SUBSCRIBE_EE_STATE_RECEIVE_FAILED;
        status            = PNSS_SUBSCRIPTION_STATUS_DISCONNECTED_UNEXPECTEDLY;
        break;
    case SUBSCRIBE_EE_EVENT_DISCONNECT:
        target_state_type = SUBSCRIBE_EE_STATE_RECEIVE_STOPPED;
        status            = PNSS_SUBSCRIPTION_STATUS_DISCONNECTED;
        break;
    case SUBSCRIBE_EE_EVENT_UNSUBSCRIBE_ALL:
        target_state_type = SUBSCRIBE_EE_STATE_UNSUBSCRIBED;
        status            = PNSS_SUBSCRIPTION_STATUS_DISCONNECTED;
        break;
    default:
        data = NULL;
        break;
    }

    if (SUBSCRIBE_EE_EVENT_RECEIVE_SUCCESS == event->type || -1 != status) {
        invocations = pbarray_alloc(
            1,
            PBARRAY_RESIZE_NONE,
            PBARRAY_GENERIC_CONTENT_TYPE,
            (pbarray_element_free)pbcc_ee_invocation_free);

        if (NULL == invocations) {
            PUBNUB_LOG_ERROR("pbcc_receiving_state_transition_alloc: failed "
                "to allocate memory for invocations\n");
            return NULL;
        }
    }

    if (-1 != status) {
        const pbcc_subscribe_ee_context_t* context = pbcc_ee_data_value(data);
        pbcc_subscribe_ee_t* subscribe_ee = context->pb->core.subscribe_ee;
        invocation = pbcc_ee_invocation_alloc(
            SUBSCRIBE_EE_INVOCATION_EMIT_STATUS,
            (pbcc_ee_effect_function_t)pbcc_subscribe_ee_emit_status_effect,
            data,
            false);

        /** Update latest Subscribe Event Engine subscription status. */
        pubnub_mutex_lock(subscribe_ee->mutw);
        subscribe_ee->status = status;
        pubnub_mutex_unlock(subscribe_ee->mutw);
    }

    if (SUBSCRIBE_EE_EVENT_RECEIVE_SUCCESS == event->type) {
        invocation = pbcc_ee_invocation_alloc(
            SUBSCRIBE_EE_INVOCATION_EMIT_MESSAGE,
            (pbcc_ee_effect_function_t)pbcc_subscribe_ee_emit_messages_effect,
            data,
            false);
    }

    if (NULL != invocation) {
        if (PBAR_OK != pbarray_add(invocations, invocation)) {
            PUBNUB_LOG_ERROR("pbcc_receiving_state_transition_alloc: failed "
                "to allocate memory for invocation entry\n");
            pbcc_ee_invocation_free(invocation);
            pbarray_free(&invocations);
            return NULL;
        }
    }
    else if (SUBSCRIBE_EE_EVENT_RECEIVE_SUCCESS == event->type ||
             -1 != status) {
        PUBNUB_LOG_ERROR("pbcc_receiving_state_transition_alloc: failed "
            "to allocate memory for invocation\n");
        if (NULL != invocations) { pbarray_free(&invocations); }
        return NULL;
    }

    if (SUBSCRIBE_EE_STATE_NONE == target_state_type) {
        PUBNUB_LOG_INFO("pbcc_receiving_state_transition_alloc: can't handle "
                        "transition for %d event type\n",
                        event->type);
    }

    return pbcc_transition_alloc_(ee,
                                  target_state_type,
                                  data,
                                  invocations);
}

pbcc_ee_transition_t* pbcc_receiving_failed_state_transition_alloc(
    pbcc_event_engine_t*   ee,
    const pbcc_ee_state_t* current_state,
    const pbcc_ee_event_t* event)
{
    if (pbcc_ee_state_type(current_state) !=
        SUBSCRIBE_EE_STATE_RECEIVE_FAILED) {
        return pbcc_transition_alloc_(ee, SUBSCRIBE_EE_STATE_NONE, NULL, NULL);
    }

    pbcc_subscribe_ee_state target_state_type = SUBSCRIBE_EE_STATE_NONE;

    switch (event->type) {
    case SUBSCRIBE_EE_EVENT_SUBSCRIPTION_CHANGED:
    case SUBSCRIBE_EE_EVENT_SUBSCRIPTION_RESTORED:
    case SUBSCRIBE_EE_EVENT_RECONNECT:
        target_state_type = SUBSCRIBE_EE_STATE_HANDSHAKING;
        break;
    case SUBSCRIBE_EE_EVENT_UNSUBSCRIBE_ALL:
        target_state_type = SUBSCRIBE_EE_STATE_UNSUBSCRIBED;
        break;
    default:
        break;
    }

    if (SUBSCRIBE_EE_STATE_NONE == target_state_type) {
        PUBNUB_LOG_INFO("pbcc_receiving_failed_state_transition_alloc: can't "
                        "handle transition for %d event type\n",
                        event->type);
    }

    return pbcc_transition_alloc_(ee, target_state_type, event->data, NULL);
}

pbcc_ee_transition_t* pbcc_receiving_stopped_state_transition_alloc(
    pbcc_event_engine_t*   ee,
    const pbcc_ee_state_t* current_state,
    const pbcc_ee_event_t* event)
{
    if (pbcc_ee_state_type(current_state) !=
        SUBSCRIBE_EE_STATE_RECEIVE_STOPPED) {
        return pbcc_transition_alloc_(ee, SUBSCRIBE_EE_STATE_NONE, NULL, NULL);
    }

    pbcc_subscribe_ee_state target_state_type = SUBSCRIBE_EE_STATE_NONE;

    switch (event->type) {
    case SUBSCRIBE_EE_EVENT_SUBSCRIPTION_CHANGED:
    case SUBSCRIBE_EE_EVENT_SUBSCRIPTION_RESTORED:
        target_state_type = SUBSCRIBE_EE_STATE_RECEIVE_STOPPED;
        break;
    case SUBSCRIBE_EE_EVENT_RECONNECT:
        target_state_type = SUBSCRIBE_EE_STATE_HANDSHAKING;
        break;
    case SUBSCRIBE_EE_EVENT_UNSUBSCRIBE_ALL:
        target_state_type = SUBSCRIBE_EE_STATE_UNSUBSCRIBED;
        break;
    default:
        break;
    }

    if (SUBSCRIBE_EE_STATE_NONE == target_state_type) {
        PUBNUB_LOG_INFO("pbcc_receiving_stopped_state_transition_alloc: can't "
                        "handle transition for %d event type\n",
                        event->type);
    }

    return pbcc_transition_alloc_(ee, target_state_type, event->data, NULL);
}

pbcc_ee_transition_t* pbcc_transition_alloc_(
    pbcc_event_engine_t*          ee,
    const pbcc_subscribe_ee_state target_state_type,
    pbcc_ee_data_t*               context,
    pbarray_t*                    invocations)
{
    pbcc_ee_state_t* target_state  = NULL;
    pbcc_ee_data_t*  state_context = NULL;

    if (NULL != context && SUBSCRIBE_EE_STATE_NONE != target_state_type &&
        SUBSCRIBE_EE_STATE_UNSUBSCRIBED != target_state_type) {
        state_context = context;
    }

    switch (target_state_type) {
    case SUBSCRIBE_EE_STATE_UNSUBSCRIBED:
        target_state = pbcc_unsubscribed_state_alloc();
        break;
    case SUBSCRIBE_EE_STATE_HANDSHAKING:
        target_state = pbcc_handshaking_state_alloc(ee, state_context);
        break;
    case SUBSCRIBE_EE_STATE_HANDSHAKE_FAILED:
        target_state = pbcc_handshake_failed_state_alloc(state_context);
        break;
    case SUBSCRIBE_EE_STATE_HANDSHAKE_STOPPED:
        target_state = pbcc_handshake_stopped_state_alloc(state_context);
        break;
    case SUBSCRIBE_EE_STATE_RECEIVING:
        target_state = pbcc_receiving_state_alloc(ee, state_context);
        break;
    case SUBSCRIBE_EE_STATE_RECEIVE_FAILED:
        target_state = pbcc_receive_failed_state_alloc(state_context);
        break;
    case SUBSCRIBE_EE_STATE_RECEIVE_STOPPED:
        target_state = pbcc_receive_stopped_state_alloc(state_context);
        break;
    default:
        PUBNUB_LOG_ERROR("pbcc_transition_alloc: unknown target state type\n");
        break;
    }

    if (NULL == target_state && SUBSCRIBE_EE_STATE_NONE != target_state_type) {
        PUBNUB_LOG_ERROR("pbcc_transition_alloc: failed to allocate memory for "
            "state object\n");
        return NULL;
    }

    pbcc_ee_transition_t* transition = pbcc_ee_transition_alloc(
        ee,
        target_state,
        invocations);
    if (NULL == transition) {
        PUBNUB_LOG_ERROR("pbcc_transition_alloc: failed to allocate memory for "
            "transition object\n");
        if (NULL != target_state) { pbcc_ee_state_free(&target_state); }
    }

    return transition;
}