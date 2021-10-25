/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"

#if PUBNUB_USE_FETCH_HISTORY
#include "pubnub_memory_block.h"
#include "pubnub_server_limits.h"
#include "pubnub_fetch_history.h"
#include "pubnub_version.h"
#include "pubnub_json_parse.h"
#include "pubnub_url_encode.h"

#include "pubnub_assert.h"
#include "pubnub_log.h"
#else
#error this module can only be used if PUBNUB_USE_FETCH_HISTORY is defined and set to 1
#endif


#define PUBNUB_MIN_TIMETOKEN_LEN 4


