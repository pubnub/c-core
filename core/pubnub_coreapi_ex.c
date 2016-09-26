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
    result.store = true;
    result.cipher_key = NULL;
    return result;
}


enum pubnub_res pubnub_publish_ex(pubnub_t *pb, const char *channel, const char *message, struct pubnub_publish_options opts)
{
    enum pubnub_res rslt;
    char encrypted_msg[PUBNUB_BUF_MAXLEN];

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_ASSERT_OPT(message != NULL);

    if (PUBNUB_CRYPTO_API && (NULL != opts.cipher_key)) {
        pubnub_bymebl_t to_encrypt;
        size_t n = PUBNUB_BUF_MAXLEN - 2;

        to_encrypt.ptr = (uint8_t*)message;
        to_encrypt.size = strlen(message);
        encrypted_msg[0] = '"';
        if (0 != pubnub_encrypt(opts.cipher_key, to_encrypt, encrypted_msg + 1, &n)) {
            return PNR_INTERNAL_ERROR;
        }
        ++n;
        encrypted_msg[n++] = '"';
        encrypted_msg[n] = '\0';
        message = encrypted_msg;
    }

    pubnub_mutex_lock(pb->monitor);
    if (pb->state != PBS_IDLE) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }

    rslt = pbcc_publish_prep(&pb->core, channel, message, opts.store, false);
    if (PNR_STARTED == rslt) {
        pb->trans = PBTT_PUBLISH;
        pb->core.last_result = PNR_STARTED;
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
    result.heartbeat = PUBNUB_MINIMAL_HEARTBEAT_INTERVAL;
    return result;
}


enum pubnub_res pubnub_subscribe_ex(pubnub_t *p, const char *channel, struct pubnub_subscribe_options opt)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(p));
    
    pubnub_mutex_lock(p->monitor);
    if (p->state != PBS_IDLE) {
        pubnub_mutex_unlock(p->monitor);
        return PNR_IN_PROGRESS;
    }
    
    rslt = pbcc_subscribe_prep(&p->core, channel, opt.channel_group, &opt.heartbeat);
    if (PNR_STARTED == rslt) {
        p->trans = PBTT_SUBSCRIBE;
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
    result.state = false;
    return result;
}


enum pubnub_res pubnub_here_now_ex(pubnub_t *pb, const char *channel, struct pubnub_here_now_options opt)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (pb->state != PBS_IDLE) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }
    
    rslt = pbcc_here_now_prep(
        &pb->core, 
        channel, 
        opt.channel_group,
        opt.disable_uuids ? pbccTrue : pbccFalse,
        opt.state ? pbccTrue : pbccFalse
        );
    if (PNR_STARTED == rslt) {
        pb->trans = PBTT_HERENOW;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    
    pubnub_mutex_unlock(pb->monitor);
    return rslt;
}


enum pubnub_res pubnub_global_here_now_ex(pubnub_t *pb, struct pubnub_here_now_options opt)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_ASSERT_OPT(NULL == opt.channel_group);

    pubnub_mutex_lock(pb->monitor);
    if (pb->state != PBS_IDLE) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }
    
    rslt = pbcc_here_now_prep(
        &pb->core, 
        NULL, 
        NULL,
        opt.disable_uuids ? pbccTrue : pbccFalse,
        opt.state ? pbccTrue : pbccFalse
        );
    if (PNR_STARTED == rslt) {
        pb->trans = PBTT_HERENOW;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    
    pubnub_mutex_unlock(pb->monitor);
    return rslt;
}
