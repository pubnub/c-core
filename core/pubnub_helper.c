/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_helper.h"

#include "pubnub_assert.h"

//#include <stdlib.h>
#include <string.h>


enum pubnub_publish_res pubnub_parse_publish_result(char const *result)
{
    PUBNUB_ASSERT_OPT(result != NULL);

    if (strcmp(result, "Sent") == 0) {
        return PNPUB_SENT;
    }
    if (strcmp(result, "Invalid JSON") == 0) {
        return PNPUB_INVALID_JSON;
    }
    if (strncmp(result, "Invalid Character in Channel Name", 33) == 0) {
        return PNPUB_INVALID_CHAR_IN_CHAN_NAME;
    }
    return PNPUB_UNKOWN_ERROR;
}


char const* pubnub_res_2_string(enum pubnub_res e)
{
    switch (e) {
    case PNR_OK: return "OK";
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
    default: return "!?!?!";
    }
}
