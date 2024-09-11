#ifndef PUBNUB_SUBSCRIBE_EVENT_LISTENER_TYPES_H
#define PUBNUB_SUBSCRIBE_EVENT_LISTENER_TYPES_H


/**
 * @file  pubnub_subscribe_event_listener_types.h
 * @brief Public Subscribe Event Listener types.
 */

#include "core/pubnub_subscribe_event_engine_types.h"
#include "core/pubnub_subscribe_v2_message.h"
#include "core/pubnub_api_types.h"


// ----------------------------------------------
//                    Types
// ----------------------------------------------

// PubNub subscribe event listener events sources.
typedef enum {
    // Listener to handle real-time messages.
    LISTENER_ON_MESSAGE,
    // Listener to handle real-time signals.
    LISTENER_ON_SIGNAL,
    // Listener to handle message action real-time updates.
    LISTENER_ON_MESSAGE_ACTION,
    // Listener to handle App Context real-time updates.
    LISTENER_ON_OBJECTS,
    // Listener to handle real-time files sharing events.
    LISTENER_ON_FILES
} pubnub_subscribe_listener_type;

/**
 * @brief PubNub subscribe status change function definition.
*
 * @param pb     Pointer to the PubNub context for which subscribe status did
 *               change.
 * @param status PubNub client new subscribe status.
 * @param reason In case of `SUBSCRIPTION_STATUS_CONNECTION_ERROR` and
 *               `SUBSCRIPTION_STATUS_DISCONNECTED_UNEXPECTEDLY` may contain
 *               additional information about reasons of failure.
 *
 * @see pubnub_subscription_status
 */
typedef void (*pubnub_subscribe_status_callback_t)(
    const pubnub_t*            pb,
    pubnub_subscription_status status,
    enum pubnub_res            reason);

/**
 * @brief PubNub subscribe real-time updates (messages) function definition.
 *
 * @note Listener will be called for each received message designated to
 *       specific `pubnub_subscription_t` or `pubnub_subscription_set_t`.
 *
 * @param pb      Pointer to the PubNub context which receive real-time update.
 * @param message Object with a payload which corresponds to the type of source
 *                with which the listener has been registered.
 *
 * @see pubnub_subscription_t
 * @see pubnub_subscription_set_t
 */
typedef void (*pubnub_subscribe_message_callback_t)(
    const pubnub_t*          pb,
    struct pubnub_v2_message message);
#endif //PUBNUB_SUBSCRIBE_EVENT_LISTENER_TYPES_H