#include "pbcc_subscribe_event_engine.h"

#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "pbcc_memory_utils.h"
#include "pbcc_subscribe_event_listener.h"
#include "core/pubnub_subscribe_event_engine_internal.h"
#include "core/pubnub_internal_common.h"
#include "core/pubnub_server_limits.h"
#include "core/pbcc_event_engine.h"
#include "core/pubnub_mutex.h"
#include "core/pubnub_log.h"
#include "pubnub_callback.h"
#include "lib/pbhash_set.h"
#include "lib/pbstrdup.h"


// ----------------------------------------------
//                   Constants
// ----------------------------------------------

// How many subscribable objects `pbcc_subscribe_ee_t` can hold by default.
#define SUBSCRIBABLE_LENGTH 10


// ----------------------------------------------
//                     Types
// ----------------------------------------------

// Subscribe Event Engine events.
typedef enum {
    // Subscription list of channels / groups change event.
    SUBSCRIBE_EE_EVENT_SUBSCRIPTION_CHANGED,
    // Subscription restore / catchup event.
    SUBSCRIBE_EE_EVENT_SUBSCRIPTION_RESTORED,
    // Initial subscription handshake success event.
    SUBSCRIBE_EE_EVENT_HANDSHAKE_SUCCESS,
    // Initial subscription handshake failure event.
    SUBSCRIBE_EE_EVENT_HANDSHAKE_FAILURE,
    // Real-time updates receive success event.
    SUBSCRIBE_EE_EVENT_RECEIVE_SUCCESS,
    // Real-time updates receive failure event.
    SUBSCRIBE_EE_EVENT_RECEIVE_FAILURE,
    // Disconnect from real-time updates event.
    SUBSCRIBE_EE_EVENT_DISCONNECT,
    // Reconnect for real-time updates event.
    SUBSCRIBE_EE_EVENT_RECONNECT,
    // Unsubscribe from all channel / groups.
    SUBSCRIBE_EE_EVENT_UNSUBSCRIBE_ALL,
} pbcc_subscribe_ee_event;

// Subscribe Event Engine states.
typedef enum {
    /**
     * @brief Unknown state.
     *
     * Placeholder state type without actual implementation which won't be
     * treated as insufficient memory error.
     */
    SUBSCRIBE_EE_STATE_NONE,
    // Inactive Subscribe Event Engine state.
    SUBSCRIBE_EE_STATE_UNSUBSCRIBED,
    // Initial subscription state.
    SUBSCRIBE_EE_STATE_HANDSHAKING,
    // Initial subscription failed state.
    SUBSCRIBE_EE_STATE_HANDSHAKE_FAILED,
    // Initial subscription stopped state.
    SUBSCRIBE_EE_STATE_HANDSHAKE_STOPPED,
    // Real-time updates receiving state.
    SUBSCRIBE_EE_STATE_RECEIVING,
    // Real-time updates receive failed state.
    SUBSCRIBE_EE_STATE_RECEIVE_FAILED,
    // Real-time updates receive stopped state.
    SUBSCRIBE_EE_STATE_RECEIVE_STOPPED,
} pbcc_subscribe_ee_state;

// Subscribe event engine structure.
struct pbcc_subscribe_ee {
    /**
     * @brief Pointer to the set of subscribable objects which should be used in
     *        subscription loop.
     */
    pbhash_set_t* subscribables;
    /**
     * @brief Pointer to the list of active subscription sets.
     *
     * List of subscription sets which should be used in subscription loop and
     * listen for real-time updates.
     */
    pbarray_t* subscription_sets;
    /**
     * @brief List of active subscriptions.
     *
     * List of subscriptions which should be used in subscription loop and
     * listen for real-time updates.
     */
    pbarray_t* subscriptions;
    /**
     * @brief How long (in seconds) the server will consider the client alive
     *        for presence.
     */
    unsigned heartbeat;
    // Pointer to the real-time updates filtering expression.
    char* filter_expr;
    // Recent subscription status.
    pubnub_subscription_status status;
    // Subscribe Event Listener.
    pbcc_event_listener_t* event_listener;
    /**
     * @brief Pointer to the Event Engine which handles all states and
     *        transitions on events.
     */
    pbcc_event_engine_t* ee;
    /**
     * @brief Pointer to the PubNub context, which should be used for effects
     *        execution by Subscribe Event Engine effects dispatcher.
     */
    pubnub_t* pb;
    // Shared resources access lock.
    pubnub_mutex_t mutw;
};

/**
 * @brief Events and state context object.
 *
 * Context object contains information related required for proper
 * Subscribe Event Engine operation. States and events contain event engine
 * context 'snapshot' at the moment when they have been created.
 */
typedef struct {
    /**
     * @brief Pointer to the comma-separated channels which are or will be used
     *        in subscription loop.
     */
    pbcc_ee_data_t* channels;
    /**
     * @brief Pointer to the comma-separated channel groups which are or will be
     *        used in subscription loop.
     */
    pbcc_ee_data_t* channel_groups;
    // Subscription cursor which is or will be used in subscription loop.
    const pubnub_subscribe_cursor_t cursor;
    // Previous request failure reason.
    enum pubnub_res reason;
    /**
     * @brief Pointer to the PubNub context, which should be used for effects
     *        execution by Subscribe Event Engine effects dispatcher.
     */
    pubnub_t* pb;
} pbcc_subscribe_ee_context_t;


// ----------------------------------------------
//       Function prototypes: Event Engine
// ----------------------------------------------

/**
 * @brief Transit to `handshaking` / `receiving` state.
 *
 * Depending on from the flow, Subscribe Event Engine will transit to the
 * `receiving` state directly or through the `handshaking` state (if required).
 * Updated list of subscribables will be used to trigger proper event for
 * subscription loop.
 *
 * @param ee                   Pointer to the Subscribe Event Engine, which
 *                             should transit to `receiving` state.
 * @param cursor               Pointer to the subscription cursor to be used
 *                             with subscribe REST API. The SDK will try to
 *                             catch up on missed messages if a cursor with
 *                             older PubNub high-precision timetoken has been
 *                             provided. Pass `NULL` to keep using cursor
 *                             received from the previous subscribe REST API
 *                             response.
 * @param update Whether list of subscribable objects should be
 *                             updated or not (for unsubscribe, it updated
 *                             beforehand).
 * @return Result of subscribe enqueue transaction.
 */
static enum pubnub_res _pbcc_subscribe_ee_subscribe(
    const pbcc_subscribe_ee_t*       ee,
    const pubnub_subscribe_cursor_t* cursor,
    bool                             update);

/**
 * @brief Transit to `handshaking` / `receiving` or `unsubscribed` state.
 *
 * Depending on from whether there are any channels left for the next
 * subscription loop can be created, `SUBSCRIBE_EE_EVENT_UNSUBSCRIBE_ALL` event instead of
 * `EVENT_SUBSCRIPTION_CHANGE`.
 *
 * @param ee            Pointer to the Subscribe Event Engine, which should
 *                      transit to `handshaking` / `receiving` or `unsubscribed`
 *                      state.
 * @param subscribables Pointer to the hash with subscribables from which user
 *                      requested to unsubscribed.
 * @return Result of unsubscribe enqueue transaction.
 */
static enum pubnub_res _pbcc_subscribe_ee_unsubscribe(
    const pbcc_subscribe_ee_t* ee,
    pbhash_set_t*              subscribables);

/**
 * @berief Actualize list of subscribable objects.
 *
 * Update list of subscribable objects using `active` subscriptions and
 * subscription sets.
 *
 * @param ee Pointer to the Subscribe Event Engine for which list of
 *           subscribable objects should be updated.
 * @return Result of subscribable list update.
 *
 * @note Because of subscription / subscription set unsubscribe process, it is
 *       impossible to implement subscribable update in
 *       `_subscribe` / `_unsubscribe` methods. Subscribable addition, won't be
 *       a problem, but the removal process won't be able to figure out whether
 *       subscribable has been added by standalone subscribe object alone or
 *       another subscription object from subscription set tried to do the same.
 *       As a result, the object may be removed from the subscription loop.
 *
 * @see pubnub_subscription_t
 * @see pubnub_subscription_set_t
 */
static enum pubnub_res _pbcc_subscribe_ee_update_subscribables(
    const pbcc_subscribe_ee_t* ee);

/**
 * @brief Update Subscribe Event Engine list of subscribable objects.
 *
 * @param ee            Pointer to the Subscribe Event Engine for which
 *                      `subscribables` should be added.
 * @param subscribables Pointer to the hash set with subscribable objects which
 *                      is used for channels and channel groups list aggregation
 *                      for subscription loop.
 * @return Result of subscribables addition operation.
 */
static enum pubnub_res _pbcc_subscribe_ee_add_subscribables(
    const pbcc_subscribe_ee_t* ee,
    const pbhash_set_t*        subscribables);

/**
 * @brief Get list of channel and group identifiers from provided subscribables.
 *
 * @param subscribables        Pointer to the hash set with unique subscribable
 *                             objects for which list of comma-separated
 *                             identifiers should be generated.
 * @param [out] channels       The parameter will hold a pointer to the
 *                             byte string with comma-separated / empty channel
 *                             identifiers for subscribe loop or `NULL` in case
 *                             of insufficient memory error.
 * @param [out] channel_groups The parameter will hold a pointer to the
 *                             byte string with comma-separated / empty channel
 *                             group identifiers for subscribe loop or `NULL` in
 *                             case of insufficient memory error.
 * @param include_presence     Whether channels with `-pnpres` should be
 *                             included into `channels` and `channel_groups`.
 * @return Result of subscribables computation operation.
 */
static enum pubnub_res _pbcc_subscribe_ee_subscribables(
    pbhash_set_t* subscribables,
    char**        channels,
    char**        channel_groups,
    bool          include_presence);

/**
 * @brief Create Subscribe Event Engine context object.
 *
 * @param pb                      PubNub context, which should be used for
 *                                effects execution by Subscribe Event Engine
 *                                effects dispatcher.
 * @param [in,out] channels       Pointer to the byte string with
 *                                comma-separated of channels which should be
 *                                passed between states in context object.
 * @param [in,out] channel_groups Pointer to the byte string with
 *                                comma-separated of channel groups which should
 *                                be passed between states in context object.
 * @return Pointer to the ready to use Subscribe Event Engine context object, or
 *         `NULL` in case of insufficient memory error. The returned pointer
 *         must be passed to the `_pbcc_subscribe_ee_context_free` to avoid a
 *         memory leak.
 */
static pbcc_subscribe_ee_context_t* _pbcc_subscribe_ee_context_alloc(
    pubnub_t* pb,
    char**    channels,
    char**    channel_groups);

/**
 * @brief Copy source context with `channels` and `channel_groups` override.
*
 * @param pb                      PubNub context, which should be used for
 *                                effects execution by Subscribe Event Engine
 *                                effects dispatcher.
 * @param ctx                     Pointer to the context, from which data will
 *                                be copied to the next Subscribe Event Engine
 *                                context.
 * @param [in,out] channels       Pointer to the byte string with
 *                                comma-separated channels which should be used
 *                                instead of `channels` from provided context.
 * @param [in,out] channel_groups Pointer to the byte string with
 *                                comma-separated channel groups which should be
 *                                used instead of `channel_groups` from provided
 *                                context.
 * @return Pointer to the ready to use Subscribe Event Engine context created
 *         from the data of source context, or `NULL` in case of insufficient
 *         memory error. The returned pointer must be passed to the
 *         `_pbcc_subscribe_ee_context_free` to avoid a memory leak.
 */
static pbcc_subscribe_ee_context_t* _pbcc_subscribe_ee_context_copy(
    pubnub_t*                          pb,
    const pbcc_subscribe_ee_context_t* ctx,
    char**                             channels,
    char**                             channel_groups);

/**
 * @brief Get current state context.
 *
 * @param ee Pointer to the Subscribe Event Engine, which should provide current
 *           state object.
 * @return Current Subscribe Event Engine context.
 */
static pbcc_subscribe_ee_context_t* _pbcc_subscribe_ee_current_state_context(
    const pbcc_subscribe_ee_t* ee);

/**
 * @brief Clean up resources used by subscription context.
 *
 * @param ctx Pointer to the context, which should free up resources.
 */
static void _pbcc_subscribe_ee_context_free(pbcc_subscribe_ee_context_t* ctx);

/**
 * @brief Handle real-time updates from subscription loop.
 *
 * @param pb        Pointer to the PubNub context which received updates in
 *                  subscription loop.
 * @param trans     Type of transaction for which callback has been called.
 * @param result    Result of `trans` processing
 * @param user_data Pointer to the Subscribe Event Engine object.
 */
static void _pbcc_subscribe_callback(
    pubnub_t*         pb,
    enum pubnub_trans trans,
    enum pubnub_res   result,
    const void*       user_data);


// ----------------------------------------------
//          Function prototypes: State
// ----------------------------------------------

/**
 * @brief Inactive Subscribe Event Engine state.
 *
 * State with which Subscribe Event Engine will be initialized and end up
 * when there will be no channels or groups to subscribe to.
 *
 * @return Pointer to the `Unsubscribed` state.
 */
static pbcc_ee_state_t* _pbcc_unsubscribed_state_alloc(void);

/**
 * @brief Initial subscription state.
 *
 * State, which is used to perform initial subscription to receive the next
 * subscription loop time token (cursor) and notify channels / groups subscribes
 * about change in subscriber's presence.
 *
 * @param context Pointer to the context with updated Subscribe Event Engine
 *                information which should be used for the next subscription
 *                loop.
 * @return Pointer to the `Handshaking state`, which should be used as
 *         Subscribe Event Engine state transition target state.
 */
static pbcc_ee_state_t* _pbcc_handshaking_state_alloc(pbcc_ee_data_t* context);

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
static pbcc_ee_state_t* _pbcc_handshake_failed_state_alloc(
    pbcc_ee_data_t* context);

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
static pbcc_ee_state_t* _pbcc_handshake_stopped_state_alloc(
    pbcc_ee_data_t* context);

/**
 * @brief Real-time updates receiving state.
 *
 * State, which is used by the Subscribe Event Engine to perform a long-poll
 * subscription loop to receive real-time updates.
 *
 * @param context Pointer to the context with list of channels / groups and time
 *                token (cursor) received from previous subscription loop to
 *                receive next real-time updates.
 * @return Pointer to the `Receiving real-time updates state`, which should be
 *         used as Subscribe Event Engine state transition target state.
 */
static pbcc_ee_state_t* _pbcc_receiving_state_alloc(pbcc_ee_data_t* context);

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
static pbcc_ee_state_t* _pbcc_receive_failed_state_alloc(
    pbcc_ee_data_t* context);

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
 * @return Pointer to the `Receive real-time updates stopped state`, which
 *         should be used as Subscribe Event Engine state transition target
 *         state.
 */
static pbcc_ee_state_t* _pbcc_receive_stopped_state_alloc(
    pbcc_ee_data_t* context);


// ----------------------------------------------
//          Function prototypes: Event
// ----------------------------------------------

/**
 * @brief Subscription list of channels / groups change event.
 *
 * This event will be emitted each time when user would like to add new or
 * remove already active channels / channel groups.
 *
 * @note State and subscription context objects will handle further memory
 *       management of the provided channels and channel groups byte string
 *       pointers.
 *
 * @param ee                      Pointer to the Subscribe Event Engine for
 *                                which will process the event.
 * @param [in,out] channels       Pointer to the byte sting of comma-separated
 *                                channels from which PubNub client should
 *                                receive real-time updates.
 * @param [in,out] channel_groups Pointer to the byte sting of comma-separated
 *                                channel groups from which PubNub client should
 *                                receive real-time updates.
 * @return Pointer to the `Subscription change event`, which will be processed
 *         by the Subscribe Event Engine to get transition instructions from the
 *         current state.
 */
static pbcc_ee_event_t* _pbcc_subscription_changed_event_alloc(
    const pbcc_subscribe_ee_t* ee,
    char**                     channels,
    char**                     channel_groups);

/**
 * @brief Subscription restore / catchup event.
 *
 * This event will be emitted each time when user would like to add new or
 * remove already active channels / channel groups with specific time token
 * (cursor) to catch up messages from.
 *
 * @note State and subscription context objects will handle further memory
 *       management of the provided channels and channel groups byte string
 *       pointers.
 *
 * @param ee                      Pointer to the Subscribe Event Engine for
 *                                which will process the event.
 * @param [in,out] channels       Pointer to the byte string of comma-separated
 *                                channels from which PubNub client should
 *                                receive real-time updates.
 * @param [in,out] channel_groups Pointer to the byte string of comma-separated
 *                                channel groups from which PubNub client should
 *                                receive real-time updates.
 * @param cursor                  Time token to which PubNub client should try
 *                                to restore subscription (catch up on missing
 *                                messages) loop.
 * @return Pointer to the `Subscription restore event`, which will be processed
 *         by the Subscribe Event Engine to get transition instructions from the
 *         current state.
 */
static pbcc_ee_event_t* _pbcc_subscription_restored_event_alloc(
    const pbcc_subscribe_ee_t* ee,
    char**                     channels,
    char**                     channel_groups,
    pubnub_subscribe_cursor_t  cursor);

/**
 * @brief Initial subscription handshake success event.
 *
 * This event will be emitted each time when initial subscription to the set of
 * channels / groups will complete with a service response which contains the
 * time token (cursor) for the next subscription loop.
 *
 * @param ee     Pointer to the Subscribe Event Engine for which will process
 *               the event.
 * @param cursor Time token, which should be used with next subscription loop.
 * @return Pointer to the `Subscription handshake success event`, which will be
 *         processed by the Subscribe Event Engine to get transition
 *         instructions from the current state.
 */
static pbcc_ee_event_t* _pbcc_handshake_success_event_alloc(
    const pbcc_subscribe_ee_t* ee,
    pubnub_subscribe_cursor_t  cursor);

/**
 * @brief Initial subscription handshake failure event.
 *
 * This event will be emitted each time when initial subscription to the set of
 * channels / groups fails. Failure in most of the cases caused by permissions
 * and network issues.
 *
 * @param ee     Pointer to the Subscribe Event Engine for which will process
 *               the event.
 * @param reason Subscription processing result code which explains failure
 *               reason.
 * @return Pointer to the `Subscription handshake failure event`, which will be
 *         processed by the Subscribe Event Engine to get transition
 *         instructions from the current state.
 */
static pbcc_ee_event_t* _pbcc_handshake_failure_event_alloc(
    const pbcc_subscribe_ee_t* ee,
    enum pubnub_res            reason);

/**
 * @default Real-time updates receive success event.
 *
 * This event will be emitted each time when subscription receives a response
 * from PubNub service.
 *
 * @param ee     Pointer to the Subscribe Event Engine for which will process
 *               the event.
 * @param cursor Time token, which should be used with next subscription loop.
 * @return Pointer to the `Subscription receive success event`, which will be
 *         processed by the Subscribe Event Engine to get transition
 *         instructions from the current state.
 */
static pbcc_ee_event_t* _pbcc_receive_success_event_alloc(
    const pbcc_subscribe_ee_t* ee,
    pubnub_subscribe_cursor_t  cursor);

/**
* @brief Real-time updates receive failure event.
 *
 * @param ee     Pointer to the Subscribe Event Engine for which will process
 *               the event.
 * @param reason Subscription processing result code which explain failure
 *               reason.
 * @return Pointer to the `Subscription receive failure event`, which will be
 *         processed by the Subscribe Event Engine to get transition
 *         instructions from the current state.
 */
static pbcc_ee_event_t* _pbcc_receive_failure_event_alloc(
    const pbcc_subscribe_ee_t* ee,
    enum pubnub_res            reason);

/**
 * @brief Disconnect from real-time updates event.
 *
 * @note List of channels / groups and active time token (cursor) will be
 *       saved for subscription restore.
 *
 * @param ee Pointer to the Subscribe Event Engine for which will process the
 *           event.
 * @return Pointer to the `Disconnect event`, which will be processed by the
 *         Subscribe Event Engine to get transition instructions from the
 *         current state.
 */
static pbcc_ee_event_t* _pbcc_disconnect_event_alloc(
    const pbcc_subscribe_ee_t* ee);

/**
 * @brief Reconnect for real-time updates event.
 *
 * @note Subscription loop will use time token (cursor) which has been in
 *       use before disconnection / failure.
 *
 * @param ee     Pointer to the Subscribe Event Engine for which will process
 *               the event.
 * @param cursor Time token, which should be used with next subscription
 *               REST API loop call.
 * @return Pointer to the `Reconnect event`, which will be processed by the
 *         Subscribe Event Engine to get transition instructions from the
 *         current state.
 */
static pbcc_ee_event_t* _pbcc_reconnect_event_alloc(
    const pbcc_subscribe_ee_t* ee,
    pubnub_subscribe_cursor_t  cursor);

/**
 * @brief Unsubscribe from all channel / groups event.
 *
 * Unsubscribe from all real-time updates and notify other channel / groups
 * subscribers about client presence change (`leave`).
 *
 * @note The list of channels / groups and used time token (cursor) will be
 *       reset, and further real-time updates can't be received without a
 *       `subscribe` call.
 * @note State and subscription context objects will handle further memory
 *       management of the provided channels and channel groups byte string
 *       pointers.
 *
 * @param ee                      Pointer to the Subscribe Event Engine for
 *                                which will process the event.
 * @param [in,out] channels       Pointer to the byte sting of comma-separated
 *                                channels from which PubNub client should
 *                                receive real-time updates.
 * @param [in,out] channel_groups Pointer to the byte string of comma-separated
 *                                channel groups from which PubNub client should
 *                                receive real-time updates.
 * @return Pointer to the `Unsubscribe all event`, which will be processed by
 *         the Subscribe Event Engine to get transition instructions from the
 *         current state.
 */
static pbcc_ee_event_t* _pbcc_unsubscribe_all_event_alloc(
    const pbcc_subscribe_ee_t* ee,
    char**                     channels,
    char**                     channel_groups);

/**
 * @brief Create event for underlying Event Engine.
 *
 * @param ee                      Pointer to the Subscribe Event Engine for
 *                                which will process the event.
 * @param event                   Type of Subscribe Event Engine event.
 * @param [in,out] channels       Pointer to the byte sting of comma-separated
 *                                channels from which PubNub client should
 *                                receive real-time updates.
 * @param [in,out] channel_groups Pointer to the byte string of comma-separated
 *                                channel groups from which PubNub client should
 *                                receive real-time updates.
 * @param cursor                  Time token, which should be used with next
 *                                subscription REST API loop call.
 * @param reason                  Subscription processing result code which
 *                                explain failure reason.
 * @return Pointer to requested `event` type, which will be processed by the
 *         Subscribe Event Engine to get transition instructions from the
 *         current state.
 */
static pbcc_ee_event_t* _pbcc_subscribe_ee_event_alloc(
    const pbcc_subscribe_ee_t*       ee,
    pbcc_subscribe_ee_event          event,
    char**                           channels,
    char**                           channel_groups,
    const pubnub_subscribe_cursor_t* cursor,
    const enum pubnub_res*           reason);


// ----------------------------------------------
//       Function prototypes: Transition
// ----------------------------------------------

/**
 * @brief Create transition from `Unsubscribed` state basing on received
 *        `event`.
 *
 * @param ee            Pointer to the Event Engine which requested for
 *                      transition details.
 * @param current_state Pointer to the current Event Engine state from which
 *                      transition should be decided.
 * @param event         Pointer to the Event Engine state changing event with
 *                      information required for transition.
 * @return Pointer to the state transition object or `NULL` in case of
 *         insufficient memory error.
 */
static pbcc_ee_transition_t* _pbcc_unsubscribed_state_transition_alloc(
    const pbcc_event_engine_t* ee,
    const pbcc_ee_state_t*     current_state,
    const pbcc_ee_event_t*     event);

/**
 * @brief Create transition from `Handshaking` state basing on received `event`.
 *
 * @param ee            Pointer to the Event Engine which requested for
 *                      transition details.
 * @param current_state Pointer to the current Event Engine state from which
 *                      transition should be decided.
 * @param event         Pointer to the Event Engine state changing event with
 *                      information required for transition.
 * @return Pointer to the state transition object or `NULL` in case of
 *         insufficient memory error.
 */
static pbcc_ee_transition_t* _pbcc_handshaking_state_transition_alloc(
    const pbcc_event_engine_t* ee,
    const pbcc_ee_state_t*     current_state,
    const pbcc_ee_event_t*     event);

/**
 * @brief Create transition from `Handshake Failed` state basing on received
 *        `event`.
 *
 * @param ee            Pointer to the Event Engine which requested for
 *                      transition details.
 * @param current_state Pointer to the current Event Engine state from which
 *                      transition should be decided.
 * @param event         Pointer to the Event Engine state changing event with
 *                      information required for transition.
 * @return Pointer to the state transition object or `NULL` in case of
 *         insufficient memory error.
 */
static pbcc_ee_transition_t* _pbcc_handshake_failed_state_transition_alloc(
    const pbcc_event_engine_t* ee,
    const pbcc_ee_state_t*     current_state,
    const pbcc_ee_event_t*     event);

/**
 * @brief Create transition from `Handshake Stopped` state basing on received
 *        `event`.
 *
 * @param ee            Pointer to the Event Engine which requested for
 *                      transition details.
 * @param current_state Pointer to the current Event Engine state from which
 *                      transition should be decided.
 * @param event         Pointer to the Event Engine state changing event with
 *                      information required for transition.
 * @return Pointer to the state transition object or `NULL` in case of
 *         insufficient memory error.
 */
static pbcc_ee_transition_t* _pbcc_handshake_stopped_state_transition_alloc(
    const pbcc_event_engine_t* ee,
    const pbcc_ee_state_t*     current_state,
    const pbcc_ee_event_t*     event);

/**
 * @brief Create transition from `Receiving` state basing on received `event`.
 *
 * @param ee            Pointer to the Event Engine which requested for
 *                      transition details.
 * @param current_state Pointer to the current Event Engine state from which
 *                      transition should be decided.
 * @param event         Pointer to the Event Engine state changing event with
 *                      information required for transition.
 * @return Pointer to the state transition object or `NULL` in case of
 *         insufficient memory error.
 */
static pbcc_ee_transition_t* _pbcc_receiving_state_transition_alloc(
    const pbcc_event_engine_t* ee,
    const pbcc_ee_state_t*     current_state,
    const pbcc_ee_event_t*     event);

/**
 * @brief Create transition from `Receiving Failed` state basing on received
 *        `event`.
 *
 * @param ee            Pointer to the Event Engine which requested for
 *                      transition details.
 * @param current_state Pointer to the current Event Engine state from which
 *                      transition should be decided.
 * @param event         Pointer to the Event Engine state changing event with
 *                      information required for transition.
 * @return Pointer to the state transition object or `NULL` in case of
 *         insufficient memory error.
 */
static pbcc_ee_transition_t* _pbcc_receiving_failed_state_transition_alloc(
    const pbcc_event_engine_t* ee,
    const pbcc_ee_state_t*     current_state,
    const pbcc_ee_event_t*     event);

/**
 * @brief Create transition from `Receiving Stopped` state basing on received
 *        `event`.
 *
 * @param ee            Pointer to the Event Engine which requested for
 *                      transition details.
 * @param current_state Pointer to the current Event Engine state from which
 *                      transition should be decided.
 * @param event         Pointer to the Event Engine state changing event with
 *                      information required for transition.
 * @return Pointer to the state transition object or `NULL` in case of
 *         insufficient memory error.
 */
static pbcc_ee_transition_t* _pbcc_receiving_stopped_state_transition_alloc(
    const pbcc_event_engine_t* ee,
    const pbcc_ee_state_t*     current_state,
    const pbcc_ee_event_t*     event);

/**
 * @brief Create transition details for Event Engine.
 *
 * @param ee                Pointer to the Event Engine which requested for
 *                          transition details.
 * @param target_state_type Type of target Subscribe Event Engine state.
 * @param context           Subscribe Event Engine data which should be passed
 *                          to the next state and transition invocations.
 * @param invocations       Pointer to the list of invocations which should be
 *                          called before Event Engine will enter new state.
 * @return Pointer to the ready to use transition details or `NULL` in case of
 *         insufficient memory error. The returned pointer must be passed to the
 *         `pbcc_ee_transition_free` to avoid a memory leak.
 */
static pbcc_ee_transition_t* _pbcc_transition_alloc(
    const pbcc_event_engine_t* ee,
    pbcc_subscribe_ee_state    target_state_type,
    pbcc_ee_data_t*            context,
    const pbarray_t*           invocations);


// ----------------------------------------------
//       Function prototypes: Effect
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
 * @return Handshake effect execution (scheduling) operation result.
 */
static enum pubnub_res _handshake_effect(
    pbcc_ee_invocation_t*                invocation,
    const pbcc_ee_data_t*                context,
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
 * @return Receive effect execution (scheduling) operation result.
 */
static enum pubnub_res _receive_effect(
    pbcc_ee_invocation_t*                invocation,
    const pbcc_ee_data_t*                context,
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
 * @return Status change emit operation result.
 */
static enum pubnub_res _emit_status_effect(
    pbcc_ee_invocation_t*                invocation,
    const pbcc_ee_data_t*                context,
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
 * @return Real-time updates emit operation result.
 */
static enum pubnub_res _emit_messages_effect(
    pbcc_ee_invocation_t*                invocation,
    const pbcc_ee_data_t*                context,
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
static void _cancel_effect(
    pbcc_ee_invocation_t*                invocation,
    const pbcc_ee_data_t*                context,
    pbcc_ee_effect_completion_function_t cb);

/**
 * @brief Make actual call to the Subscribe v2.
 *
 * @param invocation Pointer to the invocation which has been used to execute
 *                   effect.
 * @param context    Pointer to the Subscription context with all required
 *                   information to perform Subscribe v2 call.
 * @param cb         Pointer to the effect execution completion callback
 *                   function.
 * @return Subscribe v2 call operation result.
 */
static enum pubnub_res _make_subscribe_request(
    pbcc_ee_invocation_t*                invocation,
    const pbcc_subscribe_ee_context_t*   context,
    pbcc_ee_effect_completion_function_t cb);


// ----------------------------------------------
//                   Functions
// ----------------------------------------------

pbcc_subscribe_ee_t* pbcc_subscribe_ee_alloc(pubnub_t* pb)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    PBCC_ALLOCATE_TYPE(ee, pbcc_subscribe_ee_t, true, NULL);
    ee->subscription_sets = pbarray_alloc(
        SUBSCRIBABLE_LENGTH,
        PBARRAY_RESIZE_BALANCED,
        PBARRAY_GENERIC_CONTENT_TYPE,
        pubnub_subscription_set_free);
    ee->subscriptions = pbarray_alloc(
        SUBSCRIBABLE_LENGTH,
        PBARRAY_RESIZE_BALANCED,
        PBARRAY_GENERIC_CONTENT_TYPE,
        pubnub_subscription_free);

    pubnub_mutex_init(ee->mutw);
    pubnub_mutex_lock(pb->monitor);
    ee->last_result = pb->core.last_result;
    pubnub_mutex_unlock(pb->monitor);

    if (NULL == ee->subscriptions) {
        PUBNUB_LOG_ERROR("pbcc_subscribe_ee_alloc: failed to allocate memory "
            "for subscriptions list\n");
        pbcc_subscribe_ee_free(ee);
        return NULL;
    }
    if (NULL == ee->subscription_sets) {
        PUBNUB_LOG_ERROR("pbcc_subscribe_ee_alloc: failed to allocate memory "
            "for subscription sets list\n");
        pbcc_subscribe_ee_free(ee);
        return NULL;
    }

    // Prepare storage for computed list of subscribable objects.
    ee->subscribables = pbhash_set_alloc(
        SUBSCRIBABLE_LENGTH,
        PBHASH_SET_CHAR_CONTENT_TYPE,
        _pubnub_subscribable_free);
    if (NULL == ee->subscribables) {
        PUBNUB_LOG_ERROR("pbcc_subscribe_ee_alloc: failed to allocate memory "
            "for subscribable objects\n");
        pbcc_subscribe_ee_free(ee);
        return NULL;
    }
    ee->filter_expr = NULL;
    ee->heartbeat   = PUBNUB_DEFAULT_PRESENCE_HEARTBEAT_VALUE;
    ee->status      = SUBSCRIPTION_STATUS_DISCONNECTED;
    ee->pb          = pb;

    // Setup event engine
    ee->ee = pbcc_ee_alloc(_pbcc_unsubscribed_state_alloc());
    if (NULL == ee->ee) {
        PUBNUB_LOG_ERROR("pbcc_subscribe_ee_alloc: failed to allocate memory "
            "for event engine with initial state\n");
        pbcc_subscribe_ee_free(ee);
        return NULL;
    }

    // Setup event listener
    ee->event_listener = pbcc_event_listener_alloc(pb);
    if (NULL == ee->ee) {
        PUBNUB_LOG_ERROR("pbcc_subscribe_ee_alloc: failed to allocate memory "
            "for event listener\n");
        pbcc_subscribe_ee_free(ee);
        return NULL;
    }

    pubnub_register_callback(pb, _pbcc_subscribe_callback, ee);

    return ee;
}

pbcc_event_listener_t* pbcc_subscribe_ee_event_listener(
    const pbcc_subscribe_ee_t* ee)
{
    return ee->event_listener;
}

void pbcc_subscribe_ee_set_filter_expression(
    pbcc_subscribe_ee_t* ee,
    const char*          filter_expr)
{
    PUBNUB_ASSERT_OPT(NULL != ee);

    pubnub_mutex_lock(ee->mutw);
    if (NULL != ee->filter_expr) { free(ee->filter_expr); }
    if (NULL == filter_expr) { ee->filter_expr = NULL; }
    else { ee->filter_expr = pbstrdup(filter_expr); }
    pubnub_mutex_unlock(ee->mutw);
}

void pbcc_subscribe_ee_set_heartbeat(
    pbcc_subscribe_ee_t* ee,
    const unsigned       heartbeat)
{
    PUBNUB_ASSERT_OPT(NULL != ee);

    pubnub_mutex_lock(ee->mutw);
    ee->heartbeat = PUBNUB_MINIMUM_PRESENCE_HEARTBEAT_VALUE > heartbeat
                        ? PUBNUB_MINIMUM_PRESENCE_HEARTBEAT_VALUE
                        : heartbeat;
    pubnub_mutex_unlock(ee->mutw);
}

void pbcc_subscribe_ee_free(pbcc_subscribe_ee_t* ee)
{
    if (NULL == ee) { return; }

    pubnub_mutex_lock(ee->mutw);
    if (NULL != ee->subscription_sets) { pbarray_free(ee->subscription_sets); }
    if (NULL != ee->subscriptions) { pbarray_free(ee->subscriptions); }
    if (NULL != ee->subscribables) { pbhash_set_free(ee->subscribables); }
    if (NULL != ee->filter_expr) { free(ee->filter_expr); }
    if (NULL != ee->ee) { pbcc_ee_free(ee->ee); }
    pubnub_mutex_unlock(ee->mutw);
    pubnub_mutex_destroy(ee->mutw);
    free(ee);
}

enum pubnub_res pbcc_subscribe_ee_subscribe_with_subscription(
    pbcc_subscribe_ee_t*             ee,
    const pubnub_subscription_t*     sub,
    const pubnub_subscribe_cursor_t* cursor)
{
    PUBNUB_ASSERT_OPT(NULL != ee);

    pubnub_mutex_lock(ee->mutw);
    if (PBAR_OK != pbarray_add(ee->subscriptions, sub)) {
        PUBNUB_LOG_ERROR("pbcc_subscribe_ee_subscribe_with_subscription: failed"
            " to allocate memory to store subscription\n");
        pubnub_mutex_unlock(ee->mutw);
        return PNR_OUT_OF_MEMORY;
    }

    const enum pubnub_res rslt = _pbcc_subscribe_ee_subscribe(ee, cursor, true);
    pubnub_mutex_unlock(ee->mutw);

    return rslt;
}

enum pubnub_res pbcc_subscribe_ee_unsubscribe_with_subscription(
    pbcc_subscribe_ee_t*         ee,
    const pubnub_subscription_t* sub)
{
    PUBNUB_ASSERT_OPT(NULL != ee);

    pubnub_mutex_lock(ee->mutw);
    pbhash_set_t* subs = _pubnub_subscription_subscribables(sub, NULL);

    if (NULL == subs) {
        PUBNUB_LOG_ERROR("pbcc_subscribe_ee_unsubscribe_with_subscription: "
            "failed to allocate memory for subscribables\n");
        pubnub_mutex_unlock(ee->mutw);
        return PNR_OUT_OF_MEMORY;
    }

    pbarray_remove(ee->subscriptions, sub, true);
    const enum pubnub_res rslt = _pbcc_subscribe_ee_unsubscribe(ee, subs);
    // Subscribables list not needed anymore because channels / groups list
    // already composed for presence leave REST API in
    // `_pbcc_subscribe_ee_unsubscribe`.
    pbhash_set_free_with_destructor(subs, _pubnub_subscribable_free);
    pubnub_mutex_unlock(ee->mutw);

    return rslt;
}

enum pubnub_res pbcc_subscribe_ee_subscribe_with_subscription_set(
    pbcc_subscribe_ee_t*             ee,
    const pubnub_subscription_set_t* set,
    const pubnub_subscribe_cursor_t* cursor)
{
    PUBNUB_ASSERT_OPT(NULL != ee);

    pubnub_mutex_lock(ee->mutw);
    if (PBAR_OK != pbarray_add(ee->subscription_sets, set)) {
        PUBNUB_LOG_ERROR(
            "pbcc_subscribe_ee_subscribe_with_subscription_set: failed to "
            "allocate memory to store subscription set\n");
        pubnub_mutex_unlock(ee->mutw);
        return PNR_OUT_OF_MEMORY;
    }

    const enum pubnub_res rslt = _pbcc_subscribe_ee_subscribe(ee, cursor, true);
    pubnub_mutex_unlock(ee->mutw);

    return rslt;
}

enum pubnub_res pbcc_subscribe_ee_unsubscribe_with_subscription_set(
    pbcc_subscribe_ee_t*       ee,
    pubnub_subscription_set_t* set)
{
    PUBNUB_ASSERT_OPT(NULL != ee);

    pubnub_mutex_lock(ee->mutw);
    pbhash_set_t* subs = _pubnub_subscription_set_subscribables(set);

    if (NULL == subs) {
        PUBNUB_LOG_ERROR("pbcc_subscribe_ee_unsubscribe_with_subscription_set: "
            "failed to allocate memory for subscribables\n");
        pubnub_mutex_unlock(ee->mutw);
        return PNR_OUT_OF_MEMORY;
    }

    pbarray_remove(ee->subscription_sets, set, true);
    const enum pubnub_res rslt = _pbcc_subscribe_ee_unsubscribe(ee, subs);
    // Subscribables list not needed anymore because channels / groups list
    // already composed for presence leave REST API in
    // `_pbcc_subscribe_ee_unsubscribe`.
    pbhash_set_free_with_destructor(subs, _pubnub_subscribable_free);
    pubnub_mutex_unlock(ee->mutw);

    return rslt;
}

enum pubnub_res pbcc_subscribe_ee_change_subscription_with_subscription_set(
    pbcc_subscribe_ee_t*             ee,
    const pubnub_subscription_set_t* set,
    const pubnub_subscription_t*     sub,
    const bool                       added)
{
    PUBNUB_ASSERT_OPT(NULL != ee);

    pubnub_mutex_lock(ee->mutw);
    enum pubnub_res rslt;

    if (added) { rslt = _pbcc_subscribe_ee_subscribe(ee, NULL, true); }
    else {
        const pubnub_subscription_options_t options =
            *(pubnub_subscription_options_t*)set;
        pbhash_set_t* subs = _pubnub_subscription_subscribables(sub, &options);

        if (NULL == subs) {
            PUBNUB_LOG_ERROR(
                "pbcc_subscribe_ee_change_subscription_with_subscription_set: "
                "failed to allocate memory for subscribables\n");
            pubnub_mutex_unlock(ee->mutw);
            return PNR_OUT_OF_MEMORY;
        }

        rslt = _pbcc_subscribe_ee_unsubscribe(ee, subs);
        // Subscribables list not needed anymore because channels / groups list
        // already composed for presence leave REST API in
        // `_pbcc_subscribe_ee_unsubscribe`.
        pbhash_set_free_with_destructor(subs, _pubnub_subscribable_free);
    }
    pubnub_mutex_unlock(ee->mutw);

    return rslt;
}

enum pubnub_res pbcc_subscribe_ee_disconnect(pbcc_subscribe_ee_t* ee)
{
    PUBNUB_ASSERT_OPT(NULL != ee);

    pubnub_mutex_lock(ee->mutw);
    const pbcc_ee_event_t* event = _pbcc_disconnect_event_alloc(ee);
    if (NULL == event) {
        PUBNUB_LOG_ERROR("pbcc_subscribe_ee_disconnect: failed to allocate "
            "memory for event\n");
        pubnub_mutex_unlock(ee->mutw);
        return PNR_OUT_OF_MEMORY;
    }
    pubnub_mutex_unlock(ee->mutw);

    return pbcc_ee_handle_event(ee->ee, event);
}

enum pubnub_res pbcc_subscribe_ee_reconnect(
    pbcc_subscribe_ee_t*            ee,
    const pubnub_subscribe_cursor_t cursor)
{
    PUBNUB_ASSERT_OPT(NULL != ee);

    pubnub_mutex_lock(ee->mutw);
    const pbcc_ee_event_t* event = _pbcc_reconnect_event_alloc(ee, cursor);
    if (NULL == event) {
        PUBNUB_LOG_ERROR("pbcc_subscribe_ee_reconnect: failed to allocate "
            "memory for event\n");
        pubnub_mutex_unlock(ee->mutw);
        return PNR_OUT_OF_MEMORY;
    }
    pubnub_mutex_unlock(ee->mutw);

    return pbcc_ee_handle_event(ee->ee, event);
}

enum pubnub_res pbcc_subscribe_ee_unsubscribe_all(pbcc_subscribe_ee_t* ee)
{
    PUBNUB_ASSERT_OPT(NULL != ee);

    pubnub_mutex_lock(ee->mutw);
    char*           ch,* cg;
    enum pubnub_res rslt = _pbcc_subscribe_ee_subscribables(ee->subscribables,
        &ch,
        &cg,
        false);

    if (PNR_OK == rslt) {
        const pbcc_ee_event_t* event =
            _pbcc_unsubscribe_all_event_alloc(ee, &ch, &cg);
        if (NULL == event) {
            PUBNUB_LOG_ERROR("pbcc_subscribe_ee_reconnect: failed to allocate "
                "memory for event\n");
            rslt = PNR_OUT_OF_MEMORY;
        }
        else {
            pbarray_remove_all(ee->subscription_sets);
            pbarray_remove_all(ee->subscriptions);

            rslt = pbcc_ee_handle_event(ee->ee, event);
        }
    }

    if (PNR_OUT_OF_MEMORY == rslt) {
        if (NULL != ch) { free(ch); }
        if (NULL != cg) { free(cg); }
    }
    pubnub_mutex_unlock(ee->mutw);

    // Looks like there is nothing to unsubscribe from.
    if (PNR_INVALID_PARAMETERS == rslt) { rslt = PNR_OK; }

    return rslt;
}

pubnub_subscription_t** pbcc_subscribe_ee_subscriptions(
    pbcc_subscribe_ee_t* ee,
    size_t*              count)
{
    PUBNUB_ASSERT_OPT(NULL != ee);

    pubnub_mutex_lock(ee->mutw);
    pubnub_subscription_t** subs = (pubnub_subscription_t**)
        pbarray_elements(ee->subscriptions, count);
    pubnub_mutex_unlock(ee->mutw);

    return subs;
}

pubnub_subscription_set_t** pbcc_subscribe_ee_subscription_sets(
    pbcc_subscribe_ee_t* ee,
    size_t*              count)
{
    PUBNUB_ASSERT_OPT(NULL != ee);

    pubnub_mutex_lock(ee->mutw);
    pubnub_subscription_set_t** subs = (pubnub_subscription_set_t**)
        pbarray_elements(ee->subscription_sets, count);
    pubnub_mutex_unlock(ee->mutw);

    return subs;
}


// ----------------------------------------------
//            Subscription handler
// ----------------------------------------------

void _pbcc_subscribe_callback(
    pubnub_t*               pb,
    const enum pubnub_trans trans,
    const enum pubnub_res   result,
    const void*             user_data
    )
{
    const pbcc_subscribe_ee_t* ee = user_data;

    // Asynchronously cancelled subscription and presence leave shouldn't
    // trigger any events.
    if ((PBTT_SUBSCRIBE_V2 == trans && PNR_CANCELLED == result) ||
        PBTT_LEAVE == trans) {
        // Asynchronous cancellation mean that
        pbcc_ee_process_next_invocation(ee->ee, PBTT_LEAVE != trans);
        return;
    }

    PUBNUB_ASSERT_OPT(trans == PBTT_SUBSCRIBE_V2);
    const bool                error        = PNR_OK != result;
    const pbcc_ee_state_t*    state_object = pbcc_ee_current_state(ee->ee);
    const pbcc_ee_event_t*    event        = NULL;
    pubnub_subscribe_cursor_t cursor;
    cursor.region = 0;

    if (!error) {
        // Retrieve parsed timetoken stored for next subscription loop.
        pubnub_mutex_lock(pb->monitor);
        size_t token_len = strlen(pb->core.timetoken);
        memcpy(cursor.timetoken, pb->core.timetoken, token_len);
        cursor.timetoken[token_len] = '\0';
        if (pb->core.region > 0) { cursor.region = pb->core.region; }
        pbpal_mutex_unlock(pb->monitor);
    }

    if (SUBSCRIBE_EE_STATE_HANDSHAKING == state_object->type) {
        if (!error) { event = _pbcc_handshake_success_event_alloc(ee, cursor); }
        else { event = _pbcc_handshake_failure_event_alloc(ee, result); }
    }
    else if (SUBSCRIBE_EE_STATE_RECEIVING == state_object->type) {
        if (!error) { event = _pbcc_receive_success_event_alloc(ee, cursor); }
        else { event = _pbcc_receive_failure_event_alloc(ee, result); }
    }

    if (NULL != event) { pbcc_ee_handle_event(ee->ee, event); }
}


// ----------------------------------------------
//               Functions: State
// ----------------------------------------------

pbcc_ee_state_t* _pbcc_unsubscribed_state_alloc()
{
    pbcc_ee_state_t* state = pbcc_ee_state_alloc(
        SUBSCRIBE_EE_STATE_UNSUBSCRIBED,
        NULL);
    if (NULL == state) { return NULL; }

    state->transition = _pbcc_unsubscribed_state_transition_alloc;

    return state;
}

pbcc_ee_state_t* _pbcc_handshaking_state_alloc(pbcc_ee_data_t* context)
{
    pbcc_ee_state_t* state = pbcc_ee_state_alloc(
        SUBSCRIBE_EE_STATE_HANDSHAKING,
        context);
    if (NULL == state) { return NULL; }

    state->transition              = _pbcc_handshaking_state_transition_alloc;
    pbcc_ee_invocation_t* on_enter =
        pbcc_ee_invocation_alloc(_handshake_effect, context, false);

    if (NULL == on_enter ||
        PNR_OK != pbcc_ee_state_add_on_enter_invocation(state, on_enter)) {
        if (NULL != on_enter) { pbcc_ee_invocation_free(on_enter); }
        pbcc_ee_state_free(state);
        return NULL;
    }

    pbcc_ee_invocation_t* on_exit =
        pbcc_ee_invocation_alloc(_cancel_effect, context, true);
    if (NULL == on_exit ||
        PNR_OK != pbcc_ee_state_add_on_exit_invocation(state, on_exit)) {
        if (NULL != on_exit) { pbcc_ee_invocation_free(on_exit); }
        pbcc_ee_state_free(state);
        return NULL;
    }

    return state;
}

pbcc_ee_state_t* _pbcc_handshake_failed_state_alloc(pbcc_ee_data_t* context)
{
    pbcc_ee_state_t* state = pbcc_ee_state_alloc(
        SUBSCRIBE_EE_STATE_HANDSHAKE_FAILED,
        context);
    if (NULL == state) { return NULL; }

    state->transition = _pbcc_handshake_failed_state_transition_alloc;

    return state;
}

pbcc_ee_state_t* _pbcc_handshake_stopped_state_alloc(pbcc_ee_data_t* context)
{
    pbcc_ee_state_t* state = pbcc_ee_state_alloc(
        SUBSCRIBE_EE_STATE_HANDSHAKE_STOPPED,
        context);
    if (NULL == state) { return NULL; }

    state->transition = _pbcc_handshake_stopped_state_transition_alloc;

    return state;
}

pbcc_ee_state_t* _pbcc_receiving_state_alloc(pbcc_ee_data_t* context)
{
    pbcc_ee_state_t* state = pbcc_ee_state_alloc(
        SUBSCRIBE_EE_STATE_RECEIVING,
        context);
    if (NULL == state) { return NULL; }

    state->transition              = _pbcc_receiving_state_transition_alloc;
    pbcc_ee_invocation_t* on_enter =
        pbcc_ee_invocation_alloc(_receive_effect, context, false);

    if (NULL == on_enter ||
        PNR_OK != pbcc_ee_state_add_on_enter_invocation(state, on_enter)) {
        if (NULL != on_enter) { pbcc_ee_invocation_free(on_enter); }
        pbcc_ee_state_free(state);
        return NULL;
    }

    pbcc_ee_invocation_t* on_exit =
        pbcc_ee_invocation_alloc(_cancel_effect, context, true);
    if (NULL == on_exit ||
        PNR_OK != pbcc_ee_state_add_on_exit_invocation(state, on_exit)) {
        if (NULL != on_exit) { pbcc_ee_invocation_free(on_exit); }
        pbcc_ee_state_free(state);
        return NULL;
    }

    return state;
}

pbcc_ee_state_t* _pbcc_receive_failed_state_alloc(pbcc_ee_data_t* context)
{
    pbcc_ee_state_t* state = pbcc_ee_state_alloc(
        SUBSCRIBE_EE_STATE_RECEIVE_FAILED,
        context);
    if (NULL == state) { return NULL; }

    state->transition = _pbcc_receiving_failed_state_transition_alloc;

    return state;
}

pbcc_ee_state_t* _pbcc_receive_stopped_state_alloc(pbcc_ee_data_t* context)
{
    pbcc_ee_state_t* state = pbcc_ee_state_alloc(
        SUBSCRIBE_EE_STATE_RECEIVE_STOPPED,
        context);
    if (NULL == state) { return NULL; }

    state->transition = _pbcc_receiving_stopped_state_transition_alloc;

    return state;
}


// ----------------------------------------------
//               Functions: Event
// ----------------------------------------------

pbcc_ee_event_t* _pbcc_subscription_changed_event_alloc(
    const pbcc_subscribe_ee_t* ee,
    char**                     channels,
    char**                     channel_groups)
{
    if (NULL != channels && 0 == strlen(*channels)) {
        free(*channels);
        *channels = NULL;
    }
    if (NULL != channel_groups && 0 == strlen(*channel_groups)) {
        free(*channel_groups);
        *channel_groups = NULL;
    }

    const pbcc_subscribe_ee_event type =
        SUBSCRIBE_EE_EVENT_SUBSCRIPTION_CHANGED;
    return _pbcc_subscribe_ee_event_alloc(ee,
                                          type,
                                          channels,
                                          channel_groups,
                                          NULL,
                                          NULL);
}

pbcc_ee_event_t* _pbcc_subscription_restored_event_alloc(
    const pbcc_subscribe_ee_t*      ee,
    char**                          channels,
    char**                          channel_groups,
    const pubnub_subscribe_cursor_t cursor)
{
    if (NULL != channels && 0 == strlen(*channels)) {
        free(*channels);
        *channels = NULL;
    }
    if (NULL != channel_groups && 0 == strlen(*channel_groups)) {
        free(*channel_groups);
        *channel_groups = NULL;
    }

    const pbcc_subscribe_ee_event type =
        SUBSCRIBE_EE_EVENT_SUBSCRIPTION_RESTORED;
    return _pbcc_subscribe_ee_event_alloc(ee,
                                          type,
                                          channels,
                                          channel_groups,
                                          &cursor,
                                          NULL);
}

pbcc_ee_event_t* _pbcc_handshake_success_event_alloc(
    const pbcc_subscribe_ee_t*      ee,
    const pubnub_subscribe_cursor_t cursor)
{
    const pbcc_subscribe_ee_event type = SUBSCRIBE_EE_EVENT_HANDSHAKE_SUCCESS;
    return _pbcc_subscribe_ee_event_alloc(ee, type, NULL, NULL, &cursor, NULL);
}

pbcc_ee_event_t* _pbcc_handshake_failure_event_alloc(
    const pbcc_subscribe_ee_t* ee,
    const enum pubnub_res      reason)
{
    const pbcc_subscribe_ee_event type = SUBSCRIBE_EE_EVENT_HANDSHAKE_FAILURE;
    return _pbcc_subscribe_ee_event_alloc(ee, type, NULL, NULL, NULL, &reason);
}

pbcc_ee_event_t* _pbcc_receive_success_event_alloc(
    const pbcc_subscribe_ee_t*      ee,
    const pubnub_subscribe_cursor_t cursor)
{
    const pbcc_subscribe_ee_event type = SUBSCRIBE_EE_EVENT_RECEIVE_SUCCESS;
    return _pbcc_subscribe_ee_event_alloc(ee, type, NULL, NULL, &cursor, NULL);
}

pbcc_ee_event_t* _pbcc_receive_failure_event_alloc(
    const pbcc_subscribe_ee_t* ee,
    const enum pubnub_res      reason)
{
    const pbcc_subscribe_ee_event type = SUBSCRIBE_EE_EVENT_RECEIVE_FAILURE;
    return _pbcc_subscribe_ee_event_alloc(ee, type, NULL, NULL, NULL, &reason);
}

pbcc_ee_event_t* _pbcc_disconnect_event_alloc(
    const pbcc_subscribe_ee_t* ee)
{
    const pbcc_subscribe_ee_event type = SUBSCRIBE_EE_EVENT_DISCONNECT;
    return _pbcc_subscribe_ee_event_alloc(ee, type, NULL, NULL, NULL, NULL);
}

pbcc_ee_event_t* _pbcc_reconnect_event_alloc(
    const pbcc_subscribe_ee_t*      ee,
    const pubnub_subscribe_cursor_t cursor)
{
    const pbcc_subscribe_ee_event type = SUBSCRIBE_EE_EVENT_RECONNECT;
    return _pbcc_subscribe_ee_event_alloc(ee, type, NULL, NULL, &cursor, NULL);
}

pbcc_ee_event_t* _pbcc_unsubscribe_all_event_alloc(
    const pbcc_subscribe_ee_t* ee,
    char**                     channels,
    char**                     channel_groups)
{
    if (NULL != channels && 0 == strlen(*channels)) {
        free(*channels);
        *channels = NULL;
    }
    if (NULL != channel_groups && 0 == strlen(*channel_groups)) {
        free(*channel_groups);
        *channel_groups = NULL;
    }

    const pbcc_subscribe_ee_event type = SUBSCRIBE_EE_EVENT_UNSUBSCRIBE_ALL;
    return _pbcc_subscribe_ee_event_alloc(ee,
                                          type,
                                          channels,
                                          channel_groups,
                                          NULL,
                                          NULL);
}

pbcc_ee_event_t* _pbcc_subscribe_ee_event_alloc(
    const pbcc_subscribe_ee_t*       ee,
    const pbcc_subscribe_ee_event    event,
    char**                           channels,
    char**                           channel_groups,
    const pubnub_subscribe_cursor_t* cursor,
    const enum pubnub_res*           reason)
{
    pubnub_mutex_lock(ee->mutw);
    const pbcc_subscribe_ee_context_t* current_context =
        _pbcc_subscribe_ee_current_state_context(ee);
    pbcc_subscribe_ee_context_t* ctx = _pbcc_subscribe_ee_context_copy(
        ee->pb,
        current_context,
        channels,
        channel_groups);

    if (NULL == ctx) {
        pubnub_mutex_unlock(ee->mutw);
        return NULL;
    }
    if (NULL != cursor) { ctx->cursor = *cursor; }
    if (NULL != reason) { ctx->reason = *reason; }

    pbcc_ee_data_t* data =
        pbcc_ee_data_alloc(ctx, _pbcc_subscribe_ee_context_free);
    if (NULL == data) {
        _pbcc_subscribe_ee_context_free(ctx);
        pubnub_mutex_unlock(ee->mutw);
        return NULL;
    }
    pubnub_mutex_unlock(ee->mutw);

    return pbcc_ee_event_alloc(event, data);
}


// ----------------------------------------------
//             Functions: Transition
// ----------------------------------------------

pbcc_ee_transition_t* _pbcc_unsubscribed_state_transition_alloc(
    const pbcc_event_engine_t* ee,
    const pbcc_ee_state_t*     current_state,
    const pbcc_ee_event_t*     event)
{
    PUBNUB_ASSERT_OPT(current_state->type == SUBSCRIBE_EE_STATE_UNSUBSCRIBED);

    if (event->type != SUBSCRIBE_EE_EVENT_SUBSCRIPTION_CHANGED &&
        event->type != SUBSCRIBE_EE_EVENT_SUBSCRIPTION_RESTORED) {
        PUBNUB_LOG_INFO("pbcc_unsubscribed_state_transition_alloc: can't "
                        "handle transition for %d event type\n",
                        event->type);
        return _pbcc_transition_alloc(ee, SUBSCRIBE_EE_STATE_NONE, NULL, NULL);
    }

    return _pbcc_transition_alloc(ee,
                                  SUBSCRIBE_EE_STATE_HANDSHAKING,
                                  event->data,
                                  NULL);
}

pbcc_ee_transition_t* _pbcc_handshaking_state_transition_alloc(
    const pbcc_event_engine_t* ee,
    const pbcc_ee_state_t*     current_state,
    const pbcc_ee_event_t*     event)
{
    PUBNUB_ASSERT_OPT(current_state->type == SUBSCRIBE_EE_STATE_HANDSHAKING);

    pbcc_subscribe_ee_state target_state_type = SUBSCRIBE_EE_STATE_NONE;
    pbcc_ee_data_t*         data              = event->data;
    pbarray_t*              invocations       = NULL;

    switch (event->type) {
    case SUBSCRIBE_EE_EVENT_SUBSCRIPTION_CHANGED: {
        PUBNUB_ASSERT_OPT(NULL != data);

        const pbcc_subscribe_ee_context_t* context = pbcc_ee_data_value(data);
        const char*                        channel_groups =
            pbcc_ee_data_value(context->channel_groups);
        const char* channels = pbcc_ee_data_value(context->channels);

        if (NULL != context && 0 == strlen(channels) &&
            0 == strlen(channel_groups)) {
            target_state_type = SUBSCRIBE_EE_STATE_UNSUBSCRIBED;
            data              = NULL;
        }
        else { target_state_type = SUBSCRIBE_EE_STATE_HANDSHAKING; }
    }
    break;
    case SUBSCRIBE_EE_EVENT_SUBSCRIPTION_RESTORED:
        target_state_type = SUBSCRIBE_EE_STATE_HANDSHAKING;
        break;
    case SUBSCRIBE_EE_EVENT_HANDSHAKE_FAILURE:
    case SUBSCRIBE_EE_EVENT_HANDSHAKE_SUCCESS: {
        const pbcc_subscribe_ee_context_t* context = pbcc_ee_data_value(data);
        pbcc_subscribe_ee_t* subscribe_ee = context->pb->core.subscribe_ee;
        target_state_type = event->type == SUBSCRIBE_EE_EVENT_HANDSHAKE_SUCCESS
                                ? SUBSCRIBE_EE_STATE_RECEIVING
                                : SUBSCRIBE_EE_STATE_HANDSHAKE_FAILED;
        invocations = pbarray_alloc(1,
                                    PBARRAY_RESIZE_NONE,
                                    PBARRAY_GENERIC_CONTENT_TYPE,
                                    pbcc_ee_invocation_free);
        pbcc_ee_invocation_t* invocation =
            pbcc_ee_invocation_alloc(_emit_status_effect, data, false);

        if (NULL == invocations || NULL == invocation) {
            PUBNUB_LOG_ERROR("pbcc_handshaking_state_transition_alloc: failed "
                "to allocate memory for invocations\n");
            if (NULL != invocations) { pbarray_free(invocations); }
            if (NULL != invocation) { pbcc_ee_invocation_free(invocation); }
            return NULL;
        }

        if (PBAR_OK != pbarray_add(invocations, invocation)) {
            PUBNUB_LOG_ERROR("pbcc_handshaking_state_transition_alloc: failed "
                "to allocate memory for invocation entry\n");
            pbcc_ee_invocation_free(invocation);
            pbarray_free(invocations);
            return NULL;
        }

        // Update latest Subscribe Event Engine subscription status.
        pubnub_mutex_lock(subscribe_ee->mutw);
        subscribe_ee->status = SUBSCRIBE_EE_STATE_RECEIVING
                                   ? SUBSCRIPTION_STATUS_CONNECTED
                                   : SUBSCRIPTION_STATUS_CONNECTION_ERROR;
        pubnub_mutex_unlock(subscribe_ee->mutw);
    }
    break;
    case SUBSCRIBE_EE_EVENT_DISCONNECT:
        target_state_type = SUBSCRIBE_EE_STATE_HANDSHAKE_STOPPED;
        break;
    case SUBSCRIBE_EE_EVENT_UNSUBSCRIBE_ALL:
        target_state_type = SUBSCRIBE_EE_STATE_UNSUBSCRIBED;
        break;
    default:
        invocations = NULL;
        data = NULL;
        break;
    }

    if (SUBSCRIBE_EE_STATE_NONE == target_state_type) {
        PUBNUB_LOG_INFO("pbcc_handshaking_state_transition_alloc: can't handle "
                        "transition for %d event type\n",
                        event->type);
    }

    return _pbcc_transition_alloc(ee, target_state_type, data, invocations);
}

pbcc_ee_transition_t* _pbcc_handshake_failed_state_transition_alloc(
    const pbcc_event_engine_t* ee,
    const pbcc_ee_state_t*     current_state,
    const pbcc_ee_event_t*     event)
{
    PUBNUB_ASSERT_OPT(
        current_state->type == SUBSCRIBE_EE_STATE_HANDSHAKE_FAILED);

    pbcc_subscribe_ee_state target_state_type = SUBSCRIBE_EE_STATE_NONE;
    pbcc_ee_data_t*         data              = event->data;

    switch (event->type) {
    case SUBSCRIBE_EE_EVENT_SUBSCRIPTION_CHANGED:
        PUBNUB_ASSERT_OPT(NULL != data);

        const pbcc_subscribe_ee_context_t* context = pbcc_ee_data_value(data);
        const char*                        channel_groups =
            pbcc_ee_data_value(context->channel_groups);
        const char* channels = pbcc_ee_data_value(context->channels);

        if (NULL != context && 0 == strlen(channels) &&
            0 == strlen(channel_groups)) {
            target_state_type = SUBSCRIBE_EE_STATE_UNSUBSCRIBED;
            data              = NULL;
        }
        else { target_state_type = SUBSCRIBE_EE_STATE_HANDSHAKING; }
        break;
    case SUBSCRIBE_EE_EVENT_SUBSCRIPTION_RESTORED:
    case SUBSCRIBE_EE_EVENT_RECONNECT:
        target_state_type = SUBSCRIBE_EE_STATE_HANDSHAKING;
        break;
    case SUBSCRIBE_EE_EVENT_UNSUBSCRIBE_ALL:
        target_state_type = SUBSCRIBE_EE_STATE_UNSUBSCRIBED;
        break;
    default:
        data = NULL;
        break;
    }

    if (SUBSCRIBE_EE_STATE_NONE == target_state_type) {
        PUBNUB_LOG_INFO("pbcc_handshake_failed_state_transition_alloc: can't "
                        "handle transition for %d event type\n",
                        event->type);
    }

    return _pbcc_transition_alloc(ee, target_state_type, data, NULL);
}

pbcc_ee_transition_t* _pbcc_handshake_stopped_state_transition_alloc(
    const pbcc_event_engine_t* ee,
    const pbcc_ee_state_t*     current_state,
    const pbcc_ee_event_t*     event)
{
    PUBNUB_ASSERT_OPT(
        current_state->type == SUBSCRIBE_EE_STATE_HANDSHAKE_STOPPED);

    pbcc_subscribe_ee_state target_state_type = SUBSCRIBE_EE_STATE_NONE;

    switch (event->type) {
    case SUBSCRIBE_EE_EVENT_SUBSCRIPTION_CHANGED:
    case SUBSCRIBE_EE_EVENT_SUBSCRIPTION_RESTORED:
        target_state_type = SUBSCRIBE_EE_STATE_HANDSHAKE_STOPPED;
        break;
    case SUBSCRIBE_EE_EVENT_RECONNECT:
        target_state_type = SUBSCRIBE_EE_STATE_HANDSHAKING;
        break;
    case SUBSCRIBE_EE_EVENT_UNSUBSCRIBE_ALL:
        target_state_type = SUBSCRIBE_EE_STATE_UNSUBSCRIBED;
        break;
    default:
        break;
    }

    if (SUBSCRIBE_EE_STATE_NONE == target_state_type) {
        PUBNUB_LOG_INFO("pbcc_handshake_stopped_state_transition_alloc: can't "
                        "handle transition for %d event type\n",
                        event->type);
    }

    return _pbcc_transition_alloc(ee, target_state_type, event->data, NULL);
}

pbcc_ee_transition_t* _pbcc_receiving_state_transition_alloc(
    const pbcc_event_engine_t* ee,
    const pbcc_ee_state_t*     current_state,
    const pbcc_ee_event_t*     event)
{
    PUBNUB_ASSERT_OPT(current_state->type == SUBSCRIBE_EE_STATE_RECEIVING);

    pbcc_subscribe_ee_state    target_state_type = SUBSCRIBE_EE_STATE_NONE;
    pubnub_subscription_status status            = -1;
    pbarray_t*                 invocations       = NULL;
    pbcc_ee_invocation_t*      invocation        = NULL;
    pbcc_ee_data_t*            data              = event->data;

    switch (event->type) {
    case SUBSCRIBE_EE_EVENT_SUBSCRIPTION_CHANGED:
    case SUBSCRIBE_EE_EVENT_SUBSCRIPTION_RESTORED:
        target_state_type = SUBSCRIBE_EE_STATE_RECEIVING;
        status = SUBSCRIPTION_STATUS_SUBSCRIPTION_CHANGED;
        break;
    case SUBSCRIBE_EE_EVENT_RECEIVE_SUCCESS:
        target_state_type = SUBSCRIBE_EE_STATE_RECEIVING;
        break;
    case SUBSCRIBE_EE_EVENT_RECEIVE_FAILURE:
        target_state_type = SUBSCRIBE_EE_STATE_RECEIVE_FAILED;
        status = SUBSCRIPTION_STATUS_DISCONNECTED_UNEXPECTEDLY;
        break;
    case SUBSCRIBE_EE_EVENT_DISCONNECT:
        target_state_type = SUBSCRIBE_EE_STATE_RECEIVE_STOPPED;
        status = SUBSCRIPTION_STATUS_DISCONNECTED;
        break;
    case SUBSCRIBE_EE_EVENT_UNSUBSCRIBE_ALL:
        target_state_type = SUBSCRIBE_EE_STATE_UNSUBSCRIBED;
        status = SUBSCRIPTION_STATUS_DISCONNECTED;
        break;
    default:
        data = NULL;
        break;
    }

    if (SUBSCRIBE_EE_EVENT_RECEIVE_SUCCESS == target_state_type ||
        -1 != status) {
        invocations = pbarray_alloc(1,
                                    PBARRAY_RESIZE_NONE,
                                    PBARRAY_GENERIC_CONTENT_TYPE,
                                    pbcc_ee_invocation_free);

        if (NULL == invocations) {
            PUBNUB_LOG_ERROR("pbcc_receiving_state_transition_alloc: failed "
                "to allocate memory for invocations\n");
            return NULL;
        }
    }

    if (-1 != status) {
        const pbcc_subscribe_ee_context_t* context = pbcc_ee_data_value(data);
        pbcc_subscribe_ee_t* subscribe_ee = context->pb->core.subscribe_ee;
        invocation =
            pbcc_ee_invocation_alloc(_emit_status_effect, data, false);

        // Update latest Subscribe Event Engine subscription status.
        pubnub_mutex_lock(subscribe_ee->mutw);
        subscribe_ee->status = status;
        pubnub_mutex_unlock(subscribe_ee->mutw);
    }

    if (SUBSCRIBE_EE_EVENT_RECEIVE_SUCCESS == target_state_type) {
        invocation = pbcc_ee_invocation_alloc(_emit_messages_effect,
                                              data,
                                              false);
    }

    if (NULL != invocation) {
        if (PBAR_OK != pbarray_add(invocations, invocation)) {
            PUBNUB_LOG_ERROR("pbcc_receiving_state_transition_alloc: failed "
                "to allocate memory for invocation entry\n");
            pbcc_ee_invocation_free(invocation);
            pbarray_free(invocations);
            return NULL;
        }
    }
    else if (SUBSCRIBE_EE_EVENT_RECEIVE_SUCCESS == target_state_type ||
             -1 != status) {
        PUBNUB_LOG_ERROR("pbcc_receiving_state_transition_alloc: failed "
            "to allocate memory for invocation\n");
        if (NULL != invocations) { pbarray_free(invocations); }
        return NULL;
    }

    if (SUBSCRIBE_EE_STATE_NONE == target_state_type) {
        PUBNUB_LOG_INFO("pbcc_receiving_state_transition_alloc: can't handle "
                        "transition for %d event type\n",
                        event->type);
    }

    return _pbcc_transition_alloc(ee,
                                  target_state_type,
                                  event->data,
                                  invocations);
}

pbcc_ee_transition_t* _pbcc_receiving_failed_state_transition_alloc(
    const pbcc_event_engine_t* ee,
    const pbcc_ee_state_t*     current_state,
    const pbcc_ee_event_t*     event)
{
    PUBNUB_ASSERT_OPT(current_state->type == SUBSCRIBE_EE_STATE_RECEIVE_FAILED);

    pbcc_subscribe_ee_state target_state_type = SUBSCRIBE_EE_STATE_NONE;

    switch (event->type) {
    case SUBSCRIBE_EE_EVENT_SUBSCRIPTION_CHANGED:
    case SUBSCRIBE_EE_EVENT_SUBSCRIPTION_RESTORED:
    case SUBSCRIBE_EE_EVENT_RECONNECT:
        target_state_type = SUBSCRIBE_EE_STATE_HANDSHAKING;
        break;
    case SUBSCRIBE_EE_EVENT_UNSUBSCRIBE_ALL:
        target_state_type = SUBSCRIBE_EE_STATE_UNSUBSCRIBED;
        break;
    default:
        break;
    }

    if (SUBSCRIBE_EE_STATE_NONE == target_state_type) {
        PUBNUB_LOG_INFO("pbcc_receiving_failed_state_transition_alloc: can't "
                        "handle transition for %d event type\n",
                        event->type);
    }

    return _pbcc_transition_alloc(ee, target_state_type, event->data, NULL);
}

pbcc_ee_transition_t* _pbcc_receiving_stopped_state_transition_alloc(
    const pbcc_event_engine_t* ee,
    const pbcc_ee_state_t*     current_state,
    const pbcc_ee_event_t*     event)
{
    PUBNUB_ASSERT_OPT(current_state->type ==
        SUBSCRIBE_EE_STATE_RECEIVE_STOPPED);

    pbcc_subscribe_ee_state target_state_type = SUBSCRIBE_EE_STATE_NONE;

    switch (event->type) {
    case SUBSCRIBE_EE_EVENT_SUBSCRIPTION_CHANGED:
    case SUBSCRIBE_EE_EVENT_SUBSCRIPTION_RESTORED:
        target_state_type = SUBSCRIBE_EE_STATE_RECEIVE_STOPPED;
        break;
    case SUBSCRIBE_EE_EVENT_RECONNECT:
        target_state_type = SUBSCRIBE_EE_STATE_HANDSHAKING;
        break;
    case SUBSCRIBE_EE_EVENT_UNSUBSCRIBE_ALL:
        target_state_type = SUBSCRIBE_EE_STATE_UNSUBSCRIBED;
        break;
    default:
        break;
    }

    if (SUBSCRIBE_EE_STATE_NONE == target_state_type) {
        PUBNUB_LOG_INFO("pbcc_receiving_stopped_state_transition_alloc: can't "
                        "handle transition for %d event type\n",
                        event->type);
    }

    return _pbcc_transition_alloc(ee, target_state_type, event->data, NULL);
}

pbcc_ee_transition_t* _pbcc_transition_alloc(
    const pbcc_event_engine_t*    ee,
    const pbcc_subscribe_ee_state target_state_type,
    pbcc_ee_data_t*               context,
    const pbarray_t*              invocations)
{
    const pbcc_ee_state_t* target_state  = NULL;
    pbcc_ee_data_t*        state_context = NULL;

    if (NULL != context && SUBSCRIBE_EE_STATE_NONE != target_state_type &&
        SUBSCRIBE_EE_STATE_UNSUBSCRIBED != target_state_type) {
        state_context = context;
    }

    switch (target_state_type) {
    case SUBSCRIBE_EE_STATE_UNSUBSCRIBED:
        target_state = _pbcc_unsubscribed_state_alloc();
        break;
    case SUBSCRIBE_EE_STATE_HANDSHAKING:
        target_state = _pbcc_handshaking_state_alloc(state_context);
        break;
    case SUBSCRIBE_EE_STATE_HANDSHAKE_FAILED:
        target_state = _pbcc_handshake_failed_state_alloc(state_context);
        break;
    case SUBSCRIBE_EE_STATE_HANDSHAKE_STOPPED:
        target_state = _pbcc_handshake_stopped_state_alloc(state_context);
        break;
    case SUBSCRIBE_EE_STATE_RECEIVING:
        target_state = _pbcc_receiving_state_alloc(state_context);
        break;
    case SUBSCRIBE_EE_STATE_RECEIVE_FAILED:
        target_state = _pbcc_receive_failed_state_alloc(state_context);
        break;
    case SUBSCRIBE_EE_STATE_RECEIVE_STOPPED:
        target_state = _pbcc_receive_stopped_state_alloc(state_context);
        break;
    default:
        PUBNUB_LOG_ERROR("pbcc_transition_alloc: unknown target state type\n");
        return NULL;
    }

    if (NULL == target_state && SUBSCRIBE_EE_STATE_NONE != target_state_type) {
        PUBNUB_LOG_ERROR("pbcc_transition_alloc: failed to allocate memory for "
            "state object\n");
        return NULL;
    }

    return pbcc_ee_transition_alloc(ee, target_state, invocations);
}


// ----------------------------------------------
//              Functions: Effect
// ----------------------------------------------

enum pubnub_res _handshake_effect(
    pbcc_ee_invocation_t*                      invocation,
    const pbcc_ee_data_t*                      context,
    const pbcc_ee_effect_completion_function_t cb)
{
    return _make_subscribe_request(invocation, pbcc_ee_data_value(context), cb);
}

enum pubnub_res _receive_effect(
    pbcc_ee_invocation_t*                      invocation,
    const pbcc_ee_data_t*                      context,
    const pbcc_ee_effect_completion_function_t cb)
{
    return _make_subscribe_request(invocation, pbcc_ee_data_value(context), cb);
}

enum pubnub_res _emit_status_effect(
    pbcc_ee_invocation_t*                      invocation,
    const pbcc_ee_data_t*                      context,
    const pbcc_ee_effect_completion_function_t cb)
{
    const pbcc_subscribe_ee_context_t* ctx = pbcc_ee_data_value(context);
    pbcc_subscribe_ee_t* subscribe_ee = ctx->pb->core.subscribe_ee;
    const enum pubnub_res reason = ctx->reason;

    // Update latest Subscribe Event Engine subscription status.
    pubnub_mutex_lock(subscribe_ee->mutw);
    const pubnub_subscription_status status = subscribe_ee->status;
    pubnub_mutex_unlock(subscribe_ee->mutw);

    pbcc_event_listener_emit_status(subscribe_ee->event_listener,
                                    status,
                                    reason);
    cb(subscribe_ee->ee, invocation);

    return PNR_OK;
}

enum pubnub_res _emit_messages_effect(
    pbcc_ee_invocation_t*                      invocation,
    const pbcc_ee_data_t*                      context,
    const pbcc_ee_effect_completion_function_t cb)
{
    const pbcc_subscribe_ee_context_t* ctx = pbcc_ee_data_value(context);
    const pbcc_subscribe_ee_t* subscribe_ee = ctx->pb->core.subscribe_ee;
    pubnub_t* pb = ctx->pb;

    for (struct pubnub_v2_message msg = pubnub_get_v2(pb); msg.payload.size > 0;
         msg                          = pubnub_get_v2(pb)) {
        pbcc_event_listener_emit_message(subscribe_ee->event_listener, msg);
    }

    cb(subscribe_ee->ee, invocation);

    return PNR_OK;
}

void _cancel_effect(
    pbcc_ee_invocation_t*                      invocation,
    const pbcc_ee_data_t*                      context,
    const pbcc_ee_effect_completion_function_t cb)
{
    PUBNUB_ASSERT_OPT(NULL != context);

    const pbcc_subscribe_ee_context_t* ctx = pbcc_ee_data_value(context);
    const pbcc_subscribe_ee_t* subscribe_ee = ctx->pb->core.subscribe_ee;
    PUBNUB_ASSERT(pb_valid_ctx_ptr(ctx->pb));

    if (PN_CANCEL_FINISHED == pubnub_cancel(ctx->pb))
        cb(subscribe_ee->ee, invocation);
}

enum pubnub_res _make_subscribe_request(
    pbcc_ee_invocation_t*                      invocation,
    const pbcc_subscribe_ee_context_t*         context,
    const pbcc_ee_effect_completion_function_t cb)
{
    PUBNUB_ASSERT_OPT(NULL != context);

    pubnub_t* pb = context->pb;
    // Override timetoken stored for next subscription loop.
    pubnub_mutex_lock(pb->monitor);
    size_t token_len = strlen(pb->core.timetoken);
    memcpy(pb->core.timetoken, context->cursor.timetoken, token_len);
    pb->core.timetoken[token_len] = '\0';
    if (context->cursor.region > 0) {
        pb->core.region = context->cursor.region;
    }
    pbpal_mutex_unlock(pb->monitor);

    struct pubnub_subscribe_v2_options opts = pubnub_subscribe_v2_defopts();
    opts.filter_expr = pb->core.subscribe_ee->filter_expr;
    opts.channel_group = pbcc_ee_data_value(context->channel_groups);
    opts.heartbeat = pb->core.subscribe_ee->heartbeat;

    const enum pubnub_res rslt = pubnub_subscribe_v2(
        pb,
        pbcc_ee_data_value(context->channels),
        opts);
    if (PNR_STARTED == rslt) { cb(pb->core.subscribe_ee->ee, invocation); }

    return rslt;
}


// ----------------------------------------------
//           Functions: Event Engine
// ----------------------------------------------

enum pubnub_res _pbcc_subscribe_ee_subscribe(
    const pbcc_subscribe_ee_t*       ee,
    const pubnub_subscribe_cursor_t* cursor,
    const bool                       update)
{
    pbcc_ee_event_t* event = NULL;
    char*            ch    = NULL,* cg = NULL;
    enum pubnub_res  rslt  = PNR_OK;

    if (update) { rslt = _pbcc_subscribe_ee_update_subscribables(ee); }
    if (PNR_OK == rslt) {
        rslt = _pbcc_subscribe_ee_subscribables(ee->subscribables,
                                                &ch,
                                                &cg,
                                                true);
        // Empty list allowed and will mean that event engine should transit to
        // the `Unsubscribed` state.
        if (PNR_INVALID_PARAMETERS == rslt) {
            if (NULL != ch) { free(ch); }
            if (NULL != cg) { free(cg); }
            ch   = NULL;
            cg   = NULL;
            rslt = PNR_OK;
        }
    }

    if (PNR_OK == rslt) {
        const bool restore = NULL != cursor && '0' != cursor->timetoken[0];

        if (NULL == cursor || !restore)
            event = _pbcc_subscription_changed_event_alloc(ee, &ch, &cg);
        else
            event = _pbcc_subscription_restored_event_alloc(
                ee,
                &ch,
                &cg,
                *cursor);
        if (NULL == event) {
            PUBNUB_LOG_ERROR(
                "pbcc_subscribe_ee_subscribe: failed to allocate memory for "
                "event\n");
            rslt = PNR_OUT_OF_MEMORY;
        }
    }

    if (PNR_OK == rslt) {
        rslt = pbcc_ee_handle_event(ee->ee, event);
        // `pubnub_heartbeat` not used separately because `pubnub_subscribe_v2`
        // internally use automated heartbeat feature.
    }

    if (PNR_OK != rslt) {
        if (NULL != event) { pbcc_ee_event_free(event); }
        if (NULL != ch) { free(ch); }
        if (NULL != cg) { free(cg); }
    }

    return rslt;
}

enum pubnub_res _pbcc_subscribe_ee_unsubscribe(
    const pbcc_subscribe_ee_t* ee,
    pbhash_set_t*              subscribables)
{
    char*           ch,* cg;
    bool            send_leave = false;
    enum pubnub_res rslt;

    if (PNR_OK == (rslt = _pbcc_subscribe_ee_update_subscribables(ee))) {
        pbhash_set_subtract(subscribables, ee->subscribables);
        _pbcc_subscribe_ee_subscribables(
            subscribables,
            &ch,
            &cg,
            false);
        if (PNR_OK == rslt || PNR_INVALID_PARAMETERS == rslt) {
            send_leave = PNR_OK == rslt;
            rslt       = PNR_OK;
        }
    }

    // Update user presence information for channels which user actually left.
    if (PNR_OK == rslt && send_leave) {
        pubnub_leave(ee->pb, ch, cg);
    }

    // Update subscription loop.
    if (PNR_OK == rslt)
        rslt = _pbcc_subscribe_ee_subscribe(ee, NULL, false);

    if (NULL != ch) { free(ch); }
    if (NULL != cg) { free(cg); }

    return rslt;
}

enum pubnub_res _pbcc_subscribe_ee_update_subscribables(
    const pbcc_subscribe_ee_t* ee)

{
    PUBNUB_ASSERT_OPT(NULL != ee);

    const pbarray_t* sets = ee->subscription_sets;
    const pbarray_t* subs = ee->subscriptions;
    enum pubnub_res  rslt = PNR_OK;

    // Clean up previous set to start with fresh list.
    pbhash_set_remove_all(ee->subscribables);

    for (int i = 0; i < pbarray_count(subs); ++i) {
        const pubnub_subscription_t* sub = pbarray_element_at(subs, i);
        pbhash_set_t* subc = _pubnub_subscription_subscribables(sub, NULL);
        rslt = _pbcc_subscribe_ee_add_subscribables(ee, subc);
        pbhash_set_free(subc);
        if (PNR_OK != rslt) { return rslt; }
    }

    for (int i = 0; i < pbarray_count(sets); ++i) {
        const pubnub_subscription_set_t* sub = pbarray_element_at(sets, i);
        pbhash_set_t* subc = _pubnub_subscription_set_subscribables(sub);
        rslt = _pbcc_subscribe_ee_add_subscribables(ee, subc);
        pbhash_set_free(subc);
        if (PNR_OK != rslt) { return rslt; }
    }

    return rslt;
}


enum pubnub_res _pbcc_subscribe_ee_add_subscribables(
    const pbcc_subscribe_ee_t* ee,
    const pbhash_set_t*        subscribables)
{
    PUBNUB_ASSERT_OPT(NULL != ee);
    PUBNUB_ASSERT_OPT(NULL != subscribables);

    enum pubnub_res rslt = PNR_OK;
    pbhash_set_t*   dups;

    if (NULL == subscribables ||
        PBHSR_OUT_OF_MEMORY == pbhash_set_union(
            ee->subscribables,
            subscribables,
            &dups)) {
        if (NULL == subscribables) {
            PUBNUB_LOG_ERROR(
                "pbcc_subscribe_ee_add_subscribables: failed to allocate "
                "memory for subscription's subscribables\n");
        }
        else {
            PUBNUB_LOG_ERROR(
                "pbcc_subscribe_ee_add_subscribables: failed to allocate "
                "memory to store subscribables\n");
        }

        rslt = PNR_OUT_OF_MEMORY;
    }

    if (NULL == dups) { return rslt; }

    // Make sure that duplicates will be freed.
    size_t                  dups_count = 0;
    pubnub_subscribable_t** elms       = (pubnub_subscribable_t**)
        pbhash_set_elements(dups, &dups_count);
    for (size_t j = 0; j < dups_count; ++j) {
        pubnub_subscribable_t* elm = elms[j];

        if (PBHSR_VALUE_EXISTS == pbhash_set_match_element(
                ee->subscribables,
                elm)) {
            _pubnub_subscribable_free(elm);
        }
    }
    if (NULL != elms) { free(elms); }
    pbhash_set_free(dups);

    return rslt;
}

enum pubnub_res _pbcc_subscribe_ee_subscribables(
    pbhash_set_t* subscribables,
    char**        channels,
    char**        channel_groups,
    const bool    include_presence)
{
    PUBNUB_ASSERT_OPT(NULL != subscribables);

    // Expected length of comma-separated channel names.
    size_t ch_list_length = 0, ch_count = 0, ch_offset = 0;
    // Expected length of comma-separated channel group names.
    size_t cg_list_length = 0, cg_count = 0, cg_offset = 0;

    size_t                        count;
    const pubnub_subscribable_t** subs = (const pubnub_subscribable_t**)
        pbhash_set_elements(subscribables, &count);

    for (int i = 0; i < count; ++i) {
        const pubnub_subscribable_t* sub = subs[i];
        // Ignoring presence subscribables if they shouldn't be included.
        if (!include_presence && _pubnub_subscribable_is_presence(sub))
            continue;

        if (!_pubnub_subscribable_is_cg(sub)) {
            ch_list_length += _pubnub_subscribable_length(sub);
            ch_count++;
        }
        else {
            cg_list_length += _pubnub_subscribable_length(sub);
            cg_count++;
        }
    }

    const size_t ch_len = ch_count ? 1 : ch_list_length + ch_count;
    const size_t cg_len = cg_count ? 1 : cg_list_length + cg_count;
    *channels           = calloc(ch_len, sizeof(char));
    *channel_groups     = calloc(cg_len, sizeof(char));

    if (NULL == *channels || NULL == *channel_groups) {
        PUBNUB_LOG_ERROR(
            "pbcc_subscribe_ee_subscribables: failed to allocate memory to "
            "store subscription channels / channel groups\n");
        if (NULL != subs) { free(subs); }
        return PNR_OUT_OF_MEMORY;
    }

    for (int i = 0; i < count; ++i) {
        const pubnub_subscribable_t* sub = subs[i];
        // Ignoring presence subscribables if they shouldn't be included.
        if (!include_presence && _pubnub_subscribable_is_presence(sub))
            continue;

        if (!_pubnub_subscribable_is_cg(sub)) {
            ch_offset += snprintf(*channels + ch_offset,
                                  ch_len - ch_offset,
                                  "%s%s",
                                  ch_offset > 0 ? ",": "",
                                  sub->id->ptr);
        }
        else {
            cg_offset += snprintf(*channels + cg_offset,
                                  cg_len - cg_offset,
                                  "%s%s",
                                  cg_offset > 0 ? ",": "",
                                  sub->id->ptr);
        }
    }
    if (NULL != subs) { free(subs); }

    return ch_list_length == 0 && cg_list_length == 0
               ? PNR_INVALID_PARAMETERS
               : PNR_OK;
}

pbcc_subscribe_ee_context_t* _pbcc_subscribe_ee_context_alloc(
    pubnub_t* pb,
    char**    channels,
    char**    channel_groups)
{
    pbcc_subscribe_ee_context_t* context =
        malloc(sizeof(pbcc_subscribe_ee_context_t));
    if (NULL == context) {
        PUBNUB_LOG_ERROR(
            "pbcc_subscribe_ee_context_alloc: failed to allocate memory\n");
        if (NULL != channels && NULL != *channels) {
            free(*channels);
            *channels = NULL;
        }
        if (NULL != channel_groups && NULL != *channel_groups) {
            free(*channel_groups);
            *channel_groups = NULL;
        }
        return NULL;
    }

    context->pb     = pb;
    context->reason = PNR_OK;
    if (NULL != channels && NULL != *channels)
        context->channels = pbcc_ee_data_alloc(*channels, free);
    else
        context->channels = NULL;
    if (NULL != channel_groups && NULL != *channel_groups)
        context->channel_groups = pbcc_ee_data_alloc(*channel_groups, free);
    else
        context->channel_groups = NULL;

    return context;
}

pbcc_subscribe_ee_context_t* _pbcc_subscribe_ee_context_copy(
    pubnub_t*                          pb,
    const pbcc_subscribe_ee_context_t* ctx,
    char**                             channels,
    char**                             channel_groups)
{
    pbcc_subscribe_ee_context_t* context = _pbcc_subscribe_ee_context_alloc(
        pb,
        channels,
        channel_groups);
    if (NULL == context) { return NULL; }
    if (NULL == ctx) { return context; }

    if (NULL == context->channels && NULL != ctx->channels)
        context->channels = pbcc_ee_data_copy(ctx->channels);
    if (NULL == context->channel_groups && NULL != ctx->channel_groups)
        context->channel_groups = pbcc_ee_data_copy(ctx->channel_groups);
    context->cursor = ctx->cursor;

    return context;
}

pbcc_subscribe_ee_context_t* _pbcc_subscribe_ee_current_state_context(
    const pbcc_subscribe_ee_t* ee)
{
    const pbcc_ee_state_t* current_state = pbcc_ee_current_state(ee->ee);
    return NULL != current_state ? current_state->data : NULL;
}

void _pbcc_subscribe_ee_context_free(pbcc_subscribe_ee_context_t* ctx)
{
    if (NULL == ctx) { return; }

    if (NULL != ctx->channels) { pbcc_ee_data_free(ctx->channels); }
    if (NULL != ctx->channel_groups) { pbcc_ee_data_free(ctx->channel_groups); }
    free(ctx);
}