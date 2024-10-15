/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#ifndef PUBNUB_SUBSCRIBE_EVENT_ENGINE_TYPES_H
#define PUBNUB_SUBSCRIBE_EVENT_ENGINE_TYPES_H
#if PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE


/**
 * @file  pubnub_subscribe_event_engine_types.h
 * @brief Public Subscribe Event Engine types.
 */

#include <stdbool.h>


// ----------------------------------------------
//               Types forwarding
// ----------------------------------------------

/**
 * @brief Subscription set definition.
 *
 * Subscription set aggregates individual subscription objects and allows
 * performing similar operations on the set as on individual subscription
 * objects. Added listeners will receive real-time updates for all entities
 * which have been used in aggregated subscription objects.
 *
 * @see pubnub_subscription_set_alloc_with_entities
 * @see pubnub_subscription_set_alloc_with_subscriptions
 */
typedef struct pubnub_subscription_set pubnub_subscription_set_t;

/**
 * @brief Subscription definition.
 *
 * Subscription object represent PubNub entities in a subscription loop which is
 * used to receive real-time updates for them.
 *
 * The Subscription Event Engine module provides an interface to:
 * - manage,
 * - subscribe,
 * - unsubscribe,
 * - add and remove real-time update listeners.
 *
 * @see pubnub_subscription_alloc
 */
typedef struct pubnub_subscription pubnub_subscription_t;


// ----------------------------------------------
//                    Types
// ----------------------------------------------

/**
 * @brief Subscription and subscription set objects configuration options.
 * @code
 * pubnub_subscription_options_t options = pubnub_subscription_options_defopts();
 * @endcode
 *
 * @see pubnub_subscription_options_defopts
 */
typedef struct pubnub_subscription_options {
    /** Whether presence events should be received or not. */
    bool receive_presence_events;
} pubnub_subscription_options_t;

/**
 * @brief Time cursor.
 *
 * Cursor used by subscription loop to identify the point in time after which
 * updates will be delivered.<br/>
 * Old cursor may help return missed messages (catch up) if they are still
 * present in the cache.
 *
 * @see pubnub_subscribe_cursor
 */
typedef struct {
    /** PubNub high-precision timestamp. */
    char timetoken[20];
    /**
     * @brief Data center region for which `timetoken` has been generated.
     *
     * @note This is an \a optional value and can be set to `0` if not needed.
     */
    int region;
} pubnub_subscribe_cursor_t;

/** PubNub subscription statuses. */
typedef enum {
    /** PubNub client subscribe and ready to receive real-time updates. */
    PNSS_SUBSCRIPTION_STATUS_CONNECTED,
    /** PubNub client were unable to subscribe to receive real-time updates. */
    PNSS_SUBSCRIPTION_STATUS_CONNECTION_ERROR,
    /** PubNub client has been disconnected because of some error. */
    PNSS_SUBSCRIPTION_STATUS_DISCONNECTED_UNEXPECTEDLY,
    /**
     * @brief PubNub client has been intentionally temporarily disconnected from
     *        the real-time updates.
     */
    PNSS_SUBSCRIPTION_STATUS_DISCONNECTED,
    /**
     * @brief PubNub client has been unsubscribed from all real-time update
     *        sources.
     */
    PNSS_SUBSCRIPTION_STATUS_SUBSCRIPTION_CHANGED
} pubnub_subscription_status;
#else // #if PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE
#error To use subscribe event engine API you must define PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE=1
#endif // #if PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE
#endif // #ifndef PUBNUB_SUBSCRIBE_EVENT_ENGINE_TYPES_H