/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_coreapi_ex.h"

#include "pubnub_ccore.h"
#include "pubnub_netcore.h"
#include "pubnub_internal.h"
#include "pubnub_assert.h"
#include "pubnub_timers.h"
#include "pubnub_crypto.h"

#include "pbpal.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>


struct pubnub_publish_options pubnub_publish_defopts(void)
{
    struct pubnub_publish_options result;
    result.store      = true;
    result.cipher_key = NULL;
    result.replicate  = true;
    result.meta       = NULL;
    result.method     = pubnubPublishViaGET;
    return result;
}

enum pubnub_res pubnub_publish_ex(pubnub_t*                     pb,
                                  const char*                   channel,
                                  const char*                   message,
                                  struct pubnub_publish_options opts)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_ASSERT_OPT(message != NULL);

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }
#if PUBNUB_CRYPTO_API
    if (NULL != opts.cipher_key) {
        pubnub_bymebl_t to_encrypt;
        char* encrypted_msg = pb->core.encrypted_msg_buf;
        size_t          n  = sizeof pb->core.encrypted_msg_buf - sizeof("\"\"");

        to_encrypt.ptr   = (uint8_t*)message;
        to_encrypt.size  = strlen(message);
        encrypted_msg[0] = '"';
        if (0 != pubnub_encrypt(opts.cipher_key, to_encrypt, encrypted_msg + 1, &n)) {
            return PNR_INTERNAL_ERROR;
        }
        encrypted_msg[++n] = '"';
        encrypted_msg[++n] = '\0';
        message            = encrypted_msg;
    }
#endif
#if PUBNUB_USE_GZIP_COMPRESSION
    pb->core.gzip_msg_len = 0;
    if (pubnubPublishViaPOSTwithGZIP == opts.method) {
        if (pbgzip_compress(pb, message) == PNR_OK) {
            message = pb->core.gzip_msg_buf;
        }
    }
#endif
    rslt = pbcc_publish_prep(
        &pb->core, channel, message, opts.store, !opts.replicate, opts.meta, opts.method);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_PUBLISH;
        pb->core.last_result = PNR_STARTED;
        pb->flags.is_publish_via_post = (opts.method != pubnubPublishViaGET);
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}


/** Minimal presence heartbeat interval supported by
    Pubnub, in seconds.
*/
#define PUBNUB_MINIMAL_HEARTBEAT_INTERVAL 270


struct pubnub_subscribe_options pubnub_subscribe_defopts(void)
{
    struct pubnub_subscribe_options result;
    result.channel_group = NULL;
    result.heartbeat     = PUBNUB_MINIMAL_HEARTBEAT_INTERVAL;
    return result;
}


enum pubnub_res pubnub_subscribe_ex(pubnub_t*                       p,
                                    const char*                     channel,
                                    struct pubnub_subscribe_options opt)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(p));

    pubnub_mutex_lock(p->monitor);
    if (!pbnc_can_start_transaction(p)) {
        pubnub_mutex_unlock(p->monitor);
        return PNR_IN_PROGRESS;
    }

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
    return result;
}


enum pubnub_res pubnub_here_now_ex(pubnub_t*                      pb,
                                   const char*                    channel,
                                   struct pubnub_here_now_options opt)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }

    rslt = pbcc_here_now_prep(&pb->core,
                              channel,
                              opt.channel_group,
                              opt.disable_uuids ? pbccTrue : pbccFalse,
                              opt.state ? pbccTrue : pbccFalse);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_HERENOW;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }

    pubnub_mutex_unlock(pb->monitor);
    return rslt;
}


enum pubnub_res pubnub_global_here_now_ex(pubnub_t*                      pb,
                                          struct pubnub_here_now_options opt)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_ASSERT_OPT(NULL == opt.channel_group);

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }

    rslt = pbcc_here_now_prep(&pb->core,
                              NULL,
                              NULL,
                              opt.disable_uuids ? pbccTrue : pbccFalse,
                              opt.state ? pbccTrue : pbccFalse);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_HERENOW;
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

    return rslt;
}


enum pubnub_res pubnub_history_ex(pubnub_t*                     pb,
                                  char const*                   channel,
                                  struct pubnub_history_options opt)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }

    rslt = pbcc_history_prep(&pb->core,
                             channel,
                             opt.count,
                             opt.include_token,
                             opt.string_token ? pbccTrue : pbccFalse,
                             opt.reverse ? pbccTrue : pbccFalse,
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
