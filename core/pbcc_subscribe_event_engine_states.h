/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#ifndef PBCC_SUBSCRIBE_EVENT_ENGINE_STATES_H
#define PBCC_SUBSCRIBE_EVENT_ENGINE_STATES_H
#if PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE
#if !PUBNUB_USE_SUBSCRIBE_V2
#error Subscribe event engine requires subscribe v2 API, so you must define PUBNUB_USE_SUBSCRIBE_V2=1
#endif // #if !PUBNUB_USE_SUBSCRIBE_V2


/**
 * @file  pbcc_subscribe_event_engine_states.h
 * @brief Subscribe Event Engine states.
 */

#include "core/pbcc_event_engine.h"


// ----------------------------------------------
//                   Functions
// ----------------------------------------------

/**
 * @brief Inactive Subscribe Event Engine state.
 *
 * State with which Subscribe Event Engine will be initialized and end up
 * when there will be no channels or groups to subscribe to.
 *
 * @param ee Pointer to the Event Engine which new state should be created.
 * @return Pointer to the `Unsubscribed` state.
 */
pbcc_ee_state_t* pbcc_unsubscribed_state_alloc(void);

/**
 * @brief Initial subscription state.
 *
 * State, which is used to perform initial subscription to receive the next
 * subscription loop time token (cursor) and notify channels / groups subscribes
 * about change in subscriber's presence.
 *
 * @param ee      Pointer to the Event Engine which new state should be created.
 * @param context Pointer to the context with updated Subscribe Event Engine
 *                information which should be used for the next subscription
 *                loop.
 * @return Pointer to the `Handshaking state`, which should be used as
 *         Subscribe Event Engine state transition target state.
 */
pbcc_ee_state_t* pbcc_handshaking_state_alloc(
    pbcc_event_engine_t* ee,
    pbcc_ee_data_t* context);

/**
 * @brief Initial subscription failed state.
 *
 * State, which is used by the Subscribe Event Engine when the  last
 * subscription loop can't be retried anymore.
 *
 * @param context Pointer to the context with list of channels / groups and time
 *                token (cursor) which has been used during last initial
 *                subscription before receiving operation failure result.
 * @return Pointer to the `Handshake failed state`, which should be used as
 *         Subscribe Event Engine state transition target state.
 */
pbcc_ee_state_t* pbcc_handshake_failed_state_alloc(pbcc_ee_data_t* context);

/**
 * @brief Initial subscription stopped state.
 *
 * State, which is used by the Subscribe Event Engine to preserve its last
 * context while disconnected, and awaiting its reconnection.
 *
 * @param context Pointer to the context with list of channels / groups and time
 *                token (cursor) which has been used during last initial
 *                subscription.
 * @return Pointer to the `Handshake stopped state`, which should be used as
 *         Subscribe Event Engine state transition target state.
 */
pbcc_ee_state_t* pbcc_handshake_stopped_state_alloc(pbcc_ee_data_t* context);

/**
 * @brief Real-time updates receiving state.
 *
 * State, which is used by the Subscribe Event Engine to perform a long-poll
 * subscription loop to receive real-time updates.
 *
 * @param ee      Pointer to the Event Engine which new state should be created.
 * @param context Pointer to the context with list of channels / groups and time
 *                token (cursor) received from previous subscription loop to
 *                receive next real-time updates.
 * @return Pointer to the `Receiving real-time updates state`, which should be
 *         used as Subscribe Event Engine state transition target state.
 */
pbcc_ee_state_t* pbcc_receiving_state_alloc(
    pbcc_event_engine_t* ee,
    pbcc_ee_data_t* context);

/**
 * @brief Real-time updates receive failed state.
 *
 * State, which is used by the Subscribe Event Engine when the last long-poll
 * subscription loop can't be retried anymore.
 *
 * @param context Pointer to the context with list of channels / groups and time
 *                token (cursor) received from previous subscription loop to
 *                receive next real-time updates before receiving operation
 *                failure result.
 * @return Pointer to the `Receive real-time updates failed state`, which should
 *         be used as Subscribe Event Engine state transition target state.
 */
pbcc_ee_state_t* pbcc_receive_failed_state_alloc(pbcc_ee_data_t* context);

/**
 * @brief Real-time updates receive stopped state.
 *
 * State, which is used by the Subscribe Event Engine to preserve its last
 * context used for long-poll subscription loop while disconnected, and awaiting
 * its reconnection.
 *
 * @param context Pointer to the context with list of channels / groups and time
 *                token (cursor) received from previous subscription loop to
 *                receive next real-time updates.
 *                receive next real-time updates.
 * @return Pointer to the `Receive real-time updates stopped state`, which
 *         should be used as Subscribe Event Engine state transition target
 *         state.
 */
pbcc_ee_state_t* pbcc_receive_stopped_state_alloc(pbcc_ee_data_t* context);
#else // #if PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE
#error To use subscribe event engine API you must define PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE=1
#endif // #if PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE
#endif //PBCC_SUBSCRIBE_EVENT_ENGINE_STATES_H
