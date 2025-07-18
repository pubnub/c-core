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

pubnub_t* pubnub_init(pubnub_t* p, const char* publish_key, const char* subscribe_key)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(p));

    pubnub_mutex_init(p->monitor);
#if !defined(PUBNUB_CALLBACK_API) || defined(PUBNUB_NTF_RUNTIME_SELECTION)
    pubnub_mutex_init(p->cancel_monitor);
#endif
    pubnub_mutex_lock(p->monitor);
    pbcc_init(&p->core, publish_key, subscribe_key);
    if (PUBNUB_TIMERS_API) {
        p->transaction_timeout_ms = PUBNUB_DEFAULT_TRANSACTION_TIMER;
        p->wait_connect_timeout_ms = PUBNUB_DEFAULT_WAIT_CONNECT_TIMER;
#if defined(PUBNUB_CALLBACK_API)
#if defined(PUBNUB_NTF_RUNTIME_SELECTION)
        if (PNA_CALLBACK == p->api_policy) {
            p->previous = p->next = NULL;
        }
#else 
        p->previous = p->next = NULL;
#endif /* PUBNUB_NTF_RUNTIME_SELECTION */
#endif /* defined(PUBNUB_CALLBACK_API) */
    }
#if defined(PUBNUB_CALLBACK_API)
#if defined(PUBNUB_NTF_RUNTIME_SELECTION)
    if (PNA_CALLBACK == p->api_policy) {
        p->cb = NULL;
        p->user_data = NULL;
        p->flags.sent_queries = 0;
    }
#else
    p->cb        = NULL;
    p->user_data = NULL;
    p->flags.sent_queries = 0;
#endif /* PUBNUB_NTF_RUNTIME_SELECTION */
#endif /* defined(PUBNUB_CALLBACK_API) */
    if (PUBNUB_ORIGIN_SETTABLE) {
        p->origin = PUBNUB_ORIGIN;
        p->port = INITIAL_PORT_VALUE;
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
    p->should_announce_presence = true;
    p->use_smart_heartbeat = true;
    p->thumperIndex = UNASSIGNED;
    p->channelInfo.channel = NULL;
    p->channelInfo.channel_group = NULL;
#endif /* PUBNUB_AUTO_HEARTBEAT */

    p->state                          = PBS_IDLE;
    p->trans                          = PBTT_NONE;
    p->options.use_http_keep_alive    = true;
#if !defined(PUBNUB_CALLBACK_API) || defined(PUBNUB_NTF_RUNTIME_SELECTION)
    p->should_stop_await              = false;
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
    pubnub_mutex_unlock(p->monitor);

    return p;
}


enum pubnub_res pubnub_publish(pubnub_t* pb, const char* channel, const char* message)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }

    rslt = pbcc_publish_prep(
        &pb->core, channel, message, NULL, true, false, NULL, SIZE_MAX, pubnubSendViaGET);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_PUBLISH;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}


enum pubnub_res pubnub_signal(pubnub_t* pb,
                              const char* channel,
                              const char* message)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
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


enum pubnub_res pubnub_subscribe(pubnub_t*   p,
                                 const char* channel,
                                 const char* channel_group)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(p));

    pubnub_mutex_lock(p->monitor);
    if (!pbnc_can_start_transaction(p)) {
        pubnub_mutex_unlock(p->monitor);
        return PNR_IN_PROGRESS;
    }
    rslt = pbauto_heartbeat_prepare_channels_and_ch_groups(p, &channel, &channel_group);
    if (rslt != PNR_OK) {
        return rslt;
    }

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

#if !defined(PUBNUB_CALLBACK_API) || defined(PUBNUB_NTF_RUNTIME_SELECTION)
    pubnub_mutex_lock(pb->cancel_monitor);
    pb->should_stop_await = true;
    pubnub_mutex_unlock(pb->cancel_monitor);
#endif

    pubnub_mutex_lock(pb->monitor);
    pbnc_stop(pb, PNR_CANCELLED);
    if (PBS_IDLE == pb->state) {
        res = PN_CANCEL_FINISHED;
    }
    pubnub_mutex_unlock(pb->monitor);

    return res;
}


enum pubnub_res pubnub_set_uuid(pubnub_t* p, const char* uuid) {
    return pubnub_set_user_id(p, uuid);
}

enum pubnub_res pubnub_set_user_id(pubnub_t* pb, const char* user_id)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    pubnub_mutex_lock(pb->monitor);
    enum pubnub_res res = pbcc_set_user_id(&pb->core, user_id);
    pubnub_mutex_unlock(pb->monitor);

    return res;
}

char const* pubnub_uuid_get(pubnub_t* p) {
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
    char* ptr;
    char const* end_of_buffer;

    if (PUBNUB_DYNAMIC_REPLY_BUFFER && (NULL == pb->core.http_reply)) {
        return "0";
    }
    if (((pb->trans != PBTT_PUBLISH) && (pb->trans != PBTT_SIGNAL)) ||
        (pb->core.http_reply[0] == '\0')) {
        return "0";
    }
    if (pb->core.http_reply[0] != '[') {
        return "0";
    }

    ptr = pb->core.http_reply + 1;
    end_of_buffer = pb->core.http_reply + pb->core.http_buf_len;

    /* Find the end of the first part (the status code) */
    char* next_part = (char*)memchr(ptr, '\0', end_of_buffer - ptr);
    if (NULL == next_part || next_part >= end_of_buffer) {
        return "0";
    }
    ptr = next_part + 1;

    /* Find the end of the second part (the message) */
    next_part = (char*)memchr(ptr, '\0', end_of_buffer - ptr);
    if (NULL == next_part || next_part >= end_of_buffer) {
        return "0";
    }
    ptr = next_part + 1;

    /* Now ptr points to the timetoken which is a string like "\"17298458222356632\""
       We should remove the quotes. */
    if (*ptr == '"') {
        char* end_of_token;
        ptr++; /* skip opening quote */
        end_of_token = strrchr(ptr, '"');
        if (end_of_token != NULL) {
            *end_of_token = '\0';
        }
    }

    return ptr;
}


static char const* do_last_publish_result(pubnub_t* pb)
{
    char* end;

    if (PUBNUB_DYNAMIC_REPLY_BUFFER && (NULL == pb->core.http_reply)) {
        return "";
    }
    if (((pb->trans != PBTT_PUBLISH) && (pb->trans != PBTT_SIGNAL))  ||
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
    if (PUBNUB_ORIGIN_SETTABLE) {
        bool origin_set = false;
        if (NULL == origin) {
            origin = PUBNUB_ORIGIN;
        }

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
    if (PUBNUB_ORIGIN_SETTABLE) {
        pubnub_mutex_lock(pb->monitor);
        pb->port = port;
        bool port_set = (PBS_IDLE == pb->state);
        pubnub_mutex_unlock(pb->monitor);

        return port_set ? 0 : +1;
    }
    return -1;
}


void pubnub_use_http_keep_alive(pubnub_t* p)
{
    p->options.use_http_keep_alive = 1;
}


void pubnub_dont_use_http_keep_alive(pubnub_t* p)
{
    p->options.use_http_keep_alive = 0;
}
