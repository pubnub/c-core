/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#ifndef PBCC_SUBSCRIBE_EVENT_LISTENER_H
#define PBCC_SUBSCRIBE_EVENT_LISTENER_H
#if !PUBNUB_USE_SUBSCRIBE_V2
#error Subscribe event engine requires subscribe v2 API, so you must define PUBNUB_USE_SUBSCRIBE_V2=1
#endif // #if !PUBNUB_USE_SUBSCRIBE_V2
#ifndef PUBNUB_CALLBACK_API
#error Subscribe event engine requires callback based PubNub context, so you must define PUBNUB_CALLBACK_API
#endif // #ifndef PUBNUB_CALLBACK_API


/**
 * @file  pbcc_subscribe_event_listener.h
 * @brief Subscribe Event Listener core implementation with base type
 *        definitions and functions.
 */

#include "core/pubnub_subscribe_event_listener_types.h"
#include "core/pubnub_api_types.h"
#include "lib/pbarray.h"


// ----------------------------------------------
//                Types forwarding
// ----------------------------------------------

/** Event Listener definition. */
typedef struct pbcc_event_listener pbcc_event_listener_t;


// ----------------------------------------------
//                   Functions
// ----------------------------------------------

/**
 * @brief Create Event Listener.
 *
 * @param pb Pointer to the PubNub context which will be passed into listener.
 * @return Pointer to the ready to use Event Listener or `NULL` in case of
 *         insufficient memory error. The returned pointer must be passed to
 *         the `pbcc_event_listener_free` to avoid a memory leak.
 */
pbcc_event_listener_t* pbcc_event_listener_alloc(const pubnub_t* pb);

/**
 * @brief Add subscription status change listener.
 *
 * @param listener Pointer to the Event Listener which is used to track and call
 *                 registered subscription status change listeners.
 * @param cb       Subscription status change handling listener function.
 * @return Results of listener addition.
 */
enum pubnub_res pbcc_event_listener_add_status_listener(
    pbcc_event_listener_t* listener,
    pubnub_subscribe_status_callback_t cb);

/**
 * @brief Remove subscription status change listener.
 *
 * \b Warning: All occurrences of the same `listener` will be removed from the
 * list. If multiple observers registered with the same listener function
* (same address) they all will stop receiving updates.
 *
 * @param listener Pointer to the Event Listener which should unregister
 *                 subscription status change listener.
 * @param cb       Subscription status change handling listener function.
 * @return Results of listener removal.
 */
enum pubnub_res pbcc_event_listener_remove_status_listener(
    pbcc_event_listener_t* listener,
    pubnub_subscribe_status_callback_t cb);

/**
 * @brief Add subscription real-time updates listener.
 *
 * PubNub context will receive real-time updates from all subscription and
 * subscription sets.
 *
 * @param listener Pointer to the Event Listener which is used to track and call
 *                 registered real-time updates listeners.
 * @param type     Type of real-time update for which listener will be called.
 * @param cb       Real-time update handling listener function.
 * @return Results of listener addition.
 */
enum pubnub_res pbcc_event_listener_add_message_listener(
    pbcc_event_listener_t* listener,
    pubnub_subscribe_listener_type type,
    pubnub_subscribe_message_callback_t cb);

/**
 * @brief Remove subscription real-time updates listener.
 *
 * \b Warning: All occurrences of the same `listener` will be removed from the
 * list. If multiple observers registered with the same listener function
* (same address) they all will stop receiving updates.
 *
 * @param listener Pointer to the Event Listener which should unregister
*                  registered real-time updates listeners.
 * @param type     Type of real-time update for which listener won't be called
 *                 anymore.
 * @param cb       Real-time update handling listener function.
 * @return Results of listener removal.
 */
enum pubnub_res pbcc_event_listener_remove_message_listener(
    pbcc_event_listener_t* listener,
    pubnub_subscribe_listener_type type,
    pubnub_subscribe_message_callback_t cb);

/**
 * @brief Add subscription real-time updates listener.
 *
 * @param listener     Pointer to the Event Listener which is used to track and
 *                     call registered real-time updates listeners.
 * @param type         Type of real-time update for which listener will be
 *                     called.
 * @param names        Pointer to the list of subscription names with which
 *                     `callback` should be associated.
 * @param subscription Pointer to the subscription object for which 'callback'
 *                     for specific real-time updates 'type' will be registered.
 * @param cb           Real-time update handling listener function.
 * @return Result of listener addition.
 */
enum pubnub_res pbcc_event_listener_add_subscription_object_listener(
    pbcc_event_listener_t* listener,
    pubnub_subscribe_listener_type type,
    pbarray_t* names,
    const void* subscription,
    pubnub_subscribe_message_callback_t cb);

/**
 * @brief Remove subscription real-time updates listener.
 *
 * \b Warning: All occurrences of the same `listener` for specific `type` will
 * be removed from the list. If multiple observers registered with the same
 * listener function (same address) and `type` they all will stop receiving
 * updates.
 *
 * @param listener     Pointer to the Event Listener which should unregister
 *                     registered real-time updates listeners.
 * @param type         Type of real-time update for which listener won't be
 *                     called anymore.
 * @param names        Pointer to the list of subscription names with which
 *                     `callback` was associated.
 * @param subscription Subscription object for which `callback` for specific
 *                     real-time updates 'type' will be removed.
 * @param cb           Real-time update handling listener function.
 * @return Results of listener removal.
 */
enum pubnub_res pbcc_event_listener_remove_subscription_object_listener(
    pbcc_event_listener_t* listener,
    pubnub_subscribe_listener_type type,
    pbarray_t* names,
    const void* subscription,
    pubnub_subscribe_message_callback_t cb);

/**
 * @brief Notify subscription status listeners about status change.
 *
 * @param listener       Pointer to the Event Listener which contains list of
 *                       listeners for subscription status change event.
 * @param status         New subscription status which should be sent the the
 *                       listeners.
 * @param reason         In case of `PNSS_SUBSCRIPTION_STATUS_CONNECTION_ERROR`
 *                       and `PNSS_SUBSCRIPTION_STATUS_DISCONNECTED_UNEXPECTEDLY`
 *                       may contain additional information about reasons of
 *                       failure.
 * @param channels       Byte string with comma-separated / `NULL` channel
 *                       identifiers which have been used with recent operation.
 * @param channel_groups Byte string with comma-separated / `NULL` channel group
 *                       identifiers which have been used with recent operation.
 */
void pbcc_event_listener_emit_status(
    pbcc_event_listener_t* listener,
    pubnub_subscription_status status,
    enum pubnub_res reason,
    const char* channels,
    const char* channel_groups);

/**
 * @brief Notify listeners about new real-time update / message.
 *
 * @param listener     Pointer to the Event Listener which contains list of
 *                     listeners for subscription real-time update / message
 *                     event.
 * @param subscribable Pointer to the subscrbable entity for which `message` has
 *                     been received.
 * @param message  Received message which should be delivered to the listeners.
 */
void pbcc_event_listener_emit_message(
    pbcc_event_listener_t* listener,
    const char* subscribable,
    struct pubnub_v2_message message);

/**
 * @brief Clean up resources used by the Event Listener object.
 *
 * @param event_listener Pointer to the Event Listener, which should free up
 *                       used resources.
 */
void pbcc_event_listener_free(pbcc_event_listener_t** event_listener);
#endif // #ifndef PBCC_SUBSCRIBE_EVENT_LISTENER_H
