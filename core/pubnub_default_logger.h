/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#ifndef PUBNUB_DEFAULT_LOGGER_H
#define PUBNUB_DEFAULT_LOGGER_H


/**
 * @file  pubnub_default_logger.h
 * @brief Default platform logger for PubNub C-core SDK.
 *
 * Declares the factory function for the default logger. The actual
 * implementation is platform-specific:
 *
 *  - Desktop (POSIX / Windows): pubnub_stdio_logger.c — writes to
 *    stdout / stderr via fprintf().
 *  - FreeRTOS: pubnub_freertos_logger.c — writes via printf() and
 *    uses FreeRTOS tick count when wall-clock time is unavailable.
 *
 * Only one implementation is linked at build time; the header remains
 * the same across all platforms.
 */


#include "pubnub_logger.h"


// ----------------------------------------------
//                   Functions
// ----------------------------------------------

/**
 * @brief Create the default platform logger for PubNub.
 *
 * @return Pointer to the initialized logger, or `NULL` on failure.
 *         The returned pointer must be passed to `pubnub_logger_free`
 *         to avoid a memory leak.
 *
 * @see pubnub_logger_free
 */
pubnub_logger_t* pubnub_default_logger_alloc(void);
#endif /* PUBNUB_DEFAULT_LOGGER_H */
