/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "core/pubnub_pubsubapi.h"

#include "core/pubnub_ccore.h"
#include "core/pubnub_netcore.h"
#include "pubnub_internal.h"
#include "core/pubnub_assert.h"
#include "core/pubnub_timers.h"

#include "core/pbpal.h"

#include <ctype.h>
#include <string.h>

pubnub_t* pubnub_init(pubnub_t* p, const char* publish_key, const char* subscribe_key)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(p));

    pubnub_mutex_init(p->monitor);
    pubnub_mutex_lock(p->monitor);
    pbcc_init(&p->core, publish_key, subscribe_key);
    if (PUBNUB_TIMERS_API) {
        p->transaction_timeout_ms = PUBNUB_DEFAULT_TRANSACTION_TIMER;
#if defined(PUBNUB_CALLBACK_API)
        p->previous = p->next = NULL;
#endif
    }
#if defined(PUBNUB_CALLBACK_API)
    p->cb        = NULL;
    p->user_data = NULL;
#endif
    if (PUBNUB_ORIGIN_SETTABLE) {
        p->origin = PUBNUB_ORIGIN;
    }
#if PUBNUB_BLOCKING_IO_SETTABLE
#if defined(PUBNUB_CALLBACK_API)
    p->options.use_blocking_io = false;
#else
    p->options.use_blocking_io = true;
#endif
#endif /* PUBNUB_BLOCKING_IO_SETTABLE */

    p->state                       = PBS_IDLE;
    p->trans                       = PBTT_NONE;
    p->options.use_http_keep_alive = 1;
    p->flags.started_while_kept_alive = false;
#if PUBNUB_ADVANCED_KEEP_ALIVE
    p->keep_alive.max     = 1000;
    p->keep_alive.timeout = 50;
#endif
    pbpal_init(p);
    pubnub_mutex_unlock(p->monitor);

#if PUBNUB_PROXY_API
    p->proxy_type               = pbproxyNONE;
    p->proxy_hostname[0]        = '\0';
    memset(&(p->proxy_ip_address), 0, sizeof p->proxy_ip_address);
    p->proxy_tunnel_established = false;
    p->proxy_port               = 80;
    p->proxy_auth_scheme        = pbhtauNone;
    p->proxy_auth_username      = NULL;
    p->proxy_auth_password      = NULL;
    p->proxy_authorization_sent = false;
#endif

#if PUBNUB_RECEIVE_GZIP_RESPONSE
    p->data_compressed = compressionNONE;
#endif

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

    rslt = pbcc_publish_prep(&pb->core, channel, message, true, false, NULL);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_PUBLISH;
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

    rslt = pbcc_subscribe_prep(&p->core, channel, channel_group, NULL, NULL);
    if (PNR_STARTED == rslt) {
        p->trans            = PBTT_SUBSCRIBE;
        p->core.last_result = PNR_STARTED;
        pbnc_fsm(p);
        rslt = p->core.last_result;
    }

    pubnub_mutex_unlock(p->monitor);
    return rslt;
}


void pubnub_cancel(pubnub_t* pb)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    pbnc_stop(pb, PNR_CANCELLED);
    pubnub_mutex_unlock(pb->monitor);
}


void pubnub_set_uuid(pubnub_t* pb, const char* uuid)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    pubnub_mutex_lock(pb->monitor);
    pbcc_set_uuid(&pb->core, uuid);
    pubnub_mutex_unlock(pb->monitor);
}


char const* pubnub_uuid_get(pubnub_t* pb)
{
    char const* result;
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    result = pb->core.uuid;
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


int pubnub_last_http_code(pubnub_t* pb)
{
    int result;
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    pubnub_mutex_lock(pb->monitor);
    result = pb->http_code;
    pubnub_mutex_unlock(pb->monitor);
    return result;
}


char const* pubnub_last_time_token(pubnub_t* pb)
{
    char const* result;
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    result = pb->core.timetoken;
    pubnub_mutex_unlock(pb->monitor);

    return result;
}


static char const* do_last_publish_result(pubnub_t* pb)
{
    char* end;

    if (PUBNUB_DYNAMIC_REPLY_BUFFER && (NULL == pb->core.http_reply)) {
        return "";
    }
    if ((pb->trans != PBTT_PUBLISH) || (pb->core.http_reply[0] == '\0')) {
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
        if (NULL == origin) {
            origin = PUBNUB_ORIGIN;
        }

        pubnub_mutex_lock(pb->monitor);
        pb->origin = origin;
        pubnub_mutex_unlock(pb->monitor);

        return 0;
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
