/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_coreapi.h"

#include "pubnub_ccore.h"
#include "pubnub_netcore.h"
#include "pubnub_internal.h"
#include "pubnub_assert.h"
#include "pubnub_timers.h"

#include "pbpal.h"

#include <stdlib.h>
#include <ctype.h>


pubnub_t* pubnub_init(pubnub_t *p, const char *publish_key, const char *subscribe_key)
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
	p->cb = NULL;
	p->user_data = NULL;
#endif
	if (PUBNUB_ORIGIN_SETTABLE) {
        p->origin = PUBNUB_ORIGIN;
    }

    p->state = PBS_IDLE;
    p->trans = PBTT_NONE;
    pbpal_init(p);
    pubnub_mutex_unlock(p->monitor);

#if PUBNUB_PROXY_API
    p->proxy_type = pbproxyNONE;
    p->proxy_hostname[0] = '\0';
    p->proxy_tunnel_established = false;
    p->proxy_port = 80;
#endif

    return p;
}


enum pubnub_res pubnub_publish(pubnub_t *pb, const char *channel, const char *message)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    
    pubnub_mutex_lock(pb->monitor);
    if (pb->state != PBS_IDLE) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }

    rslt = pbcc_publish_prep(&pb->core, channel, message, true, false);
    if (PNR_STARTED == rslt) {
        pb->trans = PBTT_PUBLISH;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    pubnub_mutex_unlock(pb->monitor);
    
    return rslt;
}


enum pubnub_res pubnub_publishv2(pubnub_t *pb, const char *channel, const char *message, bool store_in_history, bool eat_after_reading)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    
    pubnub_mutex_lock(pb->monitor);
    if (pb->state != PBS_IDLE) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }

    rslt = pbcc_publish_prep(&pb->core, channel, message, store_in_history, eat_after_reading);
    if (PNR_STARTED == rslt) {
        pb->trans = PBTT_PUBLISH;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    
    pubnub_mutex_unlock(pb->monitor);
    return rslt;
}


char const *pubnub_get(pubnub_t *pb)
{
    char const *result;
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    result = pbcc_get_msg(&pb->core);
    pubnub_mutex_unlock(pb->monitor);

    return result;
}


char const *pubnub_get_channel(pubnub_t *pb)
{
    char const *result;
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    result = pbcc_get_channel(&pb->core);
    pubnub_mutex_unlock(pb->monitor);

    return result;
}


enum pubnub_res pubnub_subscribe(pubnub_t *p, const char *channel, const char *channel_group)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(p));
    
    pubnub_mutex_lock(p->monitor);
    if (p->state != PBS_IDLE) {
        pubnub_mutex_unlock(p->monitor);
        return PNR_IN_PROGRESS;
    }
    
    rslt = pbcc_subscribe_prep(&p->core, channel, channel_group, NULL);
    if (PNR_STARTED == rslt) {
        p->trans = PBTT_SUBSCRIBE;
        p->core.last_result = PNR_STARTED;
        pbnc_fsm(p);
        rslt = p->core.last_result;
    }
    
    pubnub_mutex_unlock(p->monitor);
    return rslt;
}


enum pubnub_res pubnub_leave(pubnub_t *p, const char *channel, const char *channel_group)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(p));

    pubnub_mutex_lock(p->monitor);
    if (p->state != PBS_IDLE) {
        pubnub_mutex_unlock(p->monitor);
        return PNR_IN_PROGRESS;
    }
    
    rslt = pbcc_leave_prep(&p->core, channel, channel_group);
    if (PNR_STARTED == rslt) {
        p->trans = PBTT_LEAVE;
        p->core.last_result = PNR_STARTED;
        pbnc_fsm(p);
        rslt = p->core.last_result;
    }
    
    pubnub_mutex_unlock(p->monitor);
    return rslt;
}


enum pubnub_res pubnub_time(pubnub_t *p)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(p));

    pubnub_mutex_lock(p->monitor);
    if (p->state != PBS_IDLE) {
        pubnub_mutex_unlock(p->monitor);
        return PNR_IN_PROGRESS;
    }
    
    rslt = pbcc_time_prep(&p->core);
    if (PNR_STARTED == rslt) {
        p->trans = PBTT_TIME;
        p->core.last_result = PNR_STARTED;
        pbnc_fsm(p);
        rslt = p->core.last_result;
    }
    
    pubnub_mutex_unlock(p->monitor);
    return rslt;
}


enum pubnub_res pubnub_history(pubnub_t *pb, const char *channel, unsigned count, bool include_token)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (pb->state != PBS_IDLE) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }
    
    rslt = pbcc_history_prep(&pb->core, channel, count, include_token);
    if (PNR_STARTED == rslt) {
        pb->trans = PBTT_HISTORY;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    
    pubnub_mutex_unlock(pb->monitor);
    return rslt;
}


enum pubnub_res pubnub_heartbeat(pubnub_t *pb, const char *channel, const char *channel_group)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (pb->state != PBS_IDLE) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }
    
    rslt = pbcc_heartbeat_prep(&pb->core, channel, channel_group);
    if (PNR_STARTED == rslt) {
        pb->trans = PBTT_HEARTBEAT;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    
    pubnub_mutex_unlock(pb->monitor);
    return rslt;
}


enum pubnub_res pubnub_here_now(pubnub_t *pb, const char *channel, const char *channel_group)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (pb->state != PBS_IDLE) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }
    
    rslt = pbcc_here_now_prep(&pb->core, channel, channel_group, pbccNotSet, pbccNotSet);
    if (PNR_STARTED == rslt) {
        pb->trans = PBTT_HERENOW;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    
    pubnub_mutex_unlock(pb->monitor);
    return rslt;
}


enum pubnub_res pubnub_global_here_now(pubnub_t *pb)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (pb->state != PBS_IDLE) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }
    
    rslt = pbcc_here_now_prep(&pb->core, NULL, NULL, pbccNotSet, pbccNotSet);
    if (PNR_STARTED == rslt) {
        pb->trans = PBTT_GLOBAL_HERENOW;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    
    pubnub_mutex_unlock(pb->monitor);
    return rslt;
}


enum pubnub_res pubnub_where_now(pubnub_t *pb, const char *uuid)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (pb->state != PBS_IDLE) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }
    
    rslt = pbcc_where_now_prep(&pb->core, uuid ? uuid : pb->core.uuid);
    if (PNR_STARTED == rslt) {
        pb->trans = PBTT_WHERENOW;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    
    pubnub_mutex_unlock(pb->monitor);
    return rslt;
}


enum pubnub_res pubnub_set_state(pubnub_t *pb, char const *channel, char const *channel_group, const char *uuid, char const *state)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (pb->state != PBS_IDLE) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }
    
    rslt = pbcc_set_state_prep(&pb->core, channel, channel_group, uuid ? uuid : pb->core.uuid, state);
    if (PNR_STARTED == rslt) {
        pb->trans = PBTT_SET_STATE;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    
    pubnub_mutex_unlock(pb->monitor);
    return rslt;
}


enum pubnub_res pubnub_state_get(pubnub_t *pb, char const *channel, char const *channel_group, const char *uuid)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (pb->state != PBS_IDLE) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }
    
    rslt = pbcc_state_get_prep(&pb->core, channel, channel_group, uuid ? uuid : pb->core.uuid);
    if (PNR_STARTED == rslt) {
        pb->trans = PBTT_STATE_GET;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    
    pubnub_mutex_unlock(pb->monitor);
    return rslt;
}


enum pubnub_res pubnub_remove_channel_group(pubnub_t *pb, char const *channel_group)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (pb->state != PBS_IDLE) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }
    
    rslt = pbcc_remove_channel_group_prep(&pb->core, channel_group);
    if (PNR_STARTED == rslt) {
        pb->trans = PBTT_REMOVE_CHANNEL_GROUP;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    
    pubnub_mutex_unlock(pb->monitor);
    return rslt;
}


enum pubnub_res pubnub_remove_channel_from_group(pubnub_t *pb, char const *channel, char const *channel_group)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (pb->state != PBS_IDLE) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }
    
    rslt = pbcc_channel_registry_prep(&pb->core, channel_group, "remove", channel);
    if (PNR_STARTED == rslt) {
        pb->trans = PBTT_REMOVE_CHANNEL_FROM_GROUP;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    
    pubnub_mutex_unlock(pb->monitor);
    return rslt;
}


enum pubnub_res pubnub_add_channel_to_group(pubnub_t *pb, char const *channel, char const *channel_group)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (pb->state != PBS_IDLE) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }
    
    rslt = pbcc_channel_registry_prep(&pb->core, channel_group, "add", channel);
    if (PNR_STARTED == rslt) {
        pb->trans = PBTT_ADD_CHANNEL_TO_GROUP;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    
    pubnub_mutex_unlock(pb->monitor);
    return rslt;
}


enum pubnub_res pubnub_list_channel_group(pubnub_t *pb, char const *channel_group)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (pb->state != PBS_IDLE) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }
    
    rslt = pbcc_channel_registry_prep(&pb->core, channel_group, NULL, NULL);
    if (PNR_STARTED == rslt) {
        pb->trans = PBTT_LIST_CHANNEL_GROUP;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    
    pubnub_mutex_unlock(pb->monitor);
    return rslt;
}


void pubnub_cancel(pubnub_t *pb)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    pbnc_stop(pb, PNR_CANCELLED);
    pubnub_mutex_unlock(pb->monitor);
}


void pubnub_set_uuid(pubnub_t *pb, const char *uuid)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    pubnub_mutex_lock(pb->monitor);
    pbcc_set_uuid(&pb->core, uuid);
    pubnub_mutex_unlock(pb->monitor);
}


char const *pubnub_uuid_get(pubnub_t *pb)
{
    char const *result;
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    result = pb->core.uuid;
    pubnub_mutex_unlock(pb->monitor);

    return result;
}


void pubnub_set_auth(pubnub_t *pb, const char *auth)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    pubnub_mutex_lock(pb->monitor);
    pbcc_set_auth(&pb->core, auth);
    pubnub_mutex_unlock(pb->monitor);
}


char const *pubnub_auth_get(pubnub_t *pb)
{
    char const *result;
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    result = pb->core.auth;
    pubnub_mutex_unlock(pb->monitor);

    return result;
}


int pubnub_last_http_code(pubnub_t *pb)
{
    int result;
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    pubnub_mutex_lock(pb->monitor);
    result = pb->http_code;
    pubnub_mutex_unlock(pb->monitor);
    return result;
}


char const *pubnub_last_time_token(pubnub_t *pb)
{
    char const *result;
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    result = pb->core.timetoken;
    pubnub_mutex_unlock(pb->monitor);

    return result;
}


char const *pubnub_last_publish_result(pubnub_t *pb)
{
    char *end;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (PUBNUB_DYNAMIC_REPLY_BUFFER && (NULL == pb->core.http_reply)) {
        pubnub_mutex_unlock(pb->monitor);
        return "";
    }
    if ((pb->trans != PBTT_PUBLISH) || (pb->core.http_reply[0] == '\0')) {
        pubnub_mutex_unlock(pb->monitor);
        return "";
    }
    for (end = pb->core.http_reply + 1; isdigit((unsigned)*end); ++end) {
        continue;
    }
    pubnub_mutex_unlock(pb->monitor);

    return end + 1;
}


char const *pubnub_get_origin(pubnub_t *pb)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    if (PUBNUB_ORIGIN_SETTABLE) {
        char const *result;

        pubnub_mutex_lock(pb->monitor);
        result = pb->origin;
        pubnub_mutex_unlock(pb->monitor);

        return result;
    }
    return PUBNUB_ORIGIN;
}


int pubnub_origin_set(pubnub_t *pb, char const *origin)
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
