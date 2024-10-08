/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#ifndef PBCC_SUBSCRIBE_EVENT_ENGINE_TRANSITIONS_H
#define PBCC_SUBSCRIBE_EVENT_ENGINE_TRANSITIONS_H
#if PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE
#if !PUBNUB_USE_SUBSCRIBE_V2
#error Subscribe event engine requires subscribe v2 API, so you must define PUBNUB_USE_SUBSCRIBE_V2=1
#endif // #if !PUBNUB_USE_SUBSCRIBE_V2


/**
 * @file  pbcc_subscribe_event_engine_transitions.h
 * @brief Subscribe Event Engine state transitions.
 */

#include "core/pbcc_event_engine.h"


// ----------------------------------------------
//                   Functions
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
pbcc_ee_transition_t* pbcc_unsubscribed_state_transition_alloc(
    pbcc_event_engine_t* ee,
    const pbcc_ee_state_t* current_state,
    const pbcc_ee_event_t* event);

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
pbcc_ee_transition_t* pbcc_handshaking_state_transition_alloc(
    pbcc_event_engine_t* ee,
    const pbcc_ee_state_t* current_state,
    const pbcc_ee_event_t* event);

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
pbcc_ee_transition_t* pbcc_handshake_failed_state_transition_alloc(
    pbcc_event_engine_t* ee,
    const pbcc_ee_state_t* current_state,
    const pbcc_ee_event_t* event);

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
pbcc_ee_transition_t* pbcc_handshake_stopped_state_transition_alloc(
    pbcc_event_engine_t* ee,
    const pbcc_ee_state_t* current_state,
    const pbcc_ee_event_t* event);

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
pbcc_ee_transition_t* pbcc_receiving_state_transition_alloc(
    pbcc_event_engine_t* ee,
    const pbcc_ee_state_t* current_state,
    const pbcc_ee_event_t* event);

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
pbcc_ee_transition_t* pbcc_receiving_failed_state_transition_alloc(
    pbcc_event_engine_t* ee,
    const pbcc_ee_state_t* current_state,
    const pbcc_ee_event_t* event);

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
pbcc_ee_transition_t* pbcc_receiving_stopped_state_transition_alloc(
    pbcc_event_engine_t* ee,
    const pbcc_ee_state_t* current_state,
    const pbcc_ee_event_t* event);
#else // #if PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE
#error To use subscribe event engine API you must define PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE=1
#endif // #if PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE
#endif //PBCC_SUBSCRIBE_EVENT_ENGINE_TRANSITIONS_H
