/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#ifndef PUBNUB_SUBSCRIBE_EVENT_ENGINE_H
#define PUBNUB_SUBSCRIBE_EVENT_ENGINE_H
#if PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE
#if !PUBNUB_USE_SUBSCRIBE_V2
#error Subscribe event engine requires subscribe v2 API, so you must define PUBNUB_USE_SUBSCRIBE_V2=1
#endif // #if !PUBNUB_USE_SUBSCRIBE_V2
#ifndef PUBNUB_CALLBACK_API
#error Subscribe event engine requires callback based PubNub context, so you must define PUBNUB_CALLBACK_API
#endif // #ifndef PUBNUB_CALLBACK_API


/**
 * @file  pubnub_subscribe_event_engine.h
 * @brief PubNub `subscribe` module interface implemented atop of event engine.
 */

#include <stdbool.h>
#include <stdlib.h>

#include "core/pubnub_subscribe_event_engine_types.h"
#include "core/pubnub_entities.h"
#include "lib/pb_extern.h"


// ----------------------------------------------
//                   Functions
// ----------------------------------------------

/**
 * @brief Default subscription configuration options.
 * @code
 * pubnub_subscription_options_t options = pubnub_subscription_options_defopts();
 * options.receive_presence_events = true;
 * @endcode
 *
 * @return Default subscription and subscription set configuration options.
 */
PUBNUB_EXTERN pubnub_subscription_options_t
pubnub_subscription_options_defopts(void);

/**
 * @brief Create entity subscription.
 *
 * Subscription is high-level access to the entity which is used in the
 * subscription loop. Subscriptions make it possible to add real-time updates
 * listeners for specific entity (channel, group, or App Context object). It is
 * possible to `subscribe` and `unsubscribe` using a specific subscription.
 *
 * Create subscription without configuration options:
 * @code
 * pubnub_subscription_t* subscription = pubnub_subscription_alloc(entity, NULL);
 * if (NULL == subscription) {
 *     // Handler parameters error (missing entity or invalid PubNub context
 *     // pointer) or insufficient memory.
 * }
 * @endcode
 * <br/>
 * Create subscription which will use presence channel:
 * @code
 * pubnub_subscription_options_t options = pubnub_subscription_options_defopts();
 * options.receive_presence_events = true;
 * pubnub_subscription_t* subscription = pubnub_subscription_alloc(
 *     entity,
 *     &options);
 * if (NULL == subscription) {
 *     // Handler parameters error (missing entity or invalid PubNub context
 *     // pointer) or insufficient memory.
 * }
 * @endcode
 *
 * \b Warning: Application execution will be terminated if `entity` is `NULL` or
 * entity's PubNub pointer is an invalid context pointer.
 *
 * @param entity  Pointer to the PubNub entity which should be used in
 *                subscription.
 * @param options Pointer to the subscription configuration options. Set to
 *                `NULL` if no options required.
 * @return Pointer to the ready to use PubNub subscription, or `NULL` will be
 *         returned in case of error (in most of the cases caused by
 *         insufficient memory for structure allocation). The returned pointer
 *         must be passed to the `pubnub_subscription_free` to avoid a memory
 *         leak.
 *
 * @see pubnub_subscription_options_defopts
 * @see pubnub_channel_alloc
 * @see pubnub_channel_group_alloc
 * @see pubnub_channel_metadata_alloc
 * @see pubnub_user_metadata_alloc
 */
PUBNUB_EXTERN pubnub_subscription_t* pubnub_subscription_alloc(
    pubnub_entity_t* entity,
    const pubnub_subscription_options_t* options);

/**
 * @brief Release resources used by subscription.
 * @code
 * enum pubnub_res rslt = pubnub_subscribe_with_subscription(subscription, NULL);
 * if (PNR_OK != rslt) {
 *     // handle subscription error (mostly because of parameter error).
 * }
 * //
 * // ... later ...
 * //
 * if(!pubnub_subscription_free(&subscription)) {
 *     // This call will decrease references count, but won't actually release
 *     // resources because the event engine holds the additional reference to
 *     // the subscription object.
 *     // In this situation, when reference to the user-allocated subscription
 *     // is freed, resources will be freed only after 'pubnub_unsubscribe_all'
 *     // function call.
 * }
 * @endcode
 *
 * \b Important: Subscription resources won't be freed if:
 * - subscription used in subscription loop,
 * - subscription is part of subscription set.
 *
 * \b Warning: Make sure that calls to the `pubnub_subscription_free` are done
 * only once to not disrupt other types which still use this entity.
 *
 * @note Function may NULLify provided subscription pointer if there is no other
 *       references to it.
 *
 * @param sub Pointer to the subscription, which should free up resources.
 * @return `false` if there are more references to the subscription.
 *
 * @see pubnub_subscription_alloc
 * @see pubnub_subscribe_with_subscription
 */
PUBNUB_EXTERN bool pubnub_subscription_free(pubnub_subscription_t** sub);

/**
 * @brief Retrieve subscriptions currently used in subscribe.
 *
 * The function will return subscriptions which have been used with
 * `pubnub_subscription_subscribe` function call and currently used in the
 * subscription loop.
 * @code
 * enum pubnub_res rslt = pubnub_subscribe_with_subscription(subscription1, NULL);
 * if (PNR_OK != rslt) {
 *     // handle subscription error (mostly because of parameters error).
 * }
 * rslt = pubnub_subscribe_with_subscription(subscription2, NULL);
 * if (PNR_OK != rslt) {
 *     // handle subscription error (mostly because of parameters error).
 * }
 * rslt = pubnub_subscribe_with_subscription(subscription3, NULL);
 * if (PNR_OK != rslt) {
 *     // handle subscription error (mostly because of parameters error).
 * }
 * //
 * // ... later ...
 * //
 * size_t subs_count;
 * pubnub_subscription_t** subs = pubnub_subscriptions(pb, &subs_count);
 * if (NULL == subs) {
 *     // Handle parameters error (invalid PubNub context pointer) or there is
 *     // no active subscriptions.
 * }
 * // subs: subscription1, subscription2, subscription3
 * // subs_count: 3
 * free(subs);
 * @endcode
 *
 * \b Warning: Application execution will be terminated if PubNub pointer is an
 * invalid context pointer.
 *
 * @param pb          Pointer to the PubNub context from which list of
 *                    subscriptions should be retrieved.
 * @param [out] count Parameter will hold the count of returned subscriptions.
 * @return Pointer to the array with subscription object pointers, or `NULL`
 *         will be returned in case of insufficient memory error. The returned
 *         pointer must be passed to the `free` to avoid a memory leak.
 *
 * @see pubnub_subscription_alloc
 * @see pubnub_subscribe_with_subscription
 */
PUBNUB_EXTERN pubnub_subscription_t** pubnub_subscriptions(
    const pubnub_t* pb,
    size_t* count);

/**
 * @brief Create a subscription set from the list of entities.
 *
 * Subscriptions set is high-level access to the grouped entities which is used
 * in the subscription loop. Subscriptions set make it possible to add real-time
 * updates listeners to the group of entities (channel, group, or App Context
 * object). It is possible to `subscribe` and `unsubscribe` using multiple
 * subscriptions at once.
 * Create subscription set without configuration options:
 * @code
 * pubnub_subscription_set_t* subscription_set = pubnub_subscription_set_alloc_with_entities(
 *     entities,
 *     4,
 *     NULL);
 * if (NULL == subscription_set) {
 *     // Handler parameters error (missing entities or invalid entity's PubNub
 *     // context pointer) or insufficient memory.
 * }
 * @endcode
 * <br/>
 * Create subscription set which will use presence channel:
 * @code
 * pubnub_subscription_options_t options = pubnub_subscription_options_defopts();
 * options.receive_presence_events = true;
 * pubnub_subscription_set_t* subscription_set = pubnub_subscription_set_alloc_with_entities(
 *     entities,
 *     4,
 *     &options);
 * if (NULL == subscription_set) {
 *     // Handler parameters error (missing entities or invalid entity's PubNub
 *     // context pointer) or insufficient memory.
 * }
 * @endcode
 *
 * \b Important: Additional `pubnub_subscription_t` won't be created for
 * duplicate entity entry.
 *
 * @param entities       Pointer to the array with PubNub entities objet
 *                       pointers which should be used in subscriptions set.
 * @param entities_count Number of PubNub entities in array.
 * @param options        Pointer to the subscription configuration options. Set
 *                       to `NULL` if options not required.
 * @return Pointer to the ready to use PubNub subscriptions set, or `NULL` will
 *         be returned in case of error (in most of the cases caused by
 *         insufficient memory for structure allocation). The returned pointer
 *         must be passed to the `pubnub_subscription_set_free` to avoid a
 *         memory leak.
 *
 * @see pubnub_subscription_options_defopts
 * @see pubnub_channel_alloc
 * @see pubnub_channel_group_alloc
 * @see pubnub_channel_metadata_alloc
 * @see pubnub_user_metadata_alloc
 */
PUBNUB_EXTERN pubnub_subscription_set_t*
pubnub_subscription_set_alloc_with_entities(
    pubnub_entity_t** entities,
    int entities_count,
    const pubnub_subscription_options_t* options);

/**
 * @brief Create subscription set from subscriptions.
 *
 * Subscription set is high-level access to the grouped entities which is used
 * in the subscription loop. Subscription set make it possible to add real-time
 * updates listeners to the group of entities (channel, group, or App Context
 * object). It is possible to `subscribe` and `unsubscribe` using multiple
 * subscriptions at once.
 * @code
 * pubnub_subscription_set_t* subscription_set = pubnub_subscription_set_alloc_with_subscriptions(
 *     subscription1,
 *     subscription2,
 *     NULL);
 * if (NULL == subscription_set) {
 *     // Handler parameters error (missing subscriptions or invalid entity's
 *     // PubNub context pointer) or insufficient memory.
 * }
 * @endcode
 *
 * @param sub1    Pointer to the left-hand subscription to be used for
 *                subscription set.
 * @param sub2    Pointer to the right-hand subscription to be used for
 *                subscription set.
 * @param options Pointer to the subscription configuration options override.
 *                Set to `NULL` if override not required.
 * @return Pointer to the ready to use PubNub subscription set, or `NULL` will
 *         be returned in case of error (in most of the cases caused by
 *         insufficient memory for structures allocation). The returned pointer
 *         must be passed to the `pubnub_subscription_set_free` to avoid a
 *         memory leak.
 *
 * @see pubnub_subscription_alloc
 */
PUBNUB_EXTERN pubnub_subscription_set_t*
pubnub_subscription_set_alloc_with_subscriptions(
    pubnub_subscription_t* sub1,
    pubnub_subscription_t* sub2,
    const pubnub_subscription_options_t* options);

/**
 * @brief Release resources used by subscription set.
 * @code
 * enum pubnub_res rslt = pubnub_subscribe_with_subscription_set(
 *     subscription_set,
 *     NULL);
 * if (PNR_OK != rslt) {
 *     // handle subscription error (mostly because of parameter error).
 * }
 * //
 * // ... later ...
 * //
 * if (!pubnub_subscription_set_free(&subscription_set)) {
 *     // This call will decrease references count, but won't release resources
 *     // because the event engine holds the additional reference to the
 *     // subscription set object.
 *     // In this situation, when reference to the user-allocated subscription
 *     // set is freed, resources will be freed only after
 *     // 'pubnub_unsubscribe_all' function call.
 * }
 * @endcode
  *
 * \b Important: Only those subscription objects which are not used in other
 * subscription sets will be freed.<br/>
 * \b Warning: Make sure that calls to the `pubnub_subscription_set_free` are
 * done only once to not disrupt other types which still use this entity.
 *
 * @note Function will NULLify provided subscription set pointer if it is no
 *       other references to it.
 *
 * @param set Pointer to the subscription set, which should free up resources.
 * @return `false` if there are more references to the subscription set.
 *
 * @see pubnub_subscription_set_alloc_with_entities
 * @see pubnub_subscription_set_alloc_with_subscriptions
 * @see pubnub_subscribe_with_subscription_set
 */
PUBNUB_EXTERN bool pubnub_subscription_set_free(
    pubnub_subscription_set_t** set);

/**
 * @brief Retrieve subscription sets currently used in subscribe.
 *
 * The function will return subscription sets which have been used with
 * `pubnub_subscribe_with_subscription_set` function call and currently used in
 * the subscription loop.
 * @code
 * enum pubnub_res rslt = pubnub_subscribe_with_subscription_set(
 *     subscription_set1,
 *     NULL);
 * if (PNR_OK != rslt) {
 *     // handle subscription error (mostly because of parameters error).
 * }
 *
 * rslt = pubnub_subscribe_with_subscription_set(subscription_set2, NULL);
 * if (PNR_OK != rslt) {
 *     // handle subscription error (mostly because of parameters error).
 * }
 * //
 * // ... later ...
 * //
 * size_t sets_count;
 * pubnub_subscription_set_t** sets = pubnub_subscription_sets(pb, &sets_count);
 * if (NULL == sets) {
 *     // Handle parameters error (invalid PubNub context pointer) or there is
 *     // no active subscriptions.
 * }
 * free(sets);
 * // sets: subscription_set1, subscription_set2
 * // sets_count: 2
 * @endcode
 *
 * \b Warning: Application execution will be terminated if PubNub pointer is an
 * invalid context pointer.
 *
 * @param pb          Pointer to the PubNub context from which list of
 *                    subscriptions should be retrieved.
 * @param [out] count Parameter will hold the count of returned subscription
 *                    sets.
 * @return Pointer to the array with subscription set object pointers, or `NULL`
 *         will be returned in case of insufficient memory error. The returned
 *         pointer must be passed to the `free` to avoid a memory leak.
 *
 * @see pubnub_subscription_set_alloc_with_entities
 * @see pubnub_subscription_set_alloc_with_subscriptions
 * @see pubnub_subscribe_with_subscription_set
 */
PUBNUB_EXTERN pubnub_subscription_set_t** pubnub_subscription_sets(
    const pubnub_t* pb,
    size_t* count);

/**
 * @brief Retrieve subscriptions from subscription set.
 * @code
 * // Create `pubnub_subscription_set_t` with `subscription1` and `subscription2`.
 * if (PNR_OUT_OF_MEMORY == pubnub_subscription_set_add(subscription_set,
 *                                                      subscription3)) {
 *     // handle error because of insufficient memory.
 * }
 * if (PNR_OUT_OF_MEMORY == pubnub_subscription_set_add(subscription_set,
 *                                                      subscription4)) {
 *     // handle error because of insufficient memory.
 * }
 *
 * size_t subs_count;
 * pubnub_subscription_t** subs = pubnub_subscription_set_subscriptions(
 *     subscription_set,
 *     &subs_count);
 * if (NULL == subs) {
 *     // Handle parameters error (invalid PubNub context pointer) or there is
 *     // no active subscriptions.
 * }
 * free(subs);
 * // subs: subscription1, subscription2, subscription3, subscription4
 * // subs_count: 4
 * @endcode
 *
 * \b Warning: Application execution will be terminated if subscription `set` is
 * `NULL`.
 *
 * @param set         Pointer to the subscription set for which array with
 *                    underlying subscription object pointers should be
 *                    returned.
 * @param [out] count Parameter will hold the count of returned subscriptions.
 * @return Pointer to the array of pointers to subscriptions, or `NULL` will be
 *         returned in case of insufficient memory error. The returned pointer
 *         must be passed to the `free` to avoid a memory leak.
 *
 * @see pubnub_subscription_set_alloc_with_entities
 * @see pubnub_subscription_set_alloc_with_subscriptions
 * @see pubnub_subscription_set_add
 */
PUBNUB_EXTERN pubnub_subscription_t** pubnub_subscription_set_subscriptions(
    pubnub_subscription_set_t* set,
    size_t* count);

/**
 * @brief Add subscription to subscription set.
 *
 * Update list of set's subscription objects used in sub loop.
 * @code
 * if (PNR_OUT_OF_MEMORY == pubnub_subscription_set_add(subscription_set,
 *                                                      subscription3)) {
 *     // handle error because of insufficient memory.
 * }
 *
 * // Following addition won't change anything because it is already in the set.
 * if (PNR_SUB_ALREADY_ADDED == pubnub_subscription_set_add(subscription_set,
 *                                                          subscription3)) {
 *     // .. because we already have it in the set.
 * }
 * @endcode
 *
 * \b Important: `sub` subscription `options` will be ignored.<br/>
 * \b Important: `pubnub_subscribe_with_subscription` will be called for `sub`
 * if `set` previously has been used for
 * `pubnub_subscribe_with_subscription_set`.<br/>
 * \b Warning: Application execution will be terminated if subscription `set` or
 * subscription is `NULL`.
 *
 * @param set Pointer to the subscription set, which should be modified.
 * @param sub Pointer to the subscription, which should be added to the set.
 * @return Result of subscription add.
 *
 * @see pubnub_subscription_set_alloc_with_entities
 * @see pubnub_subscription_set_alloc_with_subscriptions
 * @see pubnub_subscribe_with_subscription_set
 * @see pubnub_subscribe_with_subscription
 */
PUBNUB_EXTERN enum pubnub_res pubnub_subscription_set_add(
    pubnub_subscription_set_t* set,
    pubnub_subscription_t* sub);

/**
 * @brief Remove subscription from subscription set.
 *
 * Update list of set's subscription objects used in subscription loop.
 * @code
 * if (PNR_SUB_NOT_FOUND == pubnub_subscription_set_remove(subscription_set,
 *                                                         &subscription2)) {
 *     // subscription is not part of set and can't be removed from it.
 * }
 * @endcode
 *
 * \b Important: `pubnub_unsubscribe_with_subscription` will be called for `sub`
 * if `set` previously has been used for
 * `pubnub_subscribe_with_subscription_set`.<br/>
 * \b Warning: Application execution will be terminated if subscription `set` or
 * subscription is `NULL`.
 *
 * @note Function may `NULL`ify provided subscription pointer if there is no
 *       other references to it.
 *
 * @param set Pointer to the subscription set, which should be modified.
 * @param sub Pointer to the subscription, which should be removed from the set.
 * @return Result of subscription removal from the subscription set operation.
 *
 * @see pubnub_subscription_set_alloc_with_entities
 * @see pubnub_subscription_set_alloc_with_subscriptions
 * @see pubnub_subscribe_with_subscription_set
 * @see pubnub_unsubscribe_with_subscription
 */
PUBNUB_EXTERN enum pubnub_res pubnub_subscription_set_remove(
    pubnub_subscription_set_t* set,
    pubnub_subscription_t** sub);

/**
 * @brief Merge a subscription set with other subscription set.
 *
 * Update list of set's subscription objects used in subscription loop.
 * @code
 * // Add subscription objects of `subscription_set2` to the `subscription_set1`.
 * If (!pubnub_subscription_set_union(subscription_set1, subscription_set2)) {
 *     // handle sets union error (verbose logs review may help find reason)
 * }
 * @endcode
 *
 * \b Important: `other_set` subscription option will be ignored.<br/>
 * \b Important: `pubnub_subscribe_with_subscription` will be called for other
 * set `subscriptions` if `set` previously has been used for
 * `pubnub_subscribe_with_subscription_set`.<br/>
 * \b Warning: Application execution will be terminated if either of provided
 * subscription sets is `NULL`.
 *
 * @param set       Pointer to the subscription set, which should be modified.
 * @param other_set Pointer to the subscription set, which should be merged into
 *                  a set.
 * @return Result of subscription sets union operation.
 *
 * @see pubnub_subscription_set_alloc_with_entities
 * @see pubnub_subscription_set_alloc_with_subscriptions
 * @see pubnub_subscribe_with_subscription_set
 * @see pubnub_subscribe_with_subscription
 */
PUBNUB_EXTERN enum pubnub_res pubnub_subscription_set_union(
    pubnub_subscription_set_t* set,
    pubnub_subscription_set_t* other_set);

/**
 * @brief Unmerge subscription set from another subscription set.
 *
 * Update list of set's subscription objects used in subscription loop.
 * @code
 * // Remove subscription objects of `subscription_set2` from the
 * // `subscription_set1`.
 * pubnub_subscription_set_subtract(subscription_set1, subscription_set2);
 * @endcode
 *
 * \b Important: `pubnub_unsubscribe_with_subscription` will be called for other
 * set `subscriptions` if `set` previously has been used for
 * `pubnub_subscribe_with_subscription_set`.<br/>
 * \b Warning: Application execution will be terminated if either of provided
 * subscription sets is `NULL`.
 *
 * @param set       Pointer to the subscription set, which should be modified.
 * @param other_set Pointer to the subscription set, which should be unmerged
 *                  from a set.
 *
 * @see pubnub_subscription_set_alloc_with_entities
 * @see pubnub_subscription_set_alloc_with_subscriptions
 * @see pubnub_subscribe_with_subscription_set
 * @see pubnub_unsubscribe_with_subscription
 */
PUBNUB_EXTERN void pubnub_subscription_set_subtract(
    const pubnub_subscription_set_t* set,
    pubnub_subscription_set_t* other_set);

/**
 * @brief Update real-time updates filter expression.
 *
 * @code
 * // Instruct PubNub service to send only messages which have been published by
 * // PubNub instance which has been configured with `bob` as `user_id`.
 * pubnub_subscribe_set_filter_expression(pb, "uuid=='bob'");
 * @endcode
 *
 * Provided filtering expression evaluated on the server side before
 * returning a response on subscribe REST API call.<br/>
 * More information about filtering expression syntax can be found on
 * <a href="https://www.pubnub.com/docs/general/messages/publish#filter-language-definition">this</a> page.
 *
 * \b Important: The filter expression will only be applied to messages
 * which have been published with `metadata`, and the rest will be returned
 * as-is.<br/>
 *
 * @param pb          Pointer to the PubNub context, which will use the provided
 *                    expression with the next subscription loop call.
 * @param filter_expr Pointer to the real-time update filter expression.
 */
PUBNUB_EXTERN void pubnub_subscribe_set_filter_expression(
    const pubnub_t* pb,
    const char* filter_expr);

/**
 * @brief Update client presence heartbeat / timeout.
 *
 * @code
 * // Instruct PubNub service to trigger a `timeout` presence event if the
 * // client didn't send another `subscribe` or `heartbeat` request within 200
 * // seconds.
 * pubnub_subscribe_set_heartbeat(pb, 200);
 * @endcode
 *
 * \b Important: If the value is lower than the pre-defined minimum, the value
 * will be reset to the minimum.
 *
 * @note The value is an \a optional and will be set to \b 300 seconds by
 * default.
 *
 * @param pb        Pointer to the PubNub context, which will use the provided
 *                  heartbeat with the next subscription loop call.
 * @param heartbeat How long (in seconds) the server will consider the client
 *                  alive for presence.
 */
PUBNUB_EXTERN void pubnub_subscribe_set_heartbeat(
    const pubnub_t* pb,
    unsigned heartbeat);

/**
 * @brief Subscription cursor.
 * @code
 * pubnub_subscribe_cursor_t cursor = pubnub_subscribe_cursor("17231917458018858");
 * @endcode
 *
 * @param timetoken Pointer to the PubNub high-precision timetoken.
 * @return Ready to use cursor with a default region.
 */
PUBNUB_EXTERN pubnub_subscribe_cursor_t pubnub_subscribe_cursor(
    const char* timetoken);

/**
 * @brief Receive real-time updates for subscription entity.
 * @code
 * enum pubnub_res rslt = pubnub_subscribe_with_subscription(subscription, NULL);
 * if (PNR_OK != rslt) {
 *     // handle subscription error (mostly because of parameters error).
 * }
 * @endcode
 *
 * \b Warning: A single PubNub context can't have multiple subscription cursors
 * and will override the active we last used.
 *
*Cursor used by subscription loop to identify the point in time after which
 * updates will be delivered.<br/>
 * Old cursor may help return missed messages (catch up) if they are still
 * present in the cache.
 *
 *
 *
 * @param sub     Pointer to the subscription, which should be used with the
*                next subscription loop.
 * @param cursor Pointer to the subscription cursor to be used with subscribe
 *               REST API. The SDK will try to catch up on missed messages if a
 *               cursor with older PubNub high-precision timetoken has been
 *               provided. Pass `NULL` to keep using cursor received from the
 *               previous subscribe REST API response.
 * @return Result of subscription subscribe transaction.
 *
 * @see pubnub_subscription_alloc
 */
PUBNUB_EXTERN enum pubnub_res pubnub_subscribe_with_subscription(
    pubnub_subscription_t* sub,
    const pubnub_subscribe_cursor_t* cursor);

/**
 * @brief Stop receiving real-time updates for subscription entity.
 * @code
 * enum pubnub_res rslt = pubnub_unsubscribe_with_subscription(&subscription);
 * if (PNR_OK != rslt) {
 *     // handle unsubscription error (mostly because of parameters error).
 * }
 * @endcode
 *
 * @note Function may NULLify provided subscription pointer if there is no other
 *       references to it.
 *
 * @param sub Pointer to the subscription, which should be removed from the next
 *            subscription loop.
 * @return Result of subscription unsubscribe transaction.
 *
 * @see pubnub_subscription_alloc
 */
PUBNUB_EXTERN enum pubnub_res pubnub_unsubscribe_with_subscription(
    pubnub_subscription_t** sub);

/**
 * @brief Receive real-time updates for subscriptions' entities from the
 *        subscription set.
 * @code
 * enum pubnub_res rslt = pubnub_subscribe_with_subscription_set(
 *     subscription_set,
 *     NULL);
 * if (PNR_OK != rslt) {
 *     // handle subscription error (mostly because of parameters error).
 * }
 * @endcode
 *
 * \b Warning: A single PubNub context can't have multiple subscription cursors
 * and will override the active we last used.
 *
 * @param set    Pointer to the set of subscriptions, which should be used with
 *               the next subscription loop.
 * @param cursor Pointer to the subscription cursor to be used with subscribe
 *               REST API. The SDK will try to catch up on missed messages if a
 *               cursor with older PubNub high-precision timetoken has been
 *               provided. Pass `NULL` to keep using cursor received from the
 *               previous subscribe REST API response.
 * @return Result of subscription set subscribe transaction.
 *
 * @see pubnub_subscription_set_alloc_with_entities
 * @see pubnub_subscription_set_alloc_with_subscriptions
 */
PUBNUB_EXTERN enum pubnub_res pubnub_subscribe_with_subscription_set(
    pubnub_subscription_set_t* set,
    const pubnub_subscribe_cursor_t* cursor);

/**
 * @brief Stop receiving real-time updates for subscriptions' entities from
 *        subscription set.
 * @code
 * enum pubnub_res rslt = pubnub_unsubscribe_with_subscription_set(subscription_set);
 * if (PNR_OK != rslt) {
 *     // handle unsubscription error (mostly because of parameters error).
 * }
 * @endcode
 *
 * @note Function will NULLify provided subscription set pointer if it is no
 *       other references to it.
 *
 * @param set Pointer to the set of subscription, which should be removed from
 *            the next subscription loop.
 * @return Result of subscription set unsubscribe transaction.
 *
 * @see pubnub_subscription_set_alloc_with_entities
 * @see pubnub_subscription_set_alloc_with_subscriptions
 */
PUBNUB_EXTERN enum pubnub_res pubnub_unsubscribe_with_subscription_set(
    pubnub_subscription_set_t** set);

/**
 * @brief Disconnect from real-time updates.
 * @code
 * enum pubnub_res rslt = pubnub_disconnect(pb);
 * if (PNR_OK != rslt) {
 *     // handle disconnection error (mostly because of parameters error).
 * }
 * @endcode
 *
 * \b Warning: Application execution will be terminated if PubNub pointer is an
 * invalid context pointer.
 *
 * @note List of channels / groups and active time token (cursor) will be
 *       saved for subscription restore.
 *
 * @param pb Pointer to the PubNub context which should be disconnected from
 *           real-time updates.
 * @return Result of disconnection transaction.
 */
PUBNUB_EXTERN enum pubnub_res pubnub_disconnect(const pubnub_t* pb);

/**
 * @brief Reconnect for real-time updates.
 *
 * Reconnect using the last list of subscribable and timetoken to try to catch
 * up with missing messages:
 * @code
 * enum pubnub_res rslt = pubnub_reconnect(pb, NULL);
 * if (PNR_OK != rslt) {
 *     // handle reconnect error (mostly because of parameter error).
 * }
 * @endcode
 *
 * Reconnect using the last list of subscribable and custom timetoken to try to
 * catch up with missing messages from specific time:
 * @code
 * pubnub_subscribe_cursor_t cursor = pubnub_subscribe_cursor("17231917458018858");
 * enum pubnub_res rslt = pubnub_reconnect(pb, &cursor);
 * if (PNR_OK != rslt) {
 *     // handle reconnect error (mostly because of parameters error).
 * }
 * @endcode
 *
 * \b Warning: Application execution will be terminated if PubNub pointer is an
 * invalid context pointer.
 *
 * @param pb     Pointer to the PubNub context which should be reconnected to
 *               real-time updates.
 * @param cursor Pointer to the time token, which should be used with the next
 *               subscription loop. Pass `NULL` to re-use cursor from the
 *               previous event engine operation time.
 * @return Result of reconnection transaction.
 *
 * @note Subscription loop will use time token (cursor) which has been in
 *       use before disconnection / failure.
 */
PUBNUB_EXTERN enum pubnub_res pubnub_reconnect(
    const pubnub_t* pb,
    const pubnub_subscribe_cursor_t* cursor);

/**
 * @brief Unsubscribe from all channel / groups events.
 *
 * Unsubscribe from all real-time updates and notify other channel / groups
 * subscribers about client presence change (`leave`).
 * @code
 * enum pubnub_res rslt = pubnub_unsubscribe_all(pb);
 * if (PNR_OK != rslt) {
 *     // handle unsubscribe all error
 * }
 * @endcode
 *
 * \b Warning: Application execution will be terminated if PubNub pointer is an
 * invalid context pointer.
 *
 * @note The list of channels / groups and used time token (cursor) will be
 *       reset, and further real-time updates can't be received without a
 *       `subscribe` call.
 *
 * @param pb Pointer to the PubNub context which should completely unsubscribe
 *           from real-time updates.
 * @return Result of unsubscription transaction.
 */
PUBNUB_EXTERN enum pubnub_res pubnub_unsubscribe_all(const pubnub_t* pb);
#else // #if PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE
#error To use subscribe event engine API you must define PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE=1
#endif // #if PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE
#endif // #ifndef PUBNUB_SUBSCRIBE_EVENT_ENGINE_H
