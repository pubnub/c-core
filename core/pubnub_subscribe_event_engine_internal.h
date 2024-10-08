/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#ifndef PUBNUB_SUBSCRIBE_EVENT_ENGINE_INTERNAL_H
#define PUBNUB_SUBSCRIBE_EVENT_ENGINE_INTERNAL_H
#if PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE


/**
 * @file  pubnub_subscribe_event_engine_internal.h
 * @brief Module to manage first-class types internal / lib interface.
 */

#include "core/pubnub_subscribe_event_engine.h"
#include "core/pbcc_subscribe_event_engine.h"
#include "core/pubnub_memory_block.h"
#include "lib/pbref_counter.h"
#include "core/pubnub_mutex.h"
#include "lib/pbhash_set.h"


// ----------------------------------------------
//                    Types
// ----------------------------------------------

/** Subscribable object location in subscribe REST API URI. */
typedef enum {
    /** Object should be used as part of the path. */
    SUBSCRIBABLE_LOCATION_PATH,
    /** Object should be used as a query parameter. */
    SUBSCRIBABLE_LOCATION_QUERY,
} pubnub_subscribable_location;

/** Subscribable object definition. */
typedef struct {
    /** Subscribable identifier location in subscribe REST API URI. */
    pubnub_subscribable_location location;
    /**
     * @brief Whether lifecycle of the memory block for subscribable identifier
     *        already managed or not.
     *
     * Calling code should free up resources when done working with subscribable
     * if memory block lifecycle not managed.
     */
    bool managed_memory_block;
    /**
     * @brief Pointer to the subscribable identifier.
     *
     * @note Declared as a pointer to make it easier to pass value between
     *       entity and subscribable object (to preserve memory).
     */
    struct pubnub_char_mem_block* id;
} pubnub_subscribable_t;

/** Subscription definition. */
struct pubnub_subscription {
    /**
     * @brief Subscription configuration options.
     *
     * \b Important: Don't change field position because it is used in code to
     * get access to the subscription / set object options.
     */
    pubnub_subscription_options_t options;
    /**
     * @brief Pointer to the PubNub entity for which real-time updates should be
     *        picked when subscription used in subscription loop.
     */
    pubnub_entity_t* entity;
    /**
     * @brief Pointer to the Subscribe Event Engine, which will operate with a
     *        subscription object.
     */
    pbcc_subscribe_ee_t* ee;
    /** Object references counter. */
    pbref_counter_t* counter;
    /** Shared resources access lock. */
    pubnub_mutex_t mutw;
};

/** Subscription set definition. */
struct pubnub_subscription_set {
    /**
     * @brief Subscription configuration options.
     *
     * \b Important: Don't change field position because it is used in code to
     * get access to the subscription / set object options.
     */
    pubnub_subscription_options_t options;
    /** Pointer to the hash set with pointers to subscription objects. */
    pbhash_set_t* subscriptions;
    /** Whether a subscription set is used in subscription loop or not. */
    bool subscribed;
    /**
     * @brief Pointer to the Subscribe Event Engine, which will operate with a
     *        subscription set object.
     */
    pbcc_subscribe_ee_t* ee;
    /** Object references counter. */
    pbref_counter_t* counter;
    /** Shared resources access lock. */
    pubnub_mutex_t mutw;
};


// ----------------------------------------------
//                   Functions
// ----------------------------------------------

/**
 * @brief Release resources used by subscription.
 *
 * @note Function may NULLify provided subscription pointer if there is no other
 *       references to it.
 *
 * @param sub Pointer to the subscription, which should free up resources.
 * @return `false` if there are more references to the subscription.
 */
bool pubnub_subscription_free_(pubnub_subscription_t* sub);

/**
 * @brief Release resources used by subscription set.
 *
 * @note Function will NULLify provided subscription set pointer if it is no
 *       other references to it.
 *
 * @param set Pointer to the subscription set, which should free up resources.
 * @return `false` if there are more references to the subscription set.
 */
bool pubnub_subscription_set_free_(pubnub_subscription_set_t* set);

/**
 * @brief Get a list of subscription's subscribable objects.
 *
 * Depending on from subscription `options`, the list may contain additional
 * entries (presence for example).
 *
 * \b Important: This function should be used in `pbcc_subscribe_event_engine.c`
 * file only to update list of active subscribables - because of this reason
 * `pbhash_set_t` configured without element destruction function since their
 * lifecycle will be managed by calling code.
 *
 * @param sub     Pointer to the subscription, which should provide subscribable
 *                objects.
 * @param options Pointer to the subscription configuration options override.
 *                `NULL` can be used and options from `sub` will be used instead.
 * @return Pointer to the hash set of `pubnub_subscribable_t` objects for
 *         subscription loop or `NULL` in case of insufficient memory error. The
 *         returned pointer must be passed to the `pbhash_set_free` to avoid a
 *         memory leak.
 *
 * @see pubnub_subscribable_t
 */
pbhash_set_t* pubnub_subscription_subscribables_(
    const pubnub_subscription_t*         sub,
    const pubnub_subscription_options_t* options);

/**
 * @brief Get a list of subscription set subscribable objects.
 *
 * Depending on from subscription set `options`, the list may contain additional
 * entries (presence for example).
 *
 * \b Important: This function should be used in `pbcc_subscribe_event_engine.c`
 * file only to update list of active subscribables - because of this reason
 * `pbhash_set_t` configured without element destruction function since their
 * lifecycle will be managed by calling code.
 *
 * @param set Pointer to the subscription set, which should provide subscribable
 *            objects.
 * @return Pointer to the hash set of `pubnub_subscribable_t` objects for
 *         subscription loop or `NULL` in case of insufficient memory error. The
 *         returned pointer must be passed to the `pbhash_set_free` to avoid a
 *         memory leak.
 *
 * @see pubnub_subscribable_t
 */
pbhash_set_t* pubnub_subscription_set_subscribables_(
    const pubnub_subscription_set_t* set);

/**
 * @brief Get length of the subscribable objects string.
 *
 * @param subscribable Pointer to the subscribable, for which length should be
 *                     retrieved.
 * @return Length of the subscribable string.
 */
size_t pubnub_subscribable_length_(const pubnub_subscribable_t* subscribable);

/**
 * @brief Check whether `subscribable` represent name for presence.
 *
 * @param subscribable Pointer to the subscribable, for which information should
 *                     be retrieved.
 * @return `false` if `subscribable` represents presence name.
 */
bool pubnub_subscribable_is_presence_(
    const pubnub_subscribable_t* subscribable);

/**
 * @brief Check whether `subscribable` represent channel group.
 *
 * @param subscribable Pointer to the subscribable, for which information should
 *                     be retrieved.
 * @return `false` if `subscribable` represents regular channel name.
 */
bool pubnub_subscribable_is_cg_(const pubnub_subscribable_t* subscribable);

/**
 * @brief Clean up resources used by subscribable.
 *
 * @param subscribable Pointer to the subscribable, which should free up
 *                     resources.
 */
void pubnub_subscribable_free_(pubnub_subscribable_t* subscribable);
#else // #if PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE
#error To use subscribe event engine API you must define PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE=1
#endif // #if PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE
#endif // #ifndef PUBNUB_SUBSCRIBE_EVENT_ENGINE_INTERNAL_H