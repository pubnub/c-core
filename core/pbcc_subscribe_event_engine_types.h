/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#ifndef PBCC_SUBSCRIBE_EVENT_ENGINE_TYPES_H
#define PBCC_SUBSCRIBE_EVENT_ENGINE_TYPES_H


/**
 * @file  pbcc_subscribe_event_engine_types.h
 * @brief Subscribe Event Engine shared types.
 */

#include "core/pbcc_subscribe_event_listener.h"
#include "core/pbcc_event_engine.h"
#include "core/pubnub_mutex.h"
#include "lib/pbhash_set.h"


// ----------------------------------------------
//                     Types
// ----------------------------------------------

/** Subscribe Event Engine events. */
typedef enum {
    /** Subscription list of channels / groups change event. */
    SUBSCRIBE_EE_EVENT_SUBSCRIPTION_CHANGED,
    /** Subscription restore / catchup event. */
    SUBSCRIBE_EE_EVENT_SUBSCRIPTION_RESTORED,
    /** Initial subscription handshake success event. */
    SUBSCRIBE_EE_EVENT_HANDSHAKE_SUCCESS,
    /** Initial subscription handshake failure event. */
    SUBSCRIBE_EE_EVENT_HANDSHAKE_FAILURE,
    /** Real-time updates receive success event. */
    SUBSCRIBE_EE_EVENT_RECEIVE_SUCCESS,
    /** Real-time updates receive failure event. */
    SUBSCRIBE_EE_EVENT_RECEIVE_FAILURE,
    /** Disconnect from real-time updates event. */
    SUBSCRIBE_EE_EVENT_DISCONNECT,
    /** Reconnect for real-time updates event. */
    SUBSCRIBE_EE_EVENT_RECONNECT,
    /** Unsubscribe from all channel / groups. */
    SUBSCRIBE_EE_EVENT_UNSUBSCRIBE_ALL,
} pbcc_subscribe_ee_event;

/** Subscribe Event Engine states. */
typedef enum {
    /**
     * @brief Unknown state.
     *
     * Placeholder state type without actual implementation which won't be
     * treated as insufficient memory error.
     */
    SUBSCRIBE_EE_STATE_NONE,
    /** Inactive Subscribe Event Engine state. */
    SUBSCRIBE_EE_STATE_UNSUBSCRIBED,
    /** Initial subscription state. */
    SUBSCRIBE_EE_STATE_HANDSHAKING,
    /** Initial subscription failed state. */
    SUBSCRIBE_EE_STATE_HANDSHAKE_FAILED,
    /** Initial subscription stopped state. */
    SUBSCRIBE_EE_STATE_HANDSHAKE_STOPPED,
    /** Real-time updates receiving state. */
    SUBSCRIBE_EE_STATE_RECEIVING,
    /** Real-time updates receive failed state. */
    SUBSCRIBE_EE_STATE_RECEIVE_FAILED,
    /** Real-time updates receive stopped state. */
    SUBSCRIBE_EE_STATE_RECEIVE_STOPPED,
} pbcc_subscribe_ee_state;

/** Subscribe Event Engine events. */
typedef enum {
 /** Subscription list of channels / groups handshake invocation. */
 SUBSCRIBE_EE_INVOCATION_HANDSHAKE,
 /** Receive real-time updates for list of channels / groups invocation. */
 SUBSCRIBE_EE_INVOCATION_RECEIVE,
 /** Emit subscription change status invocation. */
 SUBSCRIBE_EE_INVOCATION_EMIT_STATUS,
 /** Emit real-time update invocation. */
 SUBSCRIBE_EE_INVOCATION_EMIT_MESSAGE,
 /** Cancel subscription invocation. */
 SUBSCRIBE_EE_INVOCATION_CANCEL,
} pbcc_subscribe_ee_invocation;

/** Subscribe event engine structure. */
struct pbcc_subscribe_ee {
    /**
     * @brief Pointer to the set of subscribable objects which should be used in
     *        subscription loop.
     *
     * \b Important: Array allocated with `pubnub_subscribable_free_` elements
     * destructor.
     */
    pbhash_set_t* subscribables;
    /**
     * @brief Pointer to the list of active subscription sets.
     *
     * List of subscription sets which should be used in subscription loop and
     * listen for real-time updates.
     *
     * \b Important: Array allocated with `pubnub_subscription_set_free_`
     * elements destructor.
     */
    pbarray_t* subscription_sets;
    /**
     * @brief List of active subscriptions.
     *
     * List of subscriptions which should be used in subscription loop and
     * listen for real-time updates.
     *
     * \b Important: Array allocated with `pubnub_subscription_free_`
     * elements destructor.
     */
    pbarray_t* subscriptions;
    /**
     * @brief How long (in seconds) the server will consider the client alive
     *        for presence.
     */
    unsigned heartbeat;
    /** Pointer to the real-time updates filtering expression. */
    char* filter_expr;
    /** Recent subscription status. */
    pubnub_subscription_status status;
    /** Pointer to the Subscribe Event Listener. */
    pbcc_event_listener_t* event_listener;
    /**
     * @brief Pointer to the active subscription cancel invocation.
     *
     * PubNub context may need some time to complete cancellation operation and
     * report to the callback
     */
    pbcc_ee_invocation_t* cancel_invocation;
    /**
     * @brief Pointer to the list of channels to leave.
     *
     * @note Because of PubNub specific with single request at a time we need to
     *       wait when cancel invocation will complete to be able to send
     *       `leave` request for channels.
     */
    pbarray_t* leave_channels;
    /**
     * @brief Pointer to the list of channel groups to leave.
     *
     * @note Because of PubNub specific with single request at a time we need to
     *       wait when cancel invocation will complete to be able to send
     *       `leave` request for channel groups.
     */
    pbarray_t* leave_channel_groups;
    /** Type of transaction which is currently in progress. */
    enum pubnub_trans current_transaction;
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
    /** Shared resources access lock. */
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
    /** Subscription cursor which is or will be used in subscription loop. */
    pubnub_subscribe_cursor_t cursor;
    /** Previous request failure reason. */
    enum pubnub_res reason;
    /** Whether subscription requires heartbeat to be sent before subscribe. */
    bool send_heartbeat;
    /**
     * @brief Pointer to the PubNub context, which should be used for effects
     *        execution by Subscribe Event Engine effects dispatcher.
     */
    pubnub_t* pb;
} pbcc_subscribe_ee_context_t;
#endif //PBCC_SUBSCRIBE_EVENT_ENGINE_TYPES_H
