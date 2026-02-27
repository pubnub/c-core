/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"
#include "pubnub_internal_common.h"

#include "core/pubnub_pubsubapi.h"
#include "core/pubnub_ccore.h"
#include "core/pubnub_netcore.h"
#include "core/pubnub_assert.h"
#include "core/pubnub_timers.h"

#include "core/pbpal.h"
#include "pubnub_ntf_enforcement.h"

#include <ctype.h>
#include <string.h>

pubnub_t* pubnub_init(
    pubnub_t*   p,
    const char* publish_key,
    const char* subscribe_key)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(p));

    pubnub_mutex_init(p->monitor);
#if !defined(PUBNUB_CALLBACK_API) || defined(PUBNUB_NTF_RUNTIME_SELECTION)
    pubnub_mutex_init(p->cancel_monitor);
#endif
    pubnub_mutex_lock(p->monitor);
    pbcc_init(&p->core, publish_key, subscribe_key);
    if (PUBNUB_TIMERS_API) {
        p->transaction_timeout_ms  = PUBNUB_DEFAULT_TRANSACTION_TIMER;
        p->wait_connect_timeout_ms = PUBNUB_DEFAULT_WAIT_CONNECT_TIMER;
#if defined(PUBNUB_CALLBACK_API)
#if defined(PUBNUB_NTF_RUNTIME_SELECTION)
        if (PNA_CALLBACK == p->api_policy) { p->previous = p->next = NULL; }
#else
        p->previous = p->next = NULL;
#endif /* PUBNUB_NTF_RUNTIME_SELECTION */
#endif /* defined(PUBNUB_CALLBACK_API) */
    }
#if defined(PUBNUB_CALLBACK_API)
#if defined(PUBNUB_NTF_RUNTIME_SELECTION)
    if (PNA_CALLBACK == p->api_policy) {
        p->cb                 = NULL;
        p->user_data          = NULL;
        p->flags.sent_queries = 0;
    }
#else
    p->cb                 = NULL;
    p->user_data          = NULL;
    p->flags.sent_queries = 0;
#endif /* PUBNUB_NTF_RUNTIME_SELECTION */
#endif /* defined(PUBNUB_CALLBACK_API) */
    if (PUBNUB_ORIGIN_SETTABLE) {
        p->origin = PUBNUB_ORIGIN;
        p->port   = INITIAL_PORT_VALUE;
    }
#if PUBNUB_BLOCKING_IO_SETTABLE
#if defined(PUBNUB_CALLBACK_API)
#if defined(PUBNUB_NTF_RUNTIME_SELECTION)
    switch (p->api_policy) {
    case PNA_CALLBACK:
        p->options.use_blocking_io = false;
        break;
    case PNA_SYNC:
        p->options.use_blocking_io = true;
        break;
    }
#else
    p->options.use_blocking_io = false;
#endif /* PUBNUB_NTF_RUNTIME_SELECTION */
#else
    p->options.use_blocking_io = true;
#endif
#endif /* PUBNUB_BLOCKING_IO_SETTABLE */
#if PUBNUB_USE_AUTO_HEARTBEAT
    pubnub_mutex_init(p->thumper_monitor);
    p->should_announce_presence  = true;
    p->use_smart_heartbeat       = true;
    p->thumperIndex              = UNASSIGNED;
    p->channelInfo.channel       = NULL;
    p->channelInfo.channel_group = NULL;
#endif /* PUBNUB_AUTO_HEARTBEAT */

    p->state                       = PBS_IDLE;
    p->trans                       = PBTT_NONE;
    p->options.use_http_keep_alive = true;

    // Setting default TCP keep-alive options.
    p->options.tcp_keepalive.enabled  = pbccTrue;
    p->options.tcp_keepalive.time     = 60;
    p->options.tcp_keepalive.interval = 20;
    p->options.tcp_keepalive.probes   = 3;
#if !defined(PUBNUB_CALLBACK_API) || defined(PUBNUB_NTF_RUNTIME_SELECTION)
    p->should_stop_await = false;
#endif
#if PUBNUB_USE_IPV6
    /* IPv4 connectivity type by default. */
    p->options.ipv6_connectivity = false;
#endif
    p->flags.started_while_kept_alive = false;
    p->method                         = pubnubSendViaGET;
#if PUBNUB_ADVANCED_KEEP_ALIVE
    p->keep_alive.max     = 1000;
    p->keep_alive.timeout = 50;
#endif
    pbpal_init(p);
#if PUBNUB_PROXY_API
    p->proxy_type        = pbproxyNONE;
    p->proxy_hostname[0] = '\0';
#if defined(PUBNUB_CALLBACK_API)
#if defined(PUBNUB_NTF_RUNTIME_SELECTION)
    if (PNA_CALLBACK == p->api_policy) {
        memset(&(p->proxy_ipv4_address), 0, sizeof p->proxy_ipv4_address);
#if PUBNUB_USE_IPV6
        memset(&(p->proxy_ipv6_address), 0, sizeof p->proxy_ipv6_address);
#endif
    }
#else
    memset(&(p->proxy_ipv4_address), 0, sizeof p->proxy_ipv4_address);
#if PUBNUB_USE_IPV6
    memset(&(p->proxy_ipv6_address), 0, sizeof p->proxy_ipv6_address);
#endif
#endif /* PUBNUB_NTF_RUNTIME_SELECTION */
#endif /* defined(PUBNUB_CALLBACK_API) */
    p->proxy_tunnel_established = false;
    p->proxy_port               = 80;
    p->proxy_auth_scheme        = pbhtauNone;
    p->proxy_auth_username      = NULL;
    p->proxy_auth_password      = NULL;
    p->realm[0]                 = '\0';
#endif /* PUBNUB_PROXY_API */

#if PUBNUB_RECEIVE_GZIP_RESPONSE
    p->data_compressed = compressionNONE;
#endif
#if PUBNUB_CRYPTO_API
    p->core.crypto_module = NULL;
#endif
#if PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE
    p->core.subscribe_ee = pbcc_subscribe_ee_alloc(p);
#endif

#if PUBNUB_LOG_ENABLED(INFO)
    if (pubnub_logger_should_log(p, PUBNUB_LOG_LEVEL_INFO)) {
        pubnub_log_value_t config = pubnub_log_value_map_init();
        PUBNUB_LOG_MAP_SET_STRING(&config, publish_key);
        PUBNUB_LOG_MAP_SET_STRING(&config, subscribe_key);
#if PUBNUB_ORIGIN_SETTABLE
        PUBNUB_LOG_MAP_SET_STRING(&config, p->origin, origin);
        PUBNUB_LOG_MAP_SET_NUMBER(&config, p->port, port);
#endif // PUBNUB_ORIGIN_SETTABLE
        PUBNUB_LOG_MAP_SET_STRING(&config, p->core.id, clientId);
        PUBNUB_LOG_MAP_SET_NUMBER(
            &config, p->transaction_timeout_ms, transaction_timeout_ms);
        PUBNUB_LOG_MAP_SET_NUMBER(
            &config, p->wait_connect_timeout_ms, wait_connect_timeout_ms);
        PUBNUB_LOG_MAP_SET_BOOL(
            &config, p->options.use_blocking_io, use_blocking_io);
        PUBNUB_LOG_MAP_SET_BOOL(
            &config, p->options.use_http_keep_alive, use_http_keep_alive);
#if PUBNUB_ADVANCED_KEEP_ALIVE
        PUBNUB_LOG_MAP_SET_NUMBER(
            &config, p->keep_alive.timeout, keep_alive_timeout);
        PUBNUB_LOG_MAP_SET_NUMBER(
            &config, p->keep_alive.t_connect, keep_alive_t_connect);
        PUBNUB_LOG_MAP_SET_NUMBER(&config, p->keep_alive.max, keep_alive_max);
        PUBNUB_LOG_MAP_SET_NUMBER(
            &config, p->keep_alive.count, keep_alive_count);
#endif // PUBNUB_ADVANCED_KEEP_ALIVE
        const bool tcp_keep_alive_enabled =
            pbccTrue == p->options.tcp_keepalive.enabled;
        PUBNUB_LOG_MAP_SET_BOOL(&config, tcp_keep_alive_enabled);
        PUBNUB_LOG_MAP_SET_NUMBER(
            &config, p->options.tcp_keepalive.time, tcp_keep_alive_time);
        PUBNUB_LOG_MAP_SET_NUMBER(
            &config,
            p->options.tcp_keepalive.interval,
            tcp_keep_alive_interval);
        PUBNUB_LOG_MAP_SET_NUMBER(
            &config, p->options.tcp_keepalive.probes, tcp_keep_alive_probes);

#if PUBNUB_USE_SSL
        pubnub_log_value_t ssl = pubnub_log_value_bool(p->options.useSSL);
        pubnub_log_value_map_set(&config, "useSSL", &ssl);
#endif

#if defined(PUBNUB_CALLBACK_API) && defined(PUBNUB_NTF_RUNTIME_SELECTION)
        PUBNUB_LOG_MAP_SET_STRING(
            &config,
            PNA_CALLBACK == p->api_policy ? "callback" : "sync",
            enforced_api);
#endif // defined(PUBNUB_CALLBACK_API) && defined(PUBNUB_NTF_RUNTIME_SELECTION)

        pubnub_log_object(
            p,
            PUBNUB_LOG_LEVEL_INFO,
            PUBNUB_LOG_LOCATION,
            &config,
            "PubNub client initialized with configuration:");

        pubnub_log_value_t comp_flags = pubnub_log_value_array_init();
#if defined(PUBNUB_THREADSAFE)
        PUBNUB_LOG_ARRAY_APPEND_STRING(
            &comp_flags, "threadsafe", pubnub_threadsafe);
#endif // defined(PUBNUB_THREADSAFE)
#if defined(PUBNUB_CALLBACK_API)
        PUBNUB_LOG_ARRAY_APPEND_STRING(
            &comp_flags, "callback", pubnub_callback);
#endif // defined(PUBNUB_CALLBACK_API)
#if defined(PUBNUB_NTF_RUNTIME_SELECTION)
        PUBNUB_LOG_ARRAY_APPEND_STRING(
            &comp_flags, "runtime selection", pubnub_runtime_select);
#endif // defined(PUBNUB_NTF_RUNTIME_SELECTION)
#if PUBNUB_TIMERS_API
        PUBNUB_LOG_ARRAY_APPEND_STRING(&comp_flags, "timers", pubnub_timers);
#endif // PUBNUB_TIMERS_API
#if PUBNUB_USE_SYNC_API
        PUBNUB_LOG_ARRAY_APPEND_STRING(
            &comp_flags, "sync API", pubnub_sync_api);
#endif // PUBNUB_USE_SYNC_API
#if PUBNUB_PROXY_API
        PUBNUB_LOG_ARRAY_APPEND_STRING(
            &comp_flags, "proxy support", pubnub_proxy);
#endif // PUBNUB_PROXY_API
#if PUBNUB_ORIGIN_SETTABLE
        PUBNUB_LOG_ARRAY_APPEND_STRING(
            &comp_flags, "settable origin", pubnub_set_origin);
#endif // PUBNUB_ORIGIN_SETTABLE
#if PUBNUB_USE_MULTIPLE_ADDRESSES
        PUBNUB_LOG_ARRAY_APPEND_STRING(
            &comp_flags, "multiple addresses", pubnub_multiple_addresses);
#endif // PUBNUB_USE_MULTIPLE_ADDRESSES
#if PUBNUB_USE_SSL
        PUBNUB_LOG_ARRAY_APPEND_STRING(&comp_flags, "SSL", pubnub_ssl);
#endif // PUBNUB_USE_SSL
#if PUBNUB_CRYPTO_API
        PUBNUB_LOG_ARRAY_APPEND_STRING(
            &comp_flags, "crypto API", pubnub_crypto);
#endif // PUBNUB_CRYPTO_API
#if PUBNUB_RAND_INIT_VECTOR
        PUBNUB_LOG_ARRAY_APPEND_STRING(
            &comp_flags, "random initialization vector", pubnub_random_iv);
#endif // PUBNUB_RAND_INIT_VECTOR
#if PUBNUB_USE_IPV6
        PUBNUB_LOG_ARRAY_APPEND_STRING(&comp_flags, "IPv6", pubnub_use_ipv6);
#endif // PUBNUB_USE_IPV6
#if PUBNUB_ADVANCED_KEEP_ALIVE
        PUBNUB_LOG_ARRAY_APPEND_STRING(
            &comp_flags, "advanced keep-alive", pubnub_adv_ka);
#endif // PUBNUB_ADVANCED_KEEP_ALIVE
#if PUBNUB_SET_DNS_SERVERS
        PUBNUB_LOG_ARRAY_APPEND_STRING(
            &comp_flags, "settable DNS", pubnub_set_dns);
#endif // PUBNUB_SET_DNS_SERVERS
#if PUBNUB_USE_GZIP_COMPRESSION
        PUBNUB_LOG_ARRAY_APPEND_STRING(
            &comp_flags, "body GZIP compression", pubnub_gzip_compr);
#endif // PUBNUB_USE_GZIP_COMPRESSION
#if PUBNUB_RECEIVE_GZIP_RESPONSE
        PUBNUB_LOG_ARRAY_APPEND_STRING(
            &comp_flags, "GZIP compressed response", pubnub_gzip_response);
#endif // PUBNUB_RECEIVE_GZIP_RESPONSE
#if PUBNUB_USE_RETRY_CONFIGURATION
        PUBNUB_LOG_ARRAY_APPEND_STRING(
            &comp_flags, "retry configuration", pubnub_retry_configuration);
#endif // PUBNUB_USE_RETRY_CONFIGURATION
#if PUBNUB_USE_SUBSCRIBE_V2
        PUBNUB_LOG_ARRAY_APPEND_STRING(
            &comp_flags, "subscribe v2", pubnub_subscribe_v2);
#endif // PUBNUB_USE_SUBSCRIBE_V2
#if PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE
        PUBNUB_LOG_ARRAY_APPEND_STRING(
            &comp_flags, "new subscription flow", pubnub_new_subscribe_flow);
#endif // PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE
#if PUBNUB_USE_OBJECTS_API
        PUBNUB_LOG_ARRAY_APPEND_STRING(
            &comp_flags, "app context API", pubnub_app_context);
#endif // PUBNUB_USE_OBJECTS_API
#if PUBNUB_USE_ACTIONS_API
        PUBNUB_LOG_ARRAY_APPEND_STRING(
            &comp_flags, "message actions API", pubnub_message_actions);
#endif // PUBNUB_USE_ACTIONS_API
#if PUBNUB_USE_FETCH_HISTORY
        PUBNUB_LOG_ARRAY_APPEND_STRING(
            &comp_flags, "fetch history API", pubnub_fetch_history);
#endif // PUBNUB_USE_FETCH_HISTORY
#if PUBNUB_USE_ADVANCED_HISTORY
        PUBNUB_LOG_ARRAY_APPEND_STRING(
            &comp_flags, "advanced history API", pubnub_advanced_history);
#endif // PUBNUB_USE_ADVANCED_HISTORY
#if PUBNUB_USE_GRANT_TOKEN_API
        PUBNUB_LOG_ARRAY_APPEND_STRING(
            &comp_flags, "grant token API", pubnub_grant_token);
#endif // PUBNUB_USE_GRANT_TOKEN_API
#if PUBNUB_USE_REVOKE_TOKEN_API
        PUBNUB_LOG_ARRAY_APPEND_STRING(
            &comp_flags, "revoke token API", pubnub_revoke_token);
#endif // PUBNUB_USE_REVOKE_TOKEN_API
#if PUBNUB_USE_AUTO_HEARTBEAT
        PUBNUB_LOG_ARRAY_APPEND_STRING(
            &comp_flags, "auto heartbeat", pubnub_auto_heartbeat);
#endif // PUBNUB_USE_AUTO_HEARTBEAT

        pubnub_log_object(
            p,
            PUBNUB_LOG_LEVEL_DEBUG,
            PUBNUB_LOG_LOCATION,
            &comp_flags,
            "PubNub SDK built with:");
    }
#endif /* PUBNUB_LOG_ENABLED(INFO) */

    pubnub_mutex_unlock(p->monitor);

    return p;
}


enum pubnub_res pubnub_publish(
    pubnub_t*   pb,
    const char* channel,
    const char* message)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

#if PUBNUB_LOG_ENABLED(DEBUG)
    if (pubnub_logger_should_log(pb, PUBNUB_LOG_LEVEL_DEBUG)) {
        pubnub_log_value_t params = pubnub_log_value_map_init();
        PUBNUB_LOG_MAP_SET_STRING(&params, channel);
        PUBNUB_LOG_MAP_SET_STRING(&params, message);
        pubnub_log_object(
            pb,
            PUBNUB_LOG_LEVEL_DEBUG,
            PUBNUB_LOG_LOCATION,
            &params,
            "Publish with parameters:");
    }
#endif /* PUBNUB_LOG_ENABLED(DEBUG) */

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        PUBNUB_LOG_WARNING(
            pb,
            "Unable to start transaction. PubNub context is in %s state",
            pbcc_state_2_string(pb->state));
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }

    rslt = pbcc_publish_prep(
        &pb->core,
        channel,
        message,
        NULL,
        true,
        false,
        NULL,
        SIZE_MAX,
        pubnubSendViaGET);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_PUBLISH;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}


enum pubnub_res pubnub_signal(
    pubnub_t*   pb,
    const char* channel,
    const char* message)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

#if PUBNUB_LOG_ENABLED(DEBUG)
    if (pubnub_logger_should_log(pb, PUBNUB_LOG_LEVEL_DEBUG)) {
        pubnub_log_value_t params = pubnub_log_value_map_init();
        PUBNUB_LOG_MAP_SET_STRING(&params, channel);
        PUBNUB_LOG_MAP_SET_STRING(&params, message);
        pubnub_log_object(
            pb,
            PUBNUB_LOG_LEVEL_DEBUG,
            PUBNUB_LOG_LOCATION,
            &params,
            "Signal with parameters:");
    }
#endif /* PUBNUB_LOG_ENABLED(DEBUG) */

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        PUBNUB_LOG_WARNING(
            pb,
            "Unable to start transaction. PubNub context is in %s state",
            pbcc_state_2_string(pb->state));
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }

    rslt = pbcc_signal_prep(&pb->core, channel, message, NULL);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_SIGNAL;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}


char const* pubnub_get(pubnub_t* pb)
{
    char const* result;
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    result = pbcc_get_msg(&pb->core);
    pubnub_mutex_unlock(pb->monitor);

    return result;
}


char const* pubnub_get_channel(pubnub_t* pb)
{
    char const* result;
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    result = pbcc_get_channel(&pb->core);
    pubnub_mutex_unlock(pb->monitor);

    return result;
}


enum pubnub_res pubnub_subscribe(
    pubnub_t*   p,
    const char* channel,
    const char* channel_group)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(p));

#if PUBNUB_LOG_ENABLED(DEBUG)
    if (pubnub_logger_should_log(p, PUBNUB_LOG_LEVEL_DEBUG)) {
        pubnub_log_value_t params = pubnub_log_value_map_init();
        PUBNUB_LOG_MAP_SET_STRING(&params, channel);
        PUBNUB_LOG_MAP_SET_STRING(&params, channel_group);
        pubnub_log_object(
            p,
            PUBNUB_LOG_LEVEL_DEBUG,
            PUBNUB_LOG_LOCATION,
            &params,
            "Subscribe with parameters:");
    }
#endif /* PUBNUB_LOG_ENABLED(DEBUG) */


    pubnub_mutex_lock(p->monitor);
    if (!pbnc_can_start_transaction(p)) {
        PUBNUB_LOG_WARNING(
            p,
            "Unable to start transaction. PubNub context is in %s state",
            pbcc_state_2_string(p->state));
        pubnub_mutex_unlock(p->monitor);
        return PNR_IN_PROGRESS;
    }
    rslt = pbauto_heartbeat_prepare_channels_and_ch_groups(
        p, &channel, &channel_group);
    if (rslt != PNR_OK) { return rslt; }

    rslt = pbcc_subscribe_prep(&p->core, channel, channel_group, NULL);
    if (PNR_STARTED == rslt) {
        p->trans            = PBTT_SUBSCRIBE;
        p->core.last_result = PNR_STARTED;
        pbnc_fsm(p);
        rslt = p->core.last_result;
    }

    pubnub_mutex_unlock(p->monitor);
    return rslt;
}


enum pubnub_cancel_res pubnub_cancel(pubnub_t* pb)
{
    enum pubnub_cancel_res res = PN_CANCEL_STARTED;
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    PUBNUB_LOG_DEBUG(pb, "Cancel awaited transaction.");
#if !defined(PUBNUB_CALLBACK_API) || defined(PUBNUB_NTF_RUNTIME_SELECTION)
    pubnub_mutex_lock(pb->cancel_monitor);
    pb->should_stop_await = true;
    pubnub_mutex_unlock(pb->cancel_monitor);
#endif

    pubnub_mutex_lock(pb->monitor);
    pbnc_stop(pb, PNR_CANCELLED);
    if (PBS_IDLE == pb->state) { res = PN_CANCEL_FINISHED; }
    pubnub_mutex_unlock(pb->monitor);

    return res;
}


enum pubnub_res pubnub_set_uuid(pubnub_t* p, const char* uuid)
{
    return pubnub_set_user_id(p, uuid);
}

enum pubnub_res pubnub_set_user_id(pubnub_t* pb, const char* user_id)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_LOG_DEBUG(pb, "Set user_id: %s", user_id ? user_id : "(null)");
    pubnub_mutex_lock(pb->monitor);
    enum pubnub_res res = pbcc_set_user_id(&pb->core, user_id);
    pubnub_mutex_unlock(pb->monitor);

    return res;
}

char const* pubnub_uuid_get(pubnub_t* p)
{
    return pubnub_user_id_get(p);
}

char const* pubnub_user_id_get(pubnub_t* pb)
{
    char const* result;
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    result = pbcc_user_id_get(&pb->core);
    pubnub_mutex_unlock(pb->monitor);

    return result;
}


void pubnub_set_auth(pubnub_t* pb, const char* auth)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_LOG_DEBUG(pb, "Set auth: %s", auth ? auth : "(null)");
    pubnub_mutex_lock(pb->monitor);
    pbcc_set_auth(&pb->core, auth);
    pubnub_mutex_unlock(pb->monitor);
}

char const* pubnub_auth_get(pubnub_t* pb)
{
    char const* result;
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    result = pb->core.auth;
    pubnub_mutex_unlock(pb->monitor);

    return result;
}

void pubnub_set_auth_token(pubnub_t* pb, const char* token)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_LOG_DEBUG(pb, "Set auth token: %s", token ? token : "(null)");
    pubnub_mutex_lock(pb->monitor);
    pbcc_set_auth_token(&pb->core, token);
    pubnub_mutex_unlock(pb->monitor);
}

char const* pubnub_auth_token_get(pubnub_t* pb)
{
    char const* result;
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    result = pb->core.auth_token;
    pubnub_mutex_unlock(pb->monitor);

    return result;
}


void pubnub_set_sdk_version_suffix(pubnub_t* p, char const* suffix)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(p));
    pubnub_mutex_lock(p->monitor);
    p->core.sdk_version_suffix = suffix;
    pubnub_mutex_unlock(p->monitor);
}


int pubnub_last_http_code(pubnub_t* pb)
{
    int result;
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    pubnub_mutex_lock(pb->monitor);
    result = pb->http_code;
    pubnub_mutex_unlock(pb->monitor);
    return result;
}

#if PUBNUB_USE_RETRY_CONFIGURATION
uint16_t pubnub_last_http_retry_header(pubnub_t* pb)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    pubnub_mutex_lock(pb->monitor);
    uint16_t retry_after = pb->http_header_retry_after;
    pubnub_mutex_unlock(pb->monitor);
    return retry_after;
}
#endif // #if PUBNUB_USE_RETRY_CONFIGURATION

char const* pubnub_last_time_token(pubnub_t* pb)
{
    char const* result;
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    result = pb->core.timetoken;
    pubnub_mutex_unlock(pb->monitor);

    return result;
}


static char const* do_last_publish_timetoken(pubnub_t* pb)
{
    char*       ptr;
    char const* end_of_buffer;

    if (PUBNUB_DYNAMIC_REPLY_BUFFER && (NULL == pb->core.http_reply)) {
        return "0";
    }
    if (((pb->trans != PBTT_PUBLISH) && (pb->trans != PBTT_SIGNAL)) ||
        (pb->core.http_reply[0] == '\0')) {
        return "0";
    }
    if (pb->core.http_reply[0] != '[') { return "0"; }

    ptr           = pb->core.http_reply + 1;
    end_of_buffer = pb->core.http_reply + pb->core.http_buf_len;

    /* Find the end of the first part (the status code) */
    char* next_part = (char*)memchr(ptr, '\0', end_of_buffer - ptr);
    if (NULL == next_part || next_part >= end_of_buffer) { return "0"; }
    ptr = next_part + 1;

    /* Find the end of the second part (the message) */
    next_part = (char*)memchr(ptr, '\0', end_of_buffer - ptr);
    if (NULL == next_part || next_part >= end_of_buffer) { return "0"; }
    ptr = next_part + 1;

    /* Now ptr points to the timetoken which is a string like
       "\"17298458222356632\"" We should remove the quotes. */
    if (*ptr == '"') {
        char* end_of_token;
        ptr++; /* skip opening quote */
        end_of_token = strrchr(ptr, '"');
        if (end_of_token != NULL) { *end_of_token = '\0'; }
    }

    return ptr;
}


static char const* do_last_publish_result(pubnub_t* pb)
{
    char* end;

    if (PUBNUB_DYNAMIC_REPLY_BUFFER && (NULL == pb->core.http_reply)) {
        return "";
    }
    if (((pb->trans != PBTT_PUBLISH) && (pb->trans != PBTT_SIGNAL)) ||
        (pb->core.http_reply[0] == '\0')) {
        return "";
    }

    switch (pb->core.http_reply[0]) {
    case '[':
        for (end = pb->core.http_reply + 1; isdigit((unsigned)*end); ++end) {
            continue;
        }
        return end + 1;
    case '{':
        return pb->core.http_reply + 1;
    default:
        return "";
    }
}


char const* pubnub_last_publish_result(pubnub_t* pb)
{
    char const* rslt;
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    rslt = do_last_publish_result(pb);
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}


char const* pubnub_last_publish_timetoken(pubnub_t* pb)
{
    char const* rslt;
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    rslt = do_last_publish_timetoken(pb);
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}


char const* pubnub_get_origin(pubnub_t* pb)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    if (PUBNUB_ORIGIN_SETTABLE) {
        char const* result;

        pubnub_mutex_lock(pb->monitor);
        result = pb->origin;
        pubnub_mutex_unlock(pb->monitor);

        return result;
    }
    return PUBNUB_ORIGIN;
}


int pubnub_origin_set(pubnub_t* pb, char const* origin)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_LOG_DEBUG(pb, "Set origin: %s", origin ? origin : PUBNUB_ORIGIN);
    if (PUBNUB_ORIGIN_SETTABLE) {
        bool origin_set = false;
        if (NULL == origin) { origin = PUBNUB_ORIGIN; }

        pubnub_mutex_lock(pb->monitor);
#if PUBNUB_USE_MULTIPLE_ADDRESSES
        pbpal_multiple_addresses_reset_counters(&pb->spare_addresses);
#endif
        pb->origin = origin;
        origin_set = (PBS_IDLE == pb->state);
        pubnub_mutex_unlock(pb->monitor);

        return origin_set ? 0 : +1;
    }
    return -1;
}

int pubnub_port_set(pubnub_t* pb, uint16_t port)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_LOG_DEBUG(pb, "Set port: %u", (unsigned)port);
    if (PUBNUB_ORIGIN_SETTABLE) {
        pubnub_mutex_lock(pb->monitor);
        pb->port      = port;
        bool port_set = (PBS_IDLE == pb->state);
        pubnub_mutex_unlock(pb->monitor);

        return port_set ? 0 : +1;
    }
    return -1;
}


void pubnub_use_http_keep_alive(pubnub_t* p)
{
    PUBNUB_LOG_DEBUG(p, "Enable HTTP Keep-Alive.");
    p->options.use_http_keep_alive = 1;
}


void pubnub_dont_use_http_keep_alive(pubnub_t* p)
{
    PUBNUB_LOG_DEBUG(p, "Disable HTTP Keep-Alive.");
    p->options.use_http_keep_alive = 0;
}

void pubnub_use_tcp_keep_alive(
    pubnub_t*     pb,
    const uint8_t time,
    const uint8_t interval,
    const uint8_t probes)
{
#if PUBNUB_LOG_ENABLED(DEBUG)
    if (pubnub_logger_should_log(pb, PUBNUB_LOG_LEVEL_DEBUG)) {
        pubnub_log_value_t data = pubnub_log_value_map_init();
        PUBNUB_LOG_MAP_SET_NUMBER(&data, time, idle_time)
        PUBNUB_LOG_MAP_SET_NUMBER(&data, interval);
        PUBNUB_LOG_MAP_SET_NUMBER(&data, probes);
        pubnub_log_object(
            pb,
            PUBNUB_LOG_LEVEL_DEBUG,
            PUBNUB_LOG_LOCATION,
            &data,
            "Enable TCP Keep-Alive with parameters:");
    }
#endif

    pb->options.tcp_keepalive.enabled  = pbccTrue;
    pb->options.tcp_keepalive.time     = time;
    pb->options.tcp_keepalive.interval = interval;
    pb->options.tcp_keepalive.probes   = probes;
}

void pubnub_dont_use_tcp_keep_alive(pubnub_t* pb)
{
    PUBNUB_LOG_DEBUG(pb, "Disable TCP Keep-Alive.");
    pb->options.tcp_keepalive.enabled = pbccFalse;
}