/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#ifndef PUBNUB_QT
#include "core/pbcc_set_state.h"
#include "core/pubnub_coreapi.h"
#endif
#include "core/pubnub_api_types.h"
#include "pubnub_internal.h"

#ifndef PUBNUB_QT
#include "pubnub_ccore.h"
#include "pubnub_netcore.h"
#include "pubnub_timers.h"
#endif
#include "pubnub_assert.h"
#include "pubnub_crypto.h"
#include "pubnub_server_limits.h"
#include "pubnub_coreapi_ex.h"

#include "pbpal.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>


struct pubnub_publish_options pubnub_publish_defopts(void)
{
    struct pubnub_publish_options result;
    result.store               = true;
    result.cipher_key          = NULL;
    result.replicate           = true;
    result.meta                = NULL;
    result.method              = pubnubSendViaGET;
    result.ttl                 = 0;
    result.custom_message_type = NULL;
    return result;
}

#ifndef PUBNUB_QT

enum pubnub_res pubnub_publish_ex(
    pubnub_t*                     pb,
    const char*                   channel,
    const char*                   message,
    struct pubnub_publish_options opts)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_ASSERT_OPT(message != NULL);

#if PUBNUB_LOG_ENABLED(DEBUG)
    if (pubnub_logger_should_log(pb, PUBNUB_LOG_LEVEL_DEBUG)) {
        char* method     = opts.method == pubnubSendViaGET ? "GET" : "POST";
        bool  compressed = opts.method == pubnubSendViaPOSTwithGZIP;
        pubnub_log_value_t data = pubnub_log_value_map_init();
        PUBNUB_LOG_MAP_SET_STRING(&data, channel)
        PUBNUB_LOG_MAP_SET_STRING(&data, message)
        PUBNUB_LOG_MAP_SET_BOOL(&data, opts.store, store)
        PUBNUB_LOG_MAP_SET_STRING(&data, opts.cipher_key, cipher_key)
        PUBNUB_LOG_MAP_SET_BOOL(&data, opts.replicate, replicate)
        PUBNUB_LOG_MAP_SET_STRING(&data, opts.meta, meta)
        PUBNUB_LOG_MAP_SET_STRING(&data, method)
        PUBNUB_LOG_MAP_SET_BOOL(&data, compressed)
        PUBNUB_LOG_MAP_SET_NUMBER(&data, opts.ttl, ttl)
        PUBNUB_LOG_MAP_SET_STRING(
            &data, opts.custom_message_type, custom_message_type)
        pubnub_log_object(
            pb,
            PUBNUB_LOG_LEVEL_DEBUG,
            PUBNUB_LOG_LOCATION,
            &data,
            "Publish with parameters:");
    }
#endif

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        PUBNUB_LOG_DEBUG(
            pb,
            "Unable to start transaction. PubNub context is in %s state.",
            pbcc_state_2_string(pb->state));
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }
#if PUBNUB_CRYPTO_API
    if (NULL != pb->core.crypto_module && NULL != opts.cipher_key) {
        PUBNUB_LOG_WARNING(pb, "'cipher_key' ignored. Using crypto module.");
    }
    else if (NULL != opts.cipher_key) {
        pubnub_bymebl_t to_encrypt;
        char*           encrypted_msg = pb->core.encrypted_msg_buf;
        size_t          n = sizeof pb->core.encrypted_msg_buf - sizeof("\"\"");

        to_encrypt.ptr   = (uint8_t*)message;
        to_encrypt.size  = strlen(message);
        encrypted_msg[0] = '"';
        if (0 != pubnub_encrypt(
                     opts.cipher_key, to_encrypt, encrypted_msg + 1, &n)) {
            return PNR_INTERNAL_ERROR;
        }
        encrypted_msg[++n] = '"';
        encrypted_msg[++n] = '\0';
        message            = encrypted_msg;
    }
#endif
#if PUBNUB_USE_GZIP_COMPRESSION
    if (pubnubSendViaPOSTwithGZIP == opts.method) {
        message = (pbgzip_compress(pb, message) == PNR_OK)
                      ? pb->core.gzip_msg_buf
                      : message;
    }
#endif
    rslt = pbcc_publish_prep(
        &pb->core,
        channel,
        message,
        opts.custom_message_type,
        opts.store,
        !opts.replicate,
        opts.meta,
        opts.ttl,
        opts.method);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_PUBLISH;
        pb->core.last_result = PNR_STARTED;
        pb->method           = opts.method;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}

struct pubnub_signal_options pubnub_signal_defopts(void)
{
    struct pubnub_signal_options result;
    result.custom_message_type = NULL;
    return result;
}

enum pubnub_res pubnub_signal_ex(
    pubnub_t*                    pb,
    const char*                  channel,
    const char*                  message,
    struct pubnub_signal_options opts)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

#if PUBNUB_LOG_ENABLED(DEBUG)
    if (pubnub_logger_should_log(pb, PUBNUB_LOG_LEVEL_DEBUG)) {
        pubnub_log_value_t data = pubnub_log_value_map_init();
        PUBNUB_LOG_MAP_SET_STRING(&data, channel)
        PUBNUB_LOG_MAP_SET_STRING(&data, message)
        PUBNUB_LOG_MAP_SET_STRING(
            &data, opts.custom_message_type, custom_message_type)
        pubnub_log_object(
            pb,
            PUBNUB_LOG_LEVEL_DEBUG,
            PUBNUB_LOG_LOCATION,
            &data,
            "Signal with parameters:");
    }
#endif

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        PUBNUB_LOG_DEBUG(
            pb,
            "Unable to start transaction. PubNub context is in %s state.",
            pbcc_state_2_string(pb->state));
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }

    rslt =
        pbcc_signal_prep(&pb->core, channel, message, opts.custom_message_type);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_SIGNAL;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}

struct pubnub_subscribe_options pubnub_subscribe_defopts(void)
{
    struct pubnub_subscribe_options result;
    result.channel_group = NULL;
    result.heartbeat     = PUBNUB_MINIMAL_HEARTBEAT_INTERVAL;
    return result;
}


enum pubnub_res pubnub_subscribe_ex(
    pubnub_t*                       p,
    const char*                     channel,
    struct pubnub_subscribe_options opt)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(p));

#if PUBNUB_LOG_ENABLED(DEBUG)
    if (pubnub_logger_should_log(p, PUBNUB_LOG_LEVEL_DEBUG)) {
        pubnub_log_value_t data = pubnub_log_value_map_init();
        PUBNUB_LOG_MAP_SET_STRING(&data, channel)
        PUBNUB_LOG_MAP_SET_STRING(&data, opt.channel_group, channel_group)
        PUBNUB_LOG_MAP_SET_NUMBER(&data, opt.heartbeat, heartbeat)
        pubnub_log_object(
            p,
            PUBNUB_LOG_LEVEL_DEBUG,
            PUBNUB_LOG_LOCATION,
            &data,
            "Subscribe with parameters:");
    }
#endif

    pubnub_mutex_lock(p->monitor);
    if (!pbnc_can_start_transaction(p)) {
        PUBNUB_LOG_DEBUG(
            p,
            "Unable to start transaction. PubNub context is in %s state.",
            pbcc_state_2_string(p->state));
        pubnub_mutex_unlock(p->monitor);
        return PNR_IN_PROGRESS;
    }
    rslt = pbauto_heartbeat_prepare_channels_and_ch_groups(
        p, &channel, &opt.channel_group);
    if (rslt != PNR_OK) { return rslt; }

    rslt = pbcc_subscribe_prep(
        &p->core, channel, opt.channel_group, &opt.heartbeat);
    if (PNR_STARTED == rslt) {
        p->trans            = PBTT_SUBSCRIBE;
        p->core.last_result = PNR_STARTED;
        pbnc_fsm(p);
        rslt = p->core.last_result;
    }

    pubnub_mutex_unlock(p->monitor);
    return rslt;
}


struct pubnub_here_now_options pubnub_here_now_defopts(void)
{
    struct pubnub_here_now_options result;
    result.channel_group = NULL;
    result.disable_uuids = false;
    result.state         = false;
    result.limit         = PUBNUB_DEFAULT_HERE_NOW_LIMIT;
    result.offset        = 0;
    return result;
}


enum pubnub_res pubnub_here_now_ex(
    pubnub_t*                      pb,
    const char*                    channel,
    struct pubnub_here_now_options opt)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

#if PUBNUB_LOG_ENABLED(DEBUG)
    if (pubnub_logger_should_log(pb, PUBNUB_LOG_LEVEL_DEBUG)) {
        pubnub_log_value_t data = pubnub_log_value_map_init();
        PUBNUB_LOG_MAP_SET_STRING(&data, channel)
        PUBNUB_LOG_MAP_SET_STRING(&data, opt.channel_group, channel_group)
        PUBNUB_LOG_MAP_SET_BOOL(&data, opt.disable_uuids, disable_uuids)
        PUBNUB_LOG_MAP_SET_BOOL(&data, opt.state, include_state)
        PUBNUB_LOG_MAP_SET_BOOL(&data, opt.limit, limit)
        PUBNUB_LOG_MAP_SET_BOOL(&data, opt.offset, offset)
        pubnub_log_object(
            pb,
            PUBNUB_LOG_LEVEL_DEBUG,
            PUBNUB_LOG_LOCATION,
            &data,
            "Here now with parameters:");
    }
#endif

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        PUBNUB_LOG_DEBUG(
            pb,
            "Unable to start transaction. PubNub context is in %s state.",
            pbcc_state_2_string(pb->state));
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }

    rslt = pbcc_here_now_prep(
        &pb->core,
        channel,
        opt.channel_group,
        opt.disable_uuids ? pbccTrue : pbccFalse,
        opt.state ? pbccTrue : pbccFalse,
        opt.limit,
        opt.offset);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_HERENOW;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }

    pubnub_mutex_unlock(pb->monitor);
    return rslt;
}


enum pubnub_res pubnub_global_here_now_ex(
    pubnub_t*                      pb,
    struct pubnub_here_now_options opt)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_ASSERT_OPT(NULL == opt.channel_group);

#if PUBNUB_LOG_ENABLED(DEBUG)
    if (pubnub_logger_should_log(pb, PUBNUB_LOG_LEVEL_DEBUG)) {
        pubnub_log_value_t data = pubnub_log_value_map_init();
        PUBNUB_LOG_MAP_SET_BOOL(&data, opt.disable_uuids, disable_uuids)
        PUBNUB_LOG_MAP_SET_BOOL(&data, opt.state, include_state)
        PUBNUB_LOG_MAP_SET_BOOL(&data, opt.limit, limit)
        PUBNUB_LOG_MAP_SET_BOOL(&data, opt.offset, offset)
        pubnub_log_object(
            pb,
            PUBNUB_LOG_LEVEL_DEBUG,
            PUBNUB_LOG_LOCATION,
            &data,
            "Global here now with parameters:");
    }
#endif

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        PUBNUB_LOG_DEBUG(
            pb,
            "Unable to start transaction. PubNub context is in %s state.",
            pbcc_state_2_string(pb->state));
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }

    rslt = pbcc_here_now_prep(
        &pb->core,
        NULL,
        NULL,
        opt.disable_uuids ? pbccTrue : pbccFalse,
        opt.state ? pbccTrue : pbccFalse,
        opt.limit,
        opt.offset);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_GLOBAL_HERENOW;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }

    pubnub_mutex_unlock(pb->monitor);
    return rslt;
}


struct pubnub_history_options pubnub_history_defopts(void)
{
    struct pubnub_history_options rslt;

    rslt.string_token  = false;
    rslt.count         = 100;
    rslt.reverse       = false;
    rslt.start         = NULL;
    rslt.end           = NULL;
    rslt.include_token = false;
    rslt.include_meta  = false;

    return rslt;
}


enum pubnub_res pubnub_history_ex(
    pubnub_t*                     pb,
    char const*                   channel,
    struct pubnub_history_options opt)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

#if PUBNUB_LOG_ENABLED(DEBUG)
    if (pubnub_logger_should_log(pb, PUBNUB_LOG_LEVEL_DEBUG)) {
        pubnub_log_value_t data = pubnub_log_value_map_init();
        PUBNUB_LOG_MAP_SET_STRING(&data, channel)
        PUBNUB_LOG_MAP_SET_STRING(&data, opt.start, start)
        PUBNUB_LOG_MAP_SET_STRING(&data, opt.end, end)
        PUBNUB_LOG_MAP_SET_BOOL(&data, opt.string_token, string_token)
        PUBNUB_LOG_MAP_SET_NUMBER(&data, opt.count, count)
        PUBNUB_LOG_MAP_SET_BOOL(&data, opt.reverse, reverse)
        PUBNUB_LOG_MAP_SET_BOOL(&data, opt.include_token, include_token)
        PUBNUB_LOG_MAP_SET_BOOL(&data, opt.include_meta, include_meta)
        pubnub_log_object(
            pb,
            PUBNUB_LOG_LEVEL_DEBUG,
            PUBNUB_LOG_LOCATION,
            &data,
            "History with parameters:");
    }
#endif

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        PUBNUB_LOG_DEBUG(
            pb,
            "Unable to start transaction. PubNub context is in %s state.",
            pbcc_state_2_string(pb->state));
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }

    rslt = pbcc_history_prep(
        &pb->core,
        channel,
        opt.count,
        opt.include_token,
        opt.string_token ? pbccTrue : pbccFalse,
        opt.reverse ? pbccTrue : pbccFalse,
        opt.include_meta ? pbccTrue : pbccFalse,
        opt.start,
        opt.end);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_HISTORY;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }

    pubnub_mutex_unlock(pb->monitor);
    return rslt;
}

struct pubnub_set_state_options pubnub_set_state_defopts(void)
{
    struct pubnub_set_state_options options;

    options.user_id       = NULL;
    options.channel_group = NULL;
    options.heartbeat     = false;

    return options;
}

static enum pubnub_res heartbeat_with_state(
    pubnub_t*                       pb,
    const char*                     channel,
    const char*                     state,
    struct pubnub_set_state_options opts)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

#if PUBNUB_LOG_ENABLED(DEBUG)
    if (pubnub_logger_should_log(pb, PUBNUB_LOG_LEVEL_DEBUG)) {
        pubnub_log_value_t data = pubnub_log_value_map_init();
        PUBNUB_LOG_MAP_SET_STRING(&data, channel)
        PUBNUB_LOG_MAP_SET_STRING(&data, opts.channel_group, channel_group)
        PUBNUB_LOG_MAP_SET_STRING(&data, opts.user_id, user_id)
        PUBNUB_LOG_MAP_SET_BOOL(&data, opts.heartbeat, heartbeat)
        pubnub_log_object(
            pb,
            PUBNUB_LOG_LEVEL_DEBUG,
            PUBNUB_LOG_LOCATION,
            &data,
            "Set state using heartbeat with parameters:");
    }
#endif

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        PUBNUB_LOG_DEBUG(
            pb,
            "Unable to start transaction. PubNub context is in %s state",
            pbcc_state_2_string(pb->state));
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }

    rslt = pbauto_heartbeat_prepare_channels_and_ch_groups(
        pb, &channel, &opts.channel_group);
    if (rslt != PNR_OK) { return rslt; }

    pbcc_adjust_state(&pb->core, channel, opts.channel_group, state);

    rslt = pbcc_heartbeat_prep(&pb->core, channel, opts.channel_group);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_HEARTBEAT;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }

    pubnub_mutex_unlock(pb->monitor);
    return rslt;
}


enum pubnub_res pubnub_set_state_ex(
    pubnub_t*                       p,
    const char*                     channel,
    const char*                     state,
    struct pubnub_set_state_options opts)
{
    return opts.heartbeat
               ? heartbeat_with_state(p, channel, state, opts)
               : pubnub_set_state(
                     p, channel, opts.channel_group, opts.user_id, state);
}

#endif /* PUBNUB_QT */
