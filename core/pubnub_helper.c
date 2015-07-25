/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_helper.h"

#include "pubnub_assert.h"

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
