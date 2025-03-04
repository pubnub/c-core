/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
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

/** PubNub subscribe event listener events sources. */
typedef enum
{
    /** Listener to handle real-time messages. */
    PBSL_LISTENER_ON_MESSAGE,
    /** Listener to handle real-time signals. */
    PBSL_LISTENER_ON_SIGNAL,
    /** Listener to handle message action real-time updates. */
    PBSL_LISTENER_ON_MESSAGE_ACTION,
    /** Listener to handle App Context real-time updates. */
    PBSL_LISTENER_ON_OBJECTS,
    /** Listener to handle real-time files sharing events. */
    PBSL_LISTENER_ON_FILES
} pubnub_subscribe_listener_type;

/** PubNub subscription status change data object. */
typedef struct
{
    /**
     * @brief Error details in case of error.
     *
     * In case of `PNSS_SUBSCRIPTION_STATUS_CONNECTION_ERROR` and
     * `PNSS_SUBSCRIPTION_STATUS_DISCONNECTED_UNEXPECTEDLY` may contain
     * additional information about reasons of failure.
     */
    const enum pubnub_res reason;
    /**
     * @brief Affected channels.
     *
     * Byte string with comma-separated / `NULL` channel identifiers which have
     * been used with recent operation.
     *
     * \b Warning: Memory managed by PubNub context and can be freed after
     * callback function return.
     */
    const char* channels;
    /**
     * @brief Affected channel groups.
     *
     * Byte string with comma-separated / `NULL` channel group identifiers which
     * have been used with recent operation.
     *
     * \b Warning: Memory managed by PubNub context and can be freed after
     * callback function return.
     */
    const char* channel_groups;
} pubnub_subscription_status_data_t;

/**
 * @brief PubNub subscribe status change function definition.
*
 * @param pb          Pointer to the PubNub context for which subscribe status
 *                    did change.
 * @param status      PubNub client new subscribe status.
 * @param status_data Additional information with details to the recent
 *                    subscription status change.
 *
 * @see pubnub_subscription_status
 * @see pubnub_subscription_status_data_t
 */
typedef void (*pubnub_subscribe_status_callback_t)(
    const pubnub_t* pb,
    pubnub_subscription_status status,
    pubnub_subscription_status_data_t status_data,
    void* data);

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
    const pubnub_t* pb,
    struct pubnub_v2_message message, 
    void* data);
#endif // #ifndef PUBNUB_SUBSCRIBE_EVENT_LISTENER_TYPES_H
