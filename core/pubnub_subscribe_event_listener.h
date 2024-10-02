/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#ifndef PUBNUB_SUBSCRIBE_EVENT_LISTENER_H
#define PUBNUB_SUBSCRIBE_EVENT_LISTENER_H
#if PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE
#if !PUBNUB_USE_SUBSCRIBE_V2
#error Subscribe event engine requires subscribe v2 API, so you must define PUBNUB_USE_SUBSCRIBE_V2=1
#endif // #if !PUBNUB_USE_SUBSCRIBE_V2
#ifndef PUBNUB_CALLBACK_API
#error Subscribe event engine requires callback based PubNub context, so you must define PUBNUB_CALLBACK_API
#endif // #ifndef PUBNUB_CALLBACK_API


/**
 * @file pubnub_subscribe_event_listener.h
 * @brief PubNub `event listener` module interface for real-time updates
 *        retrieval.
 */

#include "core/pubnub_subscribe_event_listener_types.h"
#include "core/pubnub_subscribe_event_engine_types.h"
#include "lib/pb_extern.h"


// ----------------------------------------------
//                   Functions
// ----------------------------------------------

/**
 * @brief Add subscription status change listener.
 *
 * @note It is possible to add multiple listeners.
 *
 * @param pb       Pointer to the PubNub context from which subscription status
 *                 changes should be reported by provided listener.
 * @param callback Subscription status change handling listener function.
 * @return Results of listener addition.
 */
PUBNUB_EXTERN enum pubnub_res pubnub_subscribe_add_status_listener(
    const pubnub_t*                    pb,
    pubnub_subscribe_status_callback_t callback);

/**
 * @brief Remove subscription status change listener.
 *
 * \b Warning: All occurrences of the same `listener` will be removed from the
 * list. If multiple observers registered with the same listener function
 * (same address) they all will stop receiving updates.
 *
 * @param pb       Pointer to the PubNub context from which subscription status
 *                 changes shouldn't be reported to the provided listener.
 * @param callback Subscription status change handling listener function.
 * @return Results of listener removal.
 */
PUBNUB_EXTERN enum pubnub_res pubnub_subscribe_remove_status_listener(
    const pubnub_t*                    pb,
    pubnub_subscribe_status_callback_t callback);

/**
 * @brief Add subscription real-time updates listener.
 *
 * PubNub context will receive real-time updates from all subscription and
 * subscription sets.
 *
 * @note It is possible to add multiple listeners.
 *
 * @param pb       Pointer to the PubNub context from which subscription status
 *                 changes should be reported by provided listener.
 * @param type     Type of real-time update for which listener will be called.
 * @param callback Subscription status change handling listener function.
 * @return Results of listener addition.
 */
PUBNUB_EXTERN enum pubnub_res pubnub_subscribe_add_message_listener(
    const pubnub_t*                     pb,
    pubnub_subscribe_listener_type      type,
    pubnub_subscribe_message_callback_t callback);

/**
 * @brief Remove subscription real-time updates listener..
 *
 * \b Warning: All occurrences of the same `listener` will be removed from the
 * list. If multiple observers registered with the same listener function
 * (same address) they all will stop receiving updates.
 *
 * @param pb       Pointer to the PubNub context from which subscription status
 *                 changes shouldn't be reported to the provided listener.
 * @param type     Type of real-time update for which listener won't be called
 *                 anymore.
 * @param callback Subscription status change handling listener function.
 * @return Results of listener removal.
 */
PUBNUB_EXTERN enum pubnub_res pubnub_subscribe_remove_message_listener(
    const pubnub_t*                     pb,
    pubnub_subscribe_listener_type      type,
    pubnub_subscribe_message_callback_t callback);

/**
 * @brief Add subscription real-time updates listener.
 *
 * @param subscription Subscription object for which 'listener' for specific
 *                     real-time updates 'type' will be registered.
 * @param type         Type of real-time update for which listener will be
 *                     called.
 * @param callback     Real-time update handling listener function.
 * @return Result of listener addition.
 */
PUBNUB_EXTERN enum pubnub_res pubnub_subscribe_add_subscription_listener(
    const pubnub_subscription_t*        subscription,
    pubnub_subscribe_listener_type      type,
    pubnub_subscribe_message_callback_t callback);

/**
 * @brief Remove subscription real-time updates listener.
 *
 * \b Warning: All occurrences of the same `listener` for specific `type` will
 * be removed from the list. If multiple observers registered with the same
 * listener function (same address) and `type` they all will stop receiving
 * updates.
 *
 * @param subscription Subscription object for which 'listener' for specific
 *                     real-time updates 'type' will be removed.
 * @param type         Type of real-time update for which listener won't be
 *                     called anymore.
 * @param callback     Real-time update handling listener function.
 * @return Results of listener removal.
 */
PUBNUB_EXTERN enum pubnub_res pubnub_subscribe_remove_subscription_listener(
    const pubnub_subscription_t*        subscription,
    pubnub_subscribe_listener_type      type,
    pubnub_subscribe_message_callback_t callback);

/**
 * @brief Add subscription set real-time updates listener.
 *
 * @param subscription_set Subscription set object for which 'listener' for
 *                         specific real-time updates 'type' will be registered.
 * @param type             Type of real-time update for which listener will be
 *                         called.
 * @param callback         Real-time update handling listener function.
 * @return Result of listener addition.
 */
PUBNUB_EXTERN enum pubnub_res pubnub_subscribe_add_subscription_set_listener(
    const pubnub_subscription_set_t*    subscription_set,
    pubnub_subscribe_listener_type      type,
    pubnub_subscribe_message_callback_t callback);

/**
 * @brief Remove subscription set real-time updates listener.
 *
 * \b Warning: All occurrences of the same `listener` for specific `type` will
 * be removed from the list. If multiple observers registered with the same
 * listener function (same address) and `type` they all will stop receiving
 * updates.
 *
 * @param subscription_set Subscription set object for which 'listener' for
 *                         specific real-time updates 'type' will be removed.
 * @param type             Type of real-time update for which listener won't be
 *                         called anymore.
 * @param callback         Real-time update handling listener function.
 * @return Results of listener removal.
 */
PUBNUB_EXTERN enum pubnub_res pubnub_subscribe_remove_subscription_set_listener(
    const pubnub_subscription_set_t*    subscription_set,
    pubnub_subscribe_listener_type      type,
    pubnub_subscribe_message_callback_t callback);
#else // #if PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE
#error To use subscribe event engine API you must define PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE=1
#endif // #if PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE
#endif // #ifndef PUBNUB_SUBSCRIBE_EVENT_LISTENER_H