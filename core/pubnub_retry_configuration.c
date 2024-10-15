/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_retry_configuration_internal.h"

#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "core/pubnub_api_types.h"
#include "core/pubnub_pubsubapi.h"
#include "core/pubnub_assert.h"
#include "core/pubnub_mutex.h"
#include "core/pubnub_log.h"
#include "pubnub_internal.h"


// ----------------------------------------------
//                   Constants
// ----------------------------------------------

/** Minimum and default delay between retry attempt. */
#define DEFAULT_MINIMUM_DELAY 2
/** Maximum delay between retry attempt. */
#define DEFAULT_MAXIMUM_DELAY 150
/** Default failed request retry attempts with linear retry policy. */
#define DEFAULT_LINEAR_RETRY_ATTEMPTS 10
/** Default failed request retry attempts with exponential retry policy. */
#define DEFAULT_EXPONENTIAL_RETRY_ATTEMPTS 6


// ----------------------------------------------
//                    Types
// ----------------------------------------------

/** Retry policy type. */
typedef enum {
    /** Use equal delays between retry attempts. */
    PUBNUB_LINEAR_RETRY_POLICY,
    /**
     * @brief Use a `minimum_delay` delay that will exponentially increase with
     * each failed request retry attempt.
     */
    PUBNUB_EXPONENTIAL_RETRY_POLICY,
} pubnub_retry_policy_t;

/** Failed requests retry configuration definition. */
struct pubnub_retry_configuration {
    /** Delay computation retry policy. */
    pubnub_retry_policy_t policy;
    /**
     * @brief Maximum allowed number of failed requests that should be retried
     *        automatically before reporting an error.
     *
     * \b Important: The maximum allowed number of retries is \b 10.
     */
    int max_retries;
    /**
     * @brief Minimum delay between retry attempts.
     *
     * The delay is used for the `PUBNUB_LINEAR_RETRY_POLICY` policy, which is
     * used between retry attempts. For the `PUBNUB_EXPONENTIAL_RETRY_POLICY`
     * policy, which is used as the `minimum_delay`, which will be used to
     * calculate the next delay based on the number of retry attempts.
     *
     * \b Important: The minimum allowed delay is \b 2.
     */
    int minimum_delay;
    /**
     * @brief Maximum allowed computed delay that should be used between retry
     *        attempts.
     */
    int maximum_delay;
    /** Excluded endpoints map. */
    bool excluded_endpoints[PNRC_ENDPOINTS_COUNT];
};


// ----------------------------------------------
//              Function prototypes
// ----------------------------------------------

/**
 * @brief Create a request retry configuration with a linear retry policy and
 *        list of API groups which won't be retried.
 *
 * Configurations with a linear retry policy will use equal delays between retry
 * attempts. The default implementation uses a \b 2 seconds delay and \b 10
 * maximum retry attempts.
 *
 * @note The PubNub client will schedule request retries after a calculated
 *       delay, but the actual retry may not happen at the same moment (there is
 *       a small leeway).
 *
 * @param policy        Retry policy type.
 * @param minimum_delay Delay is used for the 'PUBNUB_LINEAR_RETRY_POLICY'
 *                      policy, which is used between retry attempts. For the
 *                      'PUBNUB_EXPONENTIAL_RETRY_POLICY' policy, which is used
 *                      as the `minimum_delay`, which will be used to calculate
 *                      the next delay based on the number of retry attempts.
 * @param maximum_delay Maximum allowed computed delay that should be used
 *                      between retry attempts.
 * @param maximum_retry The number of failed requests that should be retried
 *                      automatically before reporting an error.
 * @param excluded      Endpoint for which retry shouldn't be used.
 * @param endpoints     Variadic list of endpoints for which retry shouldn't be
 *                      used. \b Important: list should be terminated by \b -1.
 * @return Pointer to the ready to use PubNub request retry configuration or
 *         `NULL` in case of error insufficient memory error. The returned
 *         pointer must be passed to the `pubnub_retry_configuration_free` to
 *         avoid a memory leak.
 */
static pubnub_retry_configuration_t*
pubnub_retry_configuration_alloc_with_options_(
    pubnub_retry_policy_t policy,
    int                   minimum_delay,
    int                   maximum_delay,
    int                   maximum_retry,
    pubnub_endpoint_t     excluded,
    va_list               endpoints);

/**
 * @brief Handle user-provided excluded endpoints for retry configuration.
 *
 * @param configuration     Pointer to the configuration for which list of
 *                          exclusions should be set.
 * @param excluded_endpoint First entry in the list of excluded endpoints.
 * @param endpoints         Variadic list of other excluded endpoints.
 */
static void pubnub_retry_configuration_set_excluded_(
    pubnub_retry_configuration_t* configuration,
    pubnub_endpoint_t             excluded_endpoint,
    va_list                       endpoints);

/**
 * @brief Check whether `transaction` can be retried after failure.
 *
 * @param configuration Pointer to the configuration with failed request
 *                      handling details.
 * @param transaction   Failed transaction type.
 * @param current_retry How many consequential retry attempts has been done.
 * @param http_status   Failed transaction status code.
 * @return `true` if transaction can be handled once more after retry.
 */
static bool pubnub_retry_configuration_retryable_(
    const pubnub_retry_configuration_t* configuration,
    enum pubnub_trans                   transaction,
    int                                 current_retry,
    int                                 http_status);

/**
 * @brief Identify API endpoint group from transaction type.
 *
 * @param trans Transaction which should be translated.
 * @return One of known PubNub API endpoint group type.
 */
static pubnub_endpoint_t pubnub_endpoint_from_transaction_(
    enum pubnub_trans trans);


// ----------------------------------------------
//                   Functions
// ----------------------------------------------

void pubnub_set_retry_configuration(
    pubnub_t*                     pb,
    pubnub_retry_configuration_t* configuration)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    pubnub_mutex_lock(pb->monitor);
    if (NULL != pb->core.retry_configuration)
        pubnub_retry_configuration_free(&pb->core.retry_configuration);
    pb->core.retry_configuration = configuration;
    pubnub_mutex_unlock(pb->monitor);
}

pubnub_retry_configuration_t* pubnub_retry_configuration_linear_alloc(void)
{
    return pubnub_retry_configuration_linear_alloc_with_excluded(-1);
}

pubnub_retry_configuration_t*
pubnub_retry_configuration_linear_alloc_with_excluded(
    const int excluded,
    ...)
{
    va_list args;
    va_start(args, excluded);
    pubnub_retry_configuration_t* configuration =
        pubnub_retry_configuration_alloc_with_options_(
            PUBNUB_LINEAR_RETRY_POLICY,
            DEFAULT_MINIMUM_DELAY,
            0,
            DEFAULT_LINEAR_RETRY_ATTEMPTS,
            excluded,
            args);
    va_end(args);

    return configuration;
}

pubnub_retry_configuration_t*
pubnub_retry_configuration_linear_alloc_with_options(
    const int delay,
    const int maximum_retry,
    const int excluded,
    ...)
{
    va_list args;
    va_start(args, excluded);
    pubnub_retry_configuration_t* configuration =
        pubnub_retry_configuration_alloc_with_options_(
            PUBNUB_LINEAR_RETRY_POLICY,
            delay,
            0,
            maximum_retry,
            excluded,
            args);
    va_end(args);

    return configuration;
}

pubnub_retry_configuration_t* pubnub_retry_configuration_exponential_alloc(void)
{
    return pubnub_retry_configuration_exponential_alloc_with_excluded(-1);
}

pubnub_retry_configuration_t*
pubnub_retry_configuration_exponential_alloc_with_excluded(
    const int excluded,
    ...)
{
    va_list args;
    va_start(args, excluded);
    pubnub_retry_configuration_t* configuration =
        pubnub_retry_configuration_alloc_with_options_(
            PUBNUB_EXPONENTIAL_RETRY_POLICY,
            DEFAULT_MINIMUM_DELAY,
            DEFAULT_MAXIMUM_DELAY,
            DEFAULT_EXPONENTIAL_RETRY_ATTEMPTS,
            excluded,
            args);
    va_end(args);

    return configuration;
}

pubnub_retry_configuration_t*
pubnub_retry_configuration_exponential_alloc_with_options(
    const int minimum_delay,
    const int maximum_delay,
    const int maximum_retry,
    const int excluded,
    ...)
{
    va_list args;
    va_start(args, excluded);
    pubnub_retry_configuration_t* configuration =
        pubnub_retry_configuration_alloc_with_options_(
            PUBNUB_EXPONENTIAL_RETRY_POLICY,
            minimum_delay,
            maximum_delay,
            maximum_retry,
            excluded,
            args);
    va_end(args);

    return configuration;
}

void pubnub_retry_configuration_free(
    pubnub_retry_configuration_t** configuration)
{
    if (NULL == configuration || NULL == *configuration) { return; }

    free(*configuration);
    *configuration = NULL;
}

pubnub_retry_configuration_t* pubnub_retry_configuration_alloc_with_options_(
    const pubnub_retry_policy_t policy,
    const int                   minimum_delay,
    const int                   maximum_delay,
    const int                   maximum_retry,
    const pubnub_endpoint_t     excluded,
    const va_list               endpoints)
{
    pubnub_retry_configuration_t* configuration = malloc(
        sizeof(pubnub_retry_configuration_t));
    if (NULL == configuration) {
        PUBNUB_LOG_ERROR("pubnub_retry_configuration_linear_alloc: failed to "
            "allocate memory\n");
        return configuration;
    }

    memset(configuration->excluded_endpoints,
           false,
           sizeof(configuration->excluded_endpoints));
    configuration->policy        = policy;
    configuration->minimum_delay = minimum_delay;
    configuration->maximum_delay = maximum_delay;
    configuration->max_retries   = maximum_retry;

    pubnub_retry_configuration_set_excluded_(configuration,
                                             excluded,
                                             endpoints);

    return configuration;
}

bool pubnub_retry_configuration_retryable_result_(pubnub_t* pb)
{
    switch (pb->core.last_result) {
    case PNR_ADDR_RESOLUTION_FAILED:
    case PNR_WAIT_CONNECT_TIMEOUT:
    case PNR_CONNECT_FAILED:
    case PNR_CONNECTION_TIMEOUT:
    case PNR_TIMEOUT:
    case PNR_ABORTED:
    case PNR_HTTP_ERROR:
        return true;
    default:
        return false;
    }
}

bool pubnub_retry_configuration_retryable_(
    const pubnub_retry_configuration_t* configuration,
    const enum pubnub_trans             transaction,
    const int                           current_retry,
    const int                           http_status)
{
    const pubnub_endpoint_t endpoint =
        pubnub_endpoint_from_transaction_(transaction);

    if (configuration->excluded_endpoints[endpoint] ||
        current_retry >= configuration->max_retries)
        return false;

    return http_status == 429 || http_status >= 500 || http_status == 0;
}

size_t pubnub_retry_configuration_delay_(pubnub_t* pb)
{
    const pubnub_retry_configuration_t* configuration =
        pb->core.retry_configuration;
    const int current_retry = pb->core.http_retry_count;
    const int http_status   = pubnub_last_http_code(pb);
    const int retry_after   = pubnub_last_http_retry_header(pb);

    size_t delay = -1;
    if (!pubnub_retry_configuration_retryable_(configuration,
                                               pb->trans,
                                               current_retry,
                                               http_status))
        return delay;

    if (retry_after > 0 && 429 == http_status) { delay = retry_after; }
    else if (PUBNUB_LINEAR_RETRY_POLICY == configuration->policy)
        delay = configuration->minimum_delay;
    else {
        delay = (int)fmin(
            (double)(configuration->minimum_delay * pow(2, current_retry - 1)),
            (double)configuration->maximum_delay);
    }

    return delay * 1000 + random() % 1000;
}

void pubnub_retry_configuration_set_excluded_(
    pubnub_retry_configuration_t* configuration,
    const pubnub_endpoint_t       excluded_endpoint,
    va_list                       endpoints)
{
    int endpoint = excluded_endpoint;

    while (endpoint != -1) {
        if (endpoint >= 0 && endpoint < PNRC_ENDPOINTS_COUNT)
            configuration->excluded_endpoints[endpoint] = true;
        endpoint = va_arg(endpoints, int);
    }
}

pubnub_endpoint_t pubnub_endpoint_from_transaction_(
    const enum pubnub_trans trans)
{
    pubnub_endpoint_t endpoint;

    switch (trans) {
    case PBTT_SUBSCRIBE:
#if PUBNUB_USE_SUBSCRIBE_V2
    case PBTT_SUBSCRIBE_V2:
#endif // #if PUBNUB_USE_SUBSCRIBE_V2
        endpoint = PNRC_SUBSCRIBE_ENDPOINT;
        break;
    case PBTT_PUBLISH:
    case PBTT_SIGNAL:
        endpoint = PNRC_MESSAGE_SEND_ENDPOINT;
        break;
    case PBTT_WHERENOW:
    case PBTT_GLOBAL_HERENOW:
    case PBTT_HERENOW:
    case PBTT_LEAVE:
    case PBTT_HEARTBEAT:
    case PBTT_SET_STATE:
    case PBTT_STATE_GET:
        endpoint = PNRC_PRESENCE_ENDPOINT;
        break;
    case PBTT_HISTORY:
#if PUBNUB_USE_FETCH_HISTORY
    case PBTT_FETCH_HISTORY:
#endif // #if PUBNUB_USE_FETCH_HISTORY
#if PUBNUB_USE_ADVANCED_HISTORY
    case PBTT_MESSAGE_COUNTS:
#endif // #if PUBNUB_USE_ADVANCED_HISTORY
#if PUBNUB_USE_ACTIONS_API
    case PBTT_HISTORY_WITH_ACTIONS:
#endif // #if PUBNUB_USE_ACTIONS_API
        endpoint = PNRC_MESSAGE_STORAGE_ENDPOINT;
        break;
    case PBTT_REMOVE_CHANNEL_GROUP:
    case PBTT_REMOVE_CHANNEL_FROM_GROUP:
    case PBTT_ADD_CHANNEL_TO_GROUP:
    case PBTT_LIST_CHANNEL_GROUP:
        endpoint = PNRC_CHANNEL_GROUPS_ENDPOINT;
        break;
#if PUBNUB_USE_OBJECTS_API
    case PBTT_GETALL_UUIDMETADATA:
    case PBTT_SET_UUIDMETADATA:
    case PBTT_GET_UUIDMETADATA:
    case PBTT_DELETE_UUIDMETADATA:
    case PBTT_GETALL_CHANNELMETADATA:
    case PBTT_SET_CHANNELMETADATA:
    case PBTT_GET_CHANNELMETADATA:
    case PBTT_REMOVE_CHANNELMETADATA:
    case PBTT_GET_MEMBERSHIPS:
    case PBTT_SET_MEMBERSHIPS:
    case PBTT_REMOVE_MEMBERSHIPS:
    case PBTT_GET_MEMBERS:
    case PBTT_ADD_MEMBERS:
    case PBTT_SET_MEMBERS:
    case PBTT_REMOVE_MEMBERS:
        endpoint = PNRC_APP_CONTEXT_ENDPOINT;
        break;
#endif // #if PUBNUB_USE_OBJECTS_API
#if PUBNUB_USE_ACTIONS_API
    case PBTT_ADD_ACTION:
    case PBTT_REMOVE_ACTION:
    case PBTT_GET_ACTIONS:
        endpoint = PNRC_MESSAGE_REACTIONS_ENDPOINT;
        break;
#endif // #if PUBNUB_USE_ACTIONS_API
#if PUBNUB_USE_GRANT_TOKEN_API || PUBNUB_USE_REVOKE_TOKEN_API
#if PUBNUB_USE_GRANT_TOKEN_API
    case PBTT_GRANT_TOKEN:
#endif // #if PUBNUB_USE_GRANT_TOKEN_API
#if PUBNUB_USE_REVOKE_TOKEN_API
    case PBTT_REVOKE_TOKEN:
#endif // #if PUBNUB_USE_REVOKE_TOKEN_API
        endpoint = PNRC_PAM_ENDPOINT;
        break;
#endif // #if PUBNUB_USE_GRANT_TOKEN_API || PUBNUB_USE_REVOKE_TOKEN_API
    default:
        endpoint = PNRC_UNKNOWN_ENDPOINT;
        break;
    }

    return endpoint;
}