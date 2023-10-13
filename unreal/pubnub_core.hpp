/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_CORE
#define INC_PUBNUB_CORE

extern "C" {

#include "../core/pubnub_api_types.h"
#include "../core/pubnub_assert.h"
#include "../core/pubnub_helper.h"
#include "../lib/pb_deprecated.h"
#include "../core/pubnub_alloc.h"
#include "../core/pubnub_pubsubapi.h"
#include "../core/pubnub_coreapi.h"
#include "../core/pubnub_coreapi_ex.h"
#include "../core/pubnub_generate_uuid.h"
#include "../core/pubnub_blocking_io.h"
#include "../core/pubnub_ssl.h"
#include "../core/pubnub_timers.h"
#include "../core/pubnub_helper.h"
#include "../core/pubnub_free_with_timeout.h"
#include "../core/pubnub_ntf_sync.h"
#if defined(PUBNUB_CALLBACK_API)
#include "../core/pubnub_ntf_callback.h"
#endif
#if PUBNUB_PROXY_API
#include "../core/pubnub_proxy.h"
#endif
#if PUBNUB_USE_SUBSCRIBE_V2
#include "../core/pubnub_subscribe_v2.h"
#endif
#if PUBNUB_CRYPTO_API
#include "../core/pubnub_crypto.h"
#endif
#if PUBNUB_USE_ADVANCED_HISTORY
#include "../core/pubnub_advanced_history.h"
#endif
#if PUBNUB_USE_FETCH_HISTORY
#include "../core/pubnub_fetch_history.h"
#endif
#if PUBNUB_USE_OBJECTS_API
#include "../core/pubnub_objects_api.h"
#define MAX_INCLUDE_DIMENSION 100
#define MAX_ELEM_LENGTH 30
#endif
#if PUBNUB_USE_ACTIONS_API
#include "../core/pubnub_actions_api.h"
#endif
#if PUBNUB_USE_GRANT_TOKEN_API
#include "../core/pubnub_grant_token_api.h"
#endif
#if PUBNUB_USE_REVOKE_TOKEN_API
#include "../core/pubnub_revoke_token_api.h"
#endif
#include "../core/pubnub_auto_heartbeat.h"

}

#endif // !defined INC_PUBNUB_CORE
