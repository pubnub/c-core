/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#ifndef PBCC_SUBSCRIBE_EVENT_ENGINE_EFFECTS_H
#define PBCC_SUBSCRIBE_EVENT_ENGINE_EFFECTS_H
#if PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE
#if !PUBNUB_USE_SUBSCRIBE_V2
#error Subscribe event engine requires subscribe v2 API, so you must define PUBNUB_USE_SUBSCRIBE_V2=1
#endif // #if !PUBNUB_USE_SUBSCRIBE_V2


/**
 * @file  pbcc_subscribe_event_engine_events.h
 * @brief Subscribe Event Engine states transition events.
*/

#include "core/pbcc_event_engine.h"


// ----------------------------------------------
//                   Functions
// ----------------------------------------------

/**
 * @brief Execute (schedule) initial subscribe to the channels / groups.
 *
 * Effect scheduled because PubNub context uses a callback interface and status
 * will be known only with callback call.
 *
 * @param invocation Pointer to the invocation which has been used to execute
 *                   effect.
 * @param context    Pointer to the Subscription context object with information
 *                   for initial subscribe (tt=0).
 * @param cb         Pointer to the effect execution completion callback
 *                   function.
 */
void pbcc_subscribe_ee_handshake_effect(
    pbcc_ee_invocation_t* invocation,
    pbcc_ee_data_t* context,
    pbcc_ee_effect_completion_function_t cb);

/**
 * @brief Execute (schedule) next subscription loop.
 *
 * Effect scheduled because PubNub context uses a callback interface and status
 * will be known only with callback call.
 *
 * @param invocation Pointer to the invocation which has been used to execute
 *                   effect.
 * @param context    Pointer to the Subscription context object with information
 *                   for next subscription loop (tt!=0).
 * @param cb         Pointer to the effect execution completion callback
 *                   function.
 */
void pbcc_subscribe_ee_receive_effect(
    pbcc_ee_invocation_t* invocation,
    pbcc_ee_data_t* context,
    pbcc_ee_effect_completion_function_t cb);

/**
 * @brief Notify status change listeners.
 *
 * @param invocation Pointer to the invocation which has been used to execute
 *                   effect.
 * @param context    Pointer to the Subscription context with information which
 *                   is required by the Event Listener to notify all status
 *                   change listeners.
 * @param cb         Pointer to the effect execution completion callback
 *                   function.
 */
void pbcc_subscribe_ee_emit_status_effect(
    pbcc_ee_invocation_t* invocation,
    pbcc_ee_data_t* context,
    pbcc_ee_effect_completion_function_t cb);

/**
 * @brief Notify real-time updates listeners.
 *
 * @param invocation Pointer to the invocation which has been used to execute
 *                   effect.
 * @param context    Pointer to the Subscription context with information which
 *                   is required by the Event Listener to notify all real-time
 *                   update listeners.
 * @param cb         Pointer to the effect execution completion callback
 *                   function.
 */
void pbcc_subscribe_ee_emit_messages_effect(
    pbcc_ee_invocation_t* invocation,
    pbcc_ee_data_t* context,
    pbcc_ee_effect_completion_function_t cb);

/**
 * @brief Cancel previously started HTTP operation.
 *
 * @param invocation Pointer to the invocation which has been used to execute
 *                   effect.
 * @param context    Pointer to the Subscription context with information which
 *                   is required to cancel a previously started HTTP operation.
 * @param cb         Pointer to the effect execution completion callback
 *                   function.
 */
void pbcc_subscribe_ee_cancel_effect(
    pbcc_ee_invocation_t* invocation,
    pbcc_ee_data_t* context,
    pbcc_ee_effect_completion_function_t cb);
#else // #if PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE
#error To use subscribe event engine API you must define PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE=1
#endif // #if PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE
#endif //PBCC_SUBSCRIBE_EVENT_ENGINE_EFFECTS_H
