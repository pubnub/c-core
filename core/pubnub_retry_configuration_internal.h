/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#ifndef PUBNUB_RETRY_CONFIGURATION_INTERNAL_H
#define PUBNUB_RETRY_CONFIGURATION_INTERNAL_H


/**
 * @file  pubnub_retry_configuration_internal.h
 * @brief PubNub request retry configuration.
 */

#include "pubnub_retry_configuration.h"

#include <stdbool.h>
#include <stdlib.h>



// ----------------------------------------------
//                   Functions
// ----------------------------------------------

/**
 * @brief Check whether transaction and it outcome can be retried at all.
 *
 * Not all transactions can be handled and not all results makes sense to retry.
 *
 * @param pb Pointer to the PubNub context for which results of last transaction
 *           should be checked.
 * @return `true` in case if it really failed and it wasn't because of user.
 */
bool pubnub_retry_configuration_retryable_result_(pubnub_t* pb);

/**
 * @brief Check whether `transaction` can be retried after failure.
 *
 * @param pb Pointer to the PubNub context for which delay should be computed.
 * @return Next retry attempt delay value or \b -1 in case if retry is
 *         impossible.
 */
size_t pubnub_retry_configuration_delay_(pubnub_t* pb);
#endif //PUBNUB_RETRY_CONFIGURATION_INTERNAL_H
