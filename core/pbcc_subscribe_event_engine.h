/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#ifndef PBCC_SUBSCRIBE_EVENT_ENGINE_H
#define PBCC_SUBSCRIBE_EVENT_ENGINE_H
#if PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE
#if !PUBNUB_USE_SUBSCRIBE_V2
#error Subscribe event engine requires subscribe v2 API, so you must define PUBNUB_USE_SUBSCRIBE_V2=1
#endif // #if !PUBNUB_USE_SUBSCRIBE_V2
#ifndef PUBNUB_CALLBACK_API
#error Subscribe event engine requires callback based PubNub context, so you must define PUBNUB_CALLBACK_API
#endif // #ifndef PUBNUB_CALLBACK_API


/**
 * @file  pbcc_subscribe_event_engine.h
 * @brief Event engine implementation for subscription loop.
 */

#include "core/pbcc_subscribe_event_engine_types.h"
#include "core/pbcc_subscribe_event_listener.h"
#include "core/pubnub_subscribe_event_engine.h"


// ----------------------------------------------
//                Types forwarding
// ----------------------------------------------

/** Subscribe event engine structure. */
typedef struct pbcc_subscribe_ee pbcc_subscribe_ee_t;


// ----------------------------------------------
//                   Functions
// ----------------------------------------------

/**
 * @brief Create Subscribe Event Engine.
 *
 * @param pb PubNub context, which should be used for effects execution by
 *           Subscribe Event Engine effects dispatcher.
 * @return Pointer to the ready to use Subscribe Event Engine or `NULL` in case
 *         of out-of-memory error. The returned pointer must be passed to the
 *         `pbcc_subscribe_ee_free` to avoid a memory leak.
 */
pbcc_subscribe_ee_t* pbcc_subscribe_ee_alloc(pubnub_t* pb);

/**
 * @brief Dispose and free up resources used by Subscribe Event Engine.
 *
 * @note Function will NULLify provided subscribe event engine pointer.
 *
 * @param ee Pointer to the Subscribe Event Engine which should be disposed.
 */
void pbcc_subscribe_ee_free(pbcc_subscribe_ee_t** ee);

/**
 * @brief Get Subscribe Event Listener.
 *
 * @param ee Pointer to the Subscribe Event Engine, which should provide
 *           listener.
 * @return Pointer to the ready to use Event Listener.
 */
pbcc_event_listener_t* pbcc_subscribe_ee_event_listener(
    const pbcc_subscribe_ee_t* ee);

/**
 * @brief Get current state context.
 *
 * @param ee Pointer to the Subscribe Event Engine, which should provide current
 *           state object.
 * @return Current Subscribe Event Engine context data.
 */
pbcc_ee_data_t* pbcc_subscribe_ee_current_state_context(
    const pbcc_subscribe_ee_t* ee);

/**
 * @brief Update real-time updates filter expression.
 *
 * @param ee          Pointer to the Subscribe Event Engine, which will use the
 *                    provided expression with the next subscription loop call.
 * @param filter_expr Pointer to the real-time update filter expression.
 */
void pbcc_subscribe_ee_set_filter_expression(
    pbcc_subscribe_ee_t* ee,
    const char* filter_expr);

/**
 * @brief Update client presence heartbeat / timeout.
 *
 * @param ee        Pointer to the Subscribe Event Engine, which will use the
 *                  provided heartbeat with the next subscription loop call.
 * @param heartbeat How long (in seconds) the server will consider the client
 *                  alive for presence.
 */
void pbcc_subscribe_ee_set_heartbeat(
    pbcc_subscribe_ee_t* ee,
    unsigned heartbeat);

/**
 * @brief Transit to `receiving` state.
 *
 * Depending on from the flow, Subscribe Event Engine will transit to the
 * `receiving` state directly or through the `handshaking` state (if required).
 *
 * \b Warning: A single PubNub context can't have multiple subscription options
 * and will override the active we last used.
 *
 * @param ee     Pointer to the Subscribe Event Engine, which should transit to
 *               `receiving` state.
 * @param sub    Pointer to the subscription, which should be used to update
 *               context data.
 * @param cursor Pointer to the subscription cursor to be used with subscribe
 *               REST API. The SDK will try to catch up on missed messages if a
 *               cursor with older PubNub high-precision timetoken has been
 *               provided. Pass `NULL` to keep using cursor received from the
 *               previous subscribe REST API response.
 * @return Result of `subscribe` enqueue transaction.
 */
enum pubnub_res pbcc_subscribe_ee_subscribe_with_subscription(
    pbcc_subscribe_ee_t* ee,
    pubnub_subscription_t* sub,
    const pubnub_subscribe_cursor_t* cursor);

/**
 * @brief Transit to `unsubscribed` / `receiving` state.
 *
 * Depending on from data stored in the Subscription Event Engine, transition
 * may happen to the `receiving` or `unsubscribed`.
 *
 * @param ee  Pointer to the Subscribe Event Engine, which should transit to the
 *            state suitable for the current context state.
 * @param sub Pointer to the subscription, which should be used to update context
 *            data.
 * @return Result of `unsubscribe` enqueue transaction.
 */
enum pubnub_res pbcc_subscribe_ee_unsubscribe_with_subscription(
    pbcc_subscribe_ee_t* ee,
    const pubnub_subscription_t* sub);

/**
 * @brief Transit to `receiving` state.
 *
 * Depending on from the flow, Subscribe Event Engine will transit to the
 * `receiving` state directly or through the `handshaking` state (if required).
 *
 * \b Warning: A single PubNub context can't have multiple subscription options
 * and will override the active we last used.
 *
 * @param ee     Pointer to the Subscribe Event Engine, which should transit to
 *               `receiving` state.
 * @param set    Pointer to the subscription set, which should be used to update
 *               context data.
 * @param cursor Pointer to the subscription cursor to be used with subscribe
 *               REST API. The SDK will try to catch up on missed messages if a
 *               cursor with older PubNub high-precision timetoken has been
 *               provided. Pass `NULL` to keep using cursor received from the
 *               previous subscribe REST API response.
 * @return Result of `subscribe` enqueue transaction.
 */
enum pubnub_res pbcc_subscribe_ee_subscribe_with_subscription_set(
    pbcc_subscribe_ee_t* ee,
    pubnub_subscription_set_t* set,
    const pubnub_subscribe_cursor_t* cursor);

/**
 * @brief Transit to `unsubscribed` / `receiving` state.
 *
 * Depending on from data stored in the Subscription Event Engine, transition
 * may happen to the `receiving` or `unsubscribed`.
 *
 * @param ee  Pointer to the Subscribe Event Engine, which should transit to the
 *            state suitable for the current context state.
 * @param set Pointer to the subscription set, which should be used to update
 *            context data.
 * @return Result of `unsubscribe` enqueue transaction.
 */
enum pubnub_res pbcc_subscribe_ee_unsubscribe_with_subscription_set(
    pbcc_subscribe_ee_t* ee,
    pubnub_subscription_set_t* set);

/**
 * @brief Subscription set modification handler.
 *
 * Handle changes to subscribed set subscriptions list.
 *
 * @param ee    Pointer to the Subscribe Event Engine, which should transit to
 *              the state suitable for the current context state.
 * @param set   Pointer to the subscription set, which has been updated.
 * @param sub   Pointer to the subscription which has been added or removed.
 * @param added Whether new subscription has been added to the `set` or not.
 * @return Result of `subscription change` enqueue transaction.
 */
enum pubnub_res pbcc_subscribe_ee_change_subscription_with_subscription_set(
    pbcc_subscribe_ee_t* ee,
    const pubnub_subscription_set_t* set,
    const pubnub_subscription_t* sub,
    bool added);

/**
 * @brief Transit to `disconnected` state.
 *
 * Make transition from current event engine state to the `disconnected` state.
 *
 * \b Warning: Application execution will be terminated if Subscribe Event Engine
 * pointer is an invalid.
 *
 * @param ee Pointer to the Subscribe Event Engine, which should transit to
 *           `disconnected` state.
 * @return Result of `disconnect` enqueue transaction.
 */
enum pubnub_res pbcc_subscribe_ee_disconnect(
    pbcc_subscribe_ee_t* ee);

/**
 * @brief Transit to `receiving` state.
 *
 * Make transition from the current event engine state to the `receiving` state.
 *
 * \b Warning: Application execution will be terminated if Subscribe Event Engine
 * pointer is an invalid.
 *
 * @param ee     Pointer to the Subscribe Event Engine, which should transit to
 *               `receiving` state.
 * @param cursor Custom catch up time cursor to start receiving real-time
 *               updates from specific time.
 * @return Result of `reconnect` enqueue transaction.
 */
enum pubnub_res pbcc_subscribe_ee_reconnect(
    pbcc_subscribe_ee_t* ee,
    pubnub_subscribe_cursor_t cursor);

/**
 * @brief Transit to `unsubscribed` state.
 *
 * @param ee Pointer to the Subscribe Event Engine, which should transit to
 *           `unsubscribed` state.
 * @return Result of `unsubscribe all` enqueue transaction.
 */
enum pubnub_res pbcc_subscribe_ee_unsubscribe_all(pbcc_subscribe_ee_t* ee);

/**
 * @brief Handle Subscribe v2 request schedule error.
 *
 * @param ee    Pointer to the Subscribe Event Engine, which should handle next
 *              subscription schedule error.
 * @param error Subscribe operation call result (error).
 */
void pbcc_subscribe_ee_handle_subscribe_error(
    pbcc_subscribe_ee_t* ee,
    enum pubnub_res error);

/**
 * @brief Retrieve created subscriptions.
 *
 * \b Warning: Application execution will be terminated if Subscribe Event Engine
 * pointer is an invalid.
 *
 * @param ee          Pointer to the Subscribe Event Engine from which list of
 *                    subscriptions should be retrieved.
 * @param [out] count Parameter will hold the count of returned subscriptions.
 * @return Pointer to the array with subscription object pointers, or `NULL` in
 *         case of error (insufficient memory or PubNub pointer is an invalid
 *         context pointer). The returned pointer must be passed to the `free` to
 *         avoid a memory leak.
 */
pubnub_subscription_t** pbcc_subscribe_ee_subscriptions(
    pbcc_subscribe_ee_t* ee,
    size_t* count);

/**
 * @brief Retrieve created subscription sets.
 *
 * \b Warning: Application execution will be terminated if Subscribe Event Engine
 * pointer is an invalid.
 *
 * @param ee          Pointer to the Subscribe Event Engine from which list of
 *                    subscription sets should be retrieved.
 * @param [out] count Parameter will hold the count of returned subscription
 *                    sets.
 * @return Pointer to the array with subscription set object pointers, or `NULL`
 *         will be returned in case of insufficient memory error. The returned pointer
 *         must be passed to the `free` to avoid a memory leak.
 */
pubnub_subscription_set_t** pbcc_subscribe_ee_subscription_sets(
    pbcc_subscribe_ee_t* ee,
    size_t* count);
#else // #if PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE
#error To use subscribe event engine API you must define PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE=1
#endif // #if PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE
#endif // #ifndef PBCC_SUBSCRIBE_EVENT_ENGINE_H
