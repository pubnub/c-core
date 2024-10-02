/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#ifndef PUBNUB_RETRY_CONFIGURATION_H
#define PUBNUB_RETRY_CONFIGURATION_H
#if PUBNUB_USE_RETRY_CONFIGURATION


/**
 * @file  pubnub_retry_configuration.h
 * @brief PubNub request retry configuration.
 */

#include "core/pubnub_api_types.h"
#include "lib/pb_extern.h"


// ----------------------------------------------
//               Types forwarding
// ----------------------------------------------

/**
 * @brief Failed requests retry configuration definition.
 *
 * PubNub client can automatically retry failed requests basing on retry
 * configuration criteria.
 */
typedef struct pubnub_retry_configuration pubnub_retry_configuration_t;


// ----------------------------------------------
//                    Types
// ----------------------------------------------

/** Excluded endpoint groups definition. */
typedef enum
{
    /** Unknown API endpoint group. */
    PNRC_UNKNOWN_ENDPOINT,
    /** The endpoints to send messages. */
    PNRC_MESSAGE_SEND_ENDPOINT,
    /** The endpoint for real-time update retrieval. */
    PNRC_SUBSCRIBE_ENDPOINT,
    /**
     * @brief The endpoint to access and manage `user_id` presence and fetch
     *        channel presence information.
     */
    PNRC_PRESENCE_ENDPOINT,
    /**
     * @brief The endpoint to access and manage messages for a specific
     *        channel(s) in the persistent storage.
     */
    PNRC_MESSAGE_STORAGE_ENDPOINT,
    /** The endpoint to access and manage channel groups. */
    PNRC_CHANNEL_GROUPS_ENDPOINT,
#if PUBNUB_USE_OBJECTS_API
    /** The endpoint to access and manage App Context objects. */
    PNRC_APP_CONTEXT_ENDPOINT,
#endif // #if PUBNUB_USE_OBJECTS_API
#if PUBNUB_USE_ACTIONS_API
    /** The endpoint to access and manage reactions for a specific message. */
    PNRC_MESSAGE_REACTIONS_ENDPOINT,
#endif // #if PUBNUB_USE_ACTIONS_API
#if PUBNUB_USE_GRANT_TOKEN_API
    /** The endpoint to access and manage permissions (auth keys / tokens). */
    PNRC_PAM_ENDPOINT,
#endif // #if PUBNUB_USE_GRANT_TOKEN_API
    /** Number of known endpoint groups. */
    PNRC_ENDPOINTS_COUNT
} pubnub_endpoint_t;


// ----------------------------------------------
//                   Functions
// ----------------------------------------------

/**
 * @brief Set failed request retry configuration.
 * @code
 * // Create default configuration with linear retry policy.
 * pubnub_retry_configuration_t* configuration =
 *     pubnub_retry_configuration_linear_alloc(void);
 * pubnub_set_retry_configuration(pb, configuration);
 * @endcode
 *
 * @note `PubNub` context will free configuration instance on `deinit`.
 *
 * @param pb            Pointer to the PubNub context which failed to complete
 *                      transaction and will retry them depending from
 *                      configuration.
 * @param configuration Pointer to the request retry configuration, which will
 *                      be used by PubNub to compute delays between retry
 *                      attempts.
 *
 * @see pubnub_retry_configuration_linear_alloc
 */
PUBNUB_EXTERN void pubnub_set_retry_configuration(
    pubnub_t*                     pb,
    pubnub_retry_configuration_t* configuration);

/**
 * @brief Create a request retry configuration with a linear retry policy.
 * @code
 * // Create default configuration with linear retry policy.
 * pubnub_retry_configuration_t* configuration =
 *     pubnub_retry_configuration_linear_alloc(void);
 * @endcode
 *
 * Configurations with a linear retry policy will use equal delays between retry
 * attempts. The default implementation uses a \b 2 seconds delay and \b 10
 * maximum retry attempts.
 *
 * @note The PubNub client will schedule request retries after a calculated
 *       delay, but the actual retry may not happen at the same moment (there is
 *       a small leeway).
 *
 * @return Pointer to the ready to use PubNub request retry configuration or
 *         `NULL` in case of insufficient memory error. The returned pointer
 *         must be passed to the `pubnub_retry_configuration_free` to avoid a
 *         memory leak.
 *
 * @see pubnub_set_retry_configuration
 * @see pubnub_retry_configuration_free
 */
PUBNUB_EXTERN pubnub_retry_configuration_t*
    pubnub_retry_configuration_linear_alloc(void);

/**
 * @brief Create a request retry configuration with a linear retry policy and
 *        list of API groups which won't be retried.
 * @code
 * // Create default configuration with linear retry policy and excluded
 * // endpoints: publish and history.
 * pubnub_retry_configuration_t* configuration =
 *     pubnub_retry_configuration_linear_alloc_with_excluded(
 *         PNRC_MESSAGE_SEND_ENDPOINT,
 *         PNRC_MESSAGE_STORAGE_ENDPOINT,
 *         -1);
 * @endcode
 *
 * Configurations with a linear retry policy will use equal delays between retry
 * attempts. The default implementation uses a \b 2 seconds delay and \b 10
 * maximum retry attempts.
 *
 * @note The PubNub client will schedule request retries after a calculated
 *       delay, but the actual retry may not happen at the same moment (there is
 *       a small leeway).
 *
 * @param excluded Endpoint for which retry shouldn't be used.
 * @param ...      Variadic list of endpoints for which retry shouldn't be used.
 *                 <br/>\b Important: list should be terminated by \b -1.
 * @return Pointer to the ready to use PubNub request retry configuration  or
 *         `NULL` in case of insufficient memory error. The returned pointer
 *         must be passed to the `pubnub_retry_configuration_free` to avoid a
 *         memory leak.
 *
 * @see pubnub_set_retry_configuration
 * @see pubnub_retry_configuration_free
 */
PUBNUB_EXTERN pubnub_retry_configuration_t*
pubnub_retry_configuration_linear_alloc_with_excluded(int excluded, ...);

/**
 * @brief Create a request retry configuration with a linear retry policy and
 *        list of API groups which won't be retried.
 * @code
 * // Create custom configuration with linear retry policy and excluded
 * // endpoints: publish and history.
 * pubnub_retry_configuration_t* configuration =
 *     pubnub_retry_configuration_linear_alloc_with_options(
 *         5,
 *         3,
 *         PNRC_MESSAGE_SEND_ENDPOINT,
 *         PNRC_MESSAGE_STORAGE_ENDPOINT,
 *         -1);
 * @endcode
 *
 * Configurations with a linear retry policy will use equal delays between retry
 * attempts. The default implementation uses a \b 2 seconds delay and \b 10
 * maximum retry attempts.
 *
 * @note The PubNub client will schedule request retries after a calculated
 *       delay, but the actual retry may not happen at the same moment (there is
 *       a small leeway).
 *
 * @param delay         Delay between failed requests automatically retries
 *                      attempts. \b Important: The minimum allowed delay
 *                      is \b 2.0.
 * @param maximum_retry The number of failed requests that should be retried
 *                      automatically before reporting an error.
 *                      \b Important: The maximum allowed number of retries
 *                      is \b 10.
 * @param excluded      Endpoint for which retry shouldn't be used.
 * @param ...           Variadic list of endpoints for which retry shouldn't be
 *                      used. \b Important: list should be terminated by \b -1.
 * @return Pointer to the ready to use PubNub request retry configuration  or
 *         `NULL` in case of insufficient memory error. The returned pointer
 *         must be passed to the `pubnub_retry_configuration_free` to avoid a
 *         memory leak.
 *
 * @see pubnub_set_retry_configuration
 * @see pubnub_retry_configuration_free
 */
PUBNUB_EXTERN pubnub_retry_configuration_t*
pubnub_retry_configuration_linear_alloc_with_options(
    int delay,
    int maximum_retry,
    int excluded,
    ...);

/**
 * @brief Create a request retry configuration with a exponential retry policy.
 * @code
 * // Create default configuration with exponential retry policy.
 * pubnub_retry_configuration_t* configuration =
 *     pubnub_retry_configuration_exponential_alloc(void);
 * @endcode
 *
 * Configurations with an exponential retry policy will use `minimum_delay` that
 * will exponentially increase with each failed request retry attempt.<br/>
 * The default implementation uses a \b 2 seconds minimum, \b 150 maximum
 * delays, and \b 6 maximum retry attempts.
 *
 * @note The PubNub client will schedule request retries after a calculated
 *       delay, but the actual retry may not happen at the same moment (there is
 *       a small leeway).
 *
 * @return Pointer to the ready to use PubNub request retry configuration  or
 *         `NULL` in case of insufficient memory error. The returned pointer
 *         must be passed to the `pubnub_retry_configuration_free` to avoid a
 *         memory leak.
 *
 * @see pubnub_set_retry_configuration
 * @see pubnub_retry_configuration_free
 */
PUBNUB_EXTERN pubnub_retry_configuration_t*
    pubnub_retry_configuration_exponential_alloc(void);

/**
 * @brief Create a request retry configuration with a exponential retry policy
 *        and list of API groups which won't be retried.
 * @code
 * // Create default configuration with exponential retry policy and excluded
 * // endpoints: publish and history.
 * pubnub_retry_configuration_t* configuration =
 *     pubnub_retry_configuration_exponential_alloc_with_excluded(
 *         PNRC_MESSAGE_SEND_ENDPOINT,
 *         PNRC_MESSAGE_STORAGE_ENDPOINT,
 *         -1);
 * @endcode
 *
 * Configurations with an exponential retry policy will use `minimum_delay` that
 * will exponentially increase with each failed request retry attempt.<br/>
 * The default implementation uses a \b 2 seconds minimum, \b 150 maximum
 * delays, and \b 6 maximum retry attempts.
 *
 * @note The PubNub client will schedule request retries after a calculated
 *       delay, but the actual retry may not happen at the same moment (there is
 *       a small leeway).
 *
 * @param excluded Endpoint for which retry shouldn't be used.
 * @param ...      Variadic list of endpoints for which retry shouldn't be used.
 *                 \b Important: list should be terminated by \b -1.
 * @return Pointer to the ready to use PubNub request retry configuration  or
 *         `NULL` in case of insufficient memory error. The returned pointer
 *         must be passed to the `pubnub_retry_configuration_free` to avoid a
 *         memory leak.
 *
 * @see pubnub_set_retry_configuration
 * @see pubnub_retry_configuration_free
 */
PUBNUB_EXTERN pubnub_retry_configuration_t*
pubnub_retry_configuration_exponential_alloc_with_excluded(int excluded, ...);

/**
 * @brief Create a request retry configuration with a exponential retry policy
 *        and list of API groups which won't be retried.
 * @code
 * // Create custom configuration with linear retry policy and excluded
 * // endpoints: publish and history.
 * pubnub_retry_configuration_t* configuration =
 *     pubnub_retry_configuration_linear_alloc_with_options(
 *         5,
 *         20,
 *         6,
 *         PNRC_MESSAGE_SEND_ENDPOINT,
 *         PNRC_MESSAGE_STORAGE_ENDPOINT,
 *         -1);
 * @endcode
 *
 * Configurations with an exponential retry policy will use `minimum_delay` that
 * will exponentially increase with each failed request retry attempt.<br/>
 * The default implementation uses a \b 2 seconds minimum, \b 150 maximum
 * delays, and \b 6 maximum retry attempts.
 *
 * @note The PubNub client will schedule request retries after a calculated
 *       delay, but the actual retry may not happen at the same moment (there is
 *       a small leeway).
 *
 * @param minimum_delay Base delay, which will be used to calculate the next
 *                      delay depending on the number of retry attempts.
 *                      \b Important: The minimum allowed delay is \b 2.
 * @param maximum_delay Maximum allowed computed delay that should be used
 *                      between retry attempts.
 * @param maximum_retry The number of failed requests that should be retried
 *                      automatically before reporting an error.
 *                      \b Important: The maximum allowed number of retries
 *                      is \b 10.
 * @param excluded      Endpoint for which retry shouldn't be used.
 * @param ...           Variadic list of endpoints for which retry shouldn't be
 *                      used. \b Important: list should be terminated by \b -1.
 * @return Pointer to the ready to use PubNub request retry configuration  or
 *         `NULL` in case of insufficient memory error. The returned pointer
 *         must be passed to the `pubnub_retry_configuration_free` to avoid a
 *         memory leak.
 *
 * @see pubnub_set_retry_configuration
 * @see pubnub_retry_configuration_free
 */
PUBNUB_EXTERN pubnub_retry_configuration_t*
pubnub_retry_configuration_exponential_alloc_with_options(
    int minimum_delay,
    int maximum_delay,
    int maximum_retry,
    int excluded,
    ...);

/**
 * @brief Release resources used by request retry configuration object.
 *
 * @param configuration Pointer to the configuration object, which should free
 *                      up resources.
 */
PUBNUB_EXTERN void pubnub_retry_configuration_free(
    pubnub_retry_configuration_t** configuration);
#endif // #if PUBNUB_USE_RETRY_CONFIGURATION
#endif // #ifndef PUBNUB_RETRY_CONFIGURATION_H
