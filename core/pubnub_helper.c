/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_helper.h"

#include "pubnub_assert.h"
#if PUBNUB_USE_SUBSCRIBE_V2
#include "pubnub_subscribe_v2_message.h"
#endif

#include <string.h>


enum pubnub_publish_res pubnub_parse_publish_result(char const *result)
{
    PUBNUB_ASSERT_OPT(result != NULL);

    if (strcmp(result, "\"Sent\"") == 0) {
        return PNPUB_SENT;
    }
    if (strcmp(result, "\"Invalid JSON\"") == 0) {
        return PNPUB_INVALID_JSON;
    }
    if (strncmp(result, "\"Invalid Character in Channel Name", 34) == 0) {
        return PNPUB_INVALID_CHAR_IN_CHAN_NAME;
    }
    if (strncmp(result, "\"Account quota exceeded", 23) == 0) {
        return PNPUB_ACCOUNT_QUOTA_EXCEEDED;
    }
    if (strcmp(result, "\"Message Too Large\"") == 0) {
        return PNPUB_MESSAGE_TOO_LARGE;
    }
    if (strcmp(result, "\"Invalid Key\"") == 0) {
        return PNPUB_INVALID_PUBLISH_KEY;
    }
    if (strncmp(result, "\"message\":", 10) == 0) {
        if (strncmp(result + 10, "\"Invalid Subscribe Key\"", 23) == 0) {
            return PNPUB_INVALID_SUBSCRIBE_KEY;
        }
        return PNPUB_UNKNOWN_JSON_OBJECT;
    }
    return PNPUB_UNKNOWN_ERROR;
}


char const* pubnub_res_2_string(enum pubnub_res e)
{
    switch (e) {
    case PNR_OK: return "OK";
    case PNR_ADDR_RESOLUTION_FAILED: return "Pubnub host name resolution failed";
    case PNR_WAIT_CONNECT_TIMEOUT: return "Time-out while waiting on TCP connection";
    case PNR_CONNECT_FAILED: return "Connecting to Pubnub server failed";
    case PNR_CONNECTION_TIMEOUT: return "A time-out happened in the network";
    case PNR_TIMEOUT: return "Timeout";
    case PNR_ABORTED: return "Aborted";
    case PNR_IO_ERROR: return "I/O (communication) error";
    case PNR_HTTP_ERROR: return "HTTP error received from server";
    case PNR_FORMAT_ERROR: return "Response format error";
    case PNR_CANCELLED: return "Pubnub API transaction cancelled";
    case PNR_STARTED: return "Pubnub API transaction started";
    case PNR_IN_PROGRESS: return "Pubnub API transaction already in progress";
    case PNR_RX_BUFF_NOT_EMPTY: return "Rx buffer not empty";
    case PNR_TX_BUFF_TOO_SMALL:  return "Tx buffer too small for sending/publishing the message";
    case PNR_INVALID_CHANNEL: return "Invalid channel";
    case PNR_PUBLISH_FAILED: return "Publish failed";
    case PNR_CHANNEL_REGISTRY_ERROR: return "A transaction related to channel registry failed";
    case PNR_REPLY_TOO_BIG: return "Reply from Pubnub too big to fit in buffer";
    case PNR_INTERNAL_ERROR: return "Internal error in processing";
    case PNR_CRYPTO_NOT_SUPPORTED: return "Encryption/decryption not supported";
    case PNR_BAD_COMPRESSION_FORMAT: return "Bad data compression format";
    case PNR_INVALID_PARAMETERS: return "Invalid function parameters";
    case PNR_ERROR_ON_SERVER: return "Server reported an error";
    case PNR_AUTHENTICATION_FAILED: return "Proxy authentication failed";
    case PNR_OBJECTS_API_INVALID_PARAM: return "Objects API invalid parameter";
    case PNR_OBJECTS_API_ERROR: return "Objects API transaction reported an error";
    case PNR_GOT_ALL_ACTIONS: return "Actions API got all actions";
    case PNR_ACTIONS_API_ERROR: return "Actions API transaction reported an error";
    default: return "!?!?!";
    }
}


char const* pbpal_resolv_n_connect_res_2_string(enum pbpal_resolv_n_connect_result e)
{
    switch (e) {
    case pbpal_resolv_resource_failure: return "pbpal_resolv_resource_failure";
    case pbpal_resolv_failed_send: return "pbpal_resolv_failed_send";
    case pbpal_resolv_send_wouldblock: return "pbpal_resolv_send_wouldblock";
    case pbpal_resolv_sent: return "pbpal_resolv_sent";
    case pbpal_resolv_failed_rcv: return "pbpal_resolv_failed_rcv";
    case pbpal_resolv_rcv_wouldblock: return "pbpal_resolv_rcv_wouldblock";
    case pbpal_resolv_failed_processing: return "pbpal_resolv_failed_processing";
    case pbpal_connect_resource_failure: return "pbpal_connect_resource_failure";
    case pbpal_connect_failed: return "pbpal_connect_failed";
    case pbpal_connect_wouldblock: return "pbpal_connect_wouldblock";
    case pbpal_connect_success: return "pbpal_connect_success";
    default: return "!?!?!";
    }
}


#if PUBNUB_USE_SUBSCRIBE_V2
char const* pubnub_msg_type_to_str(enum pubnub_message_type type)
{
    switch (type) {
    case pbsbSignal: return "signal";
    case pbsbPublished: return "published";
    default: return "!?!?!";
    }
}
#endif


enum pubnub_tribool pubnub_should_retry(enum pubnub_res e)
{
    switch (e) {
    case PNR_OK: return pbccFalse; /* Why? All is good! */
    case PNR_ADDR_RESOLUTION_FAILED: return pbccTrue;
    case PNR_WAIT_CONNECT_TIMEOUT: return pbccNotSet;
    case PNR_CONNECT_FAILED: return pbccTrue;
    case PNR_CONNECTION_TIMEOUT: return pbccTrue;
    case PNR_TIMEOUT: return pbccNotSet;
    case PNR_ABORTED: return pbccNotSet;
    case PNR_IO_ERROR: return pbccNotSet;
    case PNR_HTTP_ERROR: return pbccFalse; /* For all known HTTP erros it doesn't make sense */
    case PNR_FORMAT_ERROR: return pbccFalse; /* Fix the format */
    case PNR_CANCELLED: return pbccFalse; /* User cancelled, what's the use? */
    case PNR_STARTED: return pbccFalse; /* We haven't finished yet! */
    case PNR_IN_PROGRESS: return pbccFalse; /* We haven't finished yet! */
    case PNR_RX_BUFF_NOT_EMPTY: return pbccFalse; /* Clear the buffer first */
    case PNR_TX_BUFF_TOO_SMALL:  return pbccFalse; /* Publish shorter message */
    case PNR_INVALID_CHANNEL: return pbccFalse; /* Give the channel a valid name */
    case PNR_PUBLISH_FAILED: return pbccFalse; /* Fix the error reported */
    case PNR_CHANNEL_REGISTRY_ERROR: return pbccFalse; /* Fix the error reported */
    case PNR_REPLY_TOO_BIG: return pbccFalse; /* Rebuild with bigger buffer */
    case PNR_INTERNAL_ERROR: return pbccFalse; /* Sorry, something went wrong... */
    case PNR_CRYPTO_NOT_SUPPORTED: return pbccFalse; /* Use a platform that supports encryption, say OpenSSL */
    case PNR_BAD_COMPRESSION_FORMAT: return pbccNotSet; /* If bad compressing was transient, a retry might help */
    case PNR_INVALID_PARAMETERS: return pbccFalse; /* Check and fix invalid parameters */
    case PNR_ERROR_ON_SERVER: return pbccFalse; /* Fix the error reported */
    case PNR_AUTHENTICATION_FAILED: return pbccFalse; /* Check and fix the error reported */
    case PNR_OBJECTS_API_INVALID_PARAM: return pbccFalse; /* Check and fix the error reported */
    case PNR_OBJECTS_API_ERROR: return pbccFalse; /* Check the error reported */
    case PNR_GOT_ALL_ACTIONS: return pbccFalse; /* Successfully finished */
    case PNR_ACTIONS_API_ERROR: return pbccFalse; /* Check the error reported */
    }
    return pbccFalse;
}
