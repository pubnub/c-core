/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#ifndef PBCC_SUBSCRIBE_EVENT_ENGINE_EVENTS_H
#define PBCC_SUBSCRIBE_EVENT_ENGINE_EVENTS_H
#if PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE
#if !PUBNUB_USE_SUBSCRIBE_V2
#error Subscribe event engine requires subscribe v2 API, so you must define PUBNUB_USE_SUBSCRIBE_V2=1
#endif // #if !PUBNUB_USE_SUBSCRIBE_V2


/**
 * @file  pbcc_subscribe_event_engine_events.h
 * @brief Subscribe Event Engine states transition events.
 */

#include "core/pbcc_subscribe_event_engine_types.h"
#include "core/pbcc_subscribe_event_engine.h"
#include "core/pbcc_event_engine.h"


// ----------------------------------------------
//                   Functions
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
 * @param sent_by_ee              Whether event has been sent by Subscribe Event
 *                                Engine or not.
 * @return Pointer to the `Subscription change event`, which will be processed
 *         by the Subscribe Event Engine to get transition instructions from the
 *         current state.
 */
pbcc_ee_event_t* pbcc_subscription_changed_event_alloc(
    pbcc_subscribe_ee_t* ee,
    char** channels,
    char** channel_groups,
    bool sent_by_ee);

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
 * @param sent_by_ee              Whether event has been sent by Subscribe Event
 *                                Engine or not.
 * @return Pointer to the `Subscription restore event`, which will be processed
 *         by the Subscribe Event Engine to get transition instructions from the
 *         current state.
 */
pbcc_ee_event_t* pbcc_subscription_restored_event_alloc(
    pbcc_subscribe_ee_t* ee,
    char** channels,
    char** channel_groups,
    pubnub_subscribe_cursor_t cursor,
    bool sent_by_ee);

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
pbcc_ee_event_t* pbcc_handshake_success_event_alloc(
    pbcc_subscribe_ee_t* ee,
    pubnub_subscribe_cursor_t cursor);

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
pbcc_ee_event_t* pbcc_handshake_failure_event_alloc(
    pbcc_subscribe_ee_t* ee,
    enum pubnub_res reason);

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
pbcc_ee_event_t* pbcc_receive_success_event_alloc(
    pbcc_subscribe_ee_t* ee,
    pubnub_subscribe_cursor_t cursor);

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
pbcc_ee_event_t* pbcc_receive_failure_event_alloc(
    pbcc_subscribe_ee_t* ee,
    enum pubnub_res reason);

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
pbcc_ee_event_t* pbcc_disconnect_event_alloc(pbcc_subscribe_ee_t* ee);

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
pbcc_ee_event_t* pbcc_reconnect_event_alloc(
    pbcc_subscribe_ee_t* ee,
    pubnub_subscribe_cursor_t cursor);

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
pbcc_ee_event_t* pbcc_unsubscribe_all_event_alloc(
    pbcc_subscribe_ee_t* ee,
    char** channels,
    char** channel_groups);
#else // #if PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE
#error To use subscribe event engine API you must define PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE=1
#endif // #if PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE
#endif //PBCC_SUBSCRIBE_EVENT_ENGINE_EVENTS_H
