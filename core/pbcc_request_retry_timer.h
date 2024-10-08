/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#ifndef PBCC_REQUEST_RETRY_TIMER_H
#define PBCC_REQUEST_RETRY_TIMER_H
#if PUBNUB_USE_RETRY_CONFIGURATION


/**
 * @file  pbcc_request_retry_timer.h
 * @brief PubNub request retry timer implementation.
 */

#include <stdint.h>

#include "core/pubnub_ntf_callback.h"
#include "core/pubnub_api_types.h"


// ----------------------------------------------
//               Types forwarding
// ----------------------------------------------

/**
 * @brief Failed requests retry timer definition.
 *
 * Timer basically holds further transactions processing by putting FSM into new
 * state for the time of delay.
 */
typedef struct pbcc_request_retry_timer pbcc_request_retry_timer_t;


// ----------------------------------------------
//                   Functions
// ----------------------------------------------

/**
 * @brief Create a request retry timer.
 *
 * @param pb Pointer to the PubNub context for which timer will restart request.
 * @return Pointer to the ready to use request retry timer  or `NULL` in case of
 *         insufficient memory error. The returned pointer must be passed to the
 *         `pbcc_request_retry_timer_free` to avoid a memory leak.
 */
pbcc_request_retry_timer_t* pbcc_request_retry_timer_alloc(pubnub_t* pb);

/**
 * @brief Release resources used by request retry timer object.
 *
 * @note Function will NULLify provided request retry timer pointer.
 *
 * @param timer Pointer to the timer object, which should free up resources.
 */
void pbcc_request_retry_timer_free(pbcc_request_retry_timer_t** timer);

/**
 * @brief Start request retry timer.
 *
 * This function will be blocking if built without PUBNUB_CALLBACK_API
 * precompile macro.
 *
 * @param timer Pointer to the timer which will start and call completion
 *              handler on timeout.
 * @param delay Timer timeout in milliseconds.
 */
void pbcc_request_retry_timer_start(
    pbcc_request_retry_timer_t* timer,
    uint16_t delay);

/**
 * @brief Stop request retry timer.
 *
 * @param timer Pointer to the timer which should stop earlier that timeout.
 */
void pbcc_request_retry_timer_stop(pbcc_request_retry_timer_t* timer);
#endif // #if PUBNUB_USE_RETRY_CONFIGURATION
#endif // #ifndef PBCC_REQUEST_RETRY_TIMER_H
