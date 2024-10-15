/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pbcc_subscribe_event_engine_states.h"

#include "core/pbcc_subscribe_event_engine_transitions.h"
#include "core/pbcc_subscribe_event_engine_effects.h"
#include "core/pbcc_subscribe_event_engine_types.h"
#include "core/pubnub_assert.h"


// ----------------------------------------------
//                   Functions
// ----------------------------------------------

pbcc_ee_state_t* pbcc_unsubscribed_state_alloc()
{
    return pbcc_ee_state_alloc(
        SUBSCRIBE_EE_STATE_UNSUBSCRIBED,
        NULL,
        pbcc_unsubscribed_state_transition_alloc);
}

pbcc_ee_state_t* pbcc_handshaking_state_alloc(
    pbcc_event_engine_t* ee,
    pbcc_ee_data_t*      context)
{
    pbcc_ee_state_t* state = pbcc_ee_state_alloc(
        SUBSCRIBE_EE_STATE_HANDSHAKING,
        context,
        pbcc_handshaking_state_transition_alloc);
    if (NULL == state) { return NULL; }

    pbcc_ee_invocation_t* on_enter = pbcc_ee_invocation_alloc(
        SUBSCRIBE_EE_INVOCATION_HANDSHAKE,
        (pbcc_ee_effect_function_t)pbcc_subscribe_ee_handshake_effect,
        context,
        false);

    /** Make sure that there is no outdated subscribe call. */
    pbcc_ee_invocation_cancel_by_type(ee, SUBSCRIBE_EE_INVOCATION_HANDSHAKE);

    if (NULL == on_enter ||
        PNR_OK != pbcc_ee_state_add_on_enter_invocation(state, on_enter)) {
        if (NULL != on_enter) { pbcc_ee_invocation_free(on_enter); }
        pbcc_ee_state_free(&state);
        return NULL;
    }

    pbcc_ee_invocation_t* on_exit = pbcc_ee_invocation_alloc(
        SUBSCRIBE_EE_INVOCATION_CANCEL,
        (pbcc_ee_effect_function_t)pbcc_subscribe_ee_cancel_effect,
        context,
        true);
    if (NULL == on_exit ||
        PNR_OK != pbcc_ee_state_add_on_exit_invocation(state, on_exit)) {
        if (NULL != on_exit) { pbcc_ee_invocation_free(on_exit); }
        pbcc_ee_state_free(&state);
        return NULL;
    }

    return state;
}

pbcc_ee_state_t* pbcc_handshake_failed_state_alloc(pbcc_ee_data_t* context)
{
    return pbcc_ee_state_alloc(
        SUBSCRIBE_EE_STATE_HANDSHAKE_FAILED,
        context,
        pbcc_handshake_failed_state_transition_alloc);
}

pbcc_ee_state_t* pbcc_handshake_stopped_state_alloc(pbcc_ee_data_t* context)
{
    return pbcc_ee_state_alloc(
        SUBSCRIBE_EE_STATE_HANDSHAKE_STOPPED,
        context,
        pbcc_handshake_stopped_state_transition_alloc);
}

pbcc_ee_state_t* pbcc_receiving_state_alloc(
    pbcc_event_engine_t* ee,
    pbcc_ee_data_t*      context)
{
    pbcc_ee_state_t* state = pbcc_ee_state_alloc(
        SUBSCRIBE_EE_STATE_RECEIVING,
        context,
        pbcc_receiving_state_transition_alloc);
    if (NULL == state) { return NULL; }

    pbcc_ee_invocation_t* on_enter = pbcc_ee_invocation_alloc(
        SUBSCRIBE_EE_INVOCATION_RECEIVE,
        (pbcc_ee_effect_function_t)pbcc_subscribe_ee_receive_effect,
        context,
        false);

    /** Make sure that there is no outdated subscribe call. */
    pbcc_ee_invocation_cancel_by_type(ee, SUBSCRIBE_EE_INVOCATION_RECEIVE);

    if (NULL == on_enter ||
        PNR_OK != pbcc_ee_state_add_on_enter_invocation(state, on_enter)) {
        if (NULL != on_enter) { pbcc_ee_invocation_free(on_enter); }
        pbcc_ee_state_free(&state);
        return NULL;
    }

    pbcc_ee_invocation_t* on_exit = pbcc_ee_invocation_alloc(
        SUBSCRIBE_EE_INVOCATION_CANCEL,
        (pbcc_ee_effect_function_t)pbcc_subscribe_ee_cancel_effect,
        context,
        true);
    if (NULL == on_exit ||
        PNR_OK != pbcc_ee_state_add_on_exit_invocation(state, on_exit)) {
        if (NULL != on_exit) { pbcc_ee_invocation_free(on_exit); }
        pbcc_ee_state_free(&state);
        return NULL;
    }

    return state;
}

pbcc_ee_state_t* pbcc_receive_failed_state_alloc(pbcc_ee_data_t* context)
{
    return pbcc_ee_state_alloc(
        SUBSCRIBE_EE_STATE_RECEIVE_FAILED,
        context,
        pbcc_receiving_failed_state_transition_alloc);
}

pbcc_ee_state_t* pbcc_receive_stopped_state_alloc(pbcc_ee_data_t* context)
{
    return pbcc_ee_state_alloc(
        SUBSCRIBE_EE_STATE_RECEIVE_STOPPED,
        context,
        pbcc_receiving_stopped_state_transition_alloc);
}