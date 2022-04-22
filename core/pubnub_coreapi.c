/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"

#include "pubnub_coreapi.h"
#include "pubnub_ccore.h"
#include "pubnub_netcore.h"
#include "pubnub_assert.h"
#include "pubnub_timers.h"
#include "pubnub_memory_block.h"

#include "pbpal.h"
#include "pubnub_log.h"
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#if PUBNUB_ONLY_PUBSUB_API
#warning This module is not useful if configured to use only the publish and subscribe API
#endif


#if PUBNUB_USE_AUTO_HEARTBEAT
#include "lib/pbstr_remove_from_list.h"
static void update_channels_and_ch_groups(pubnub_t* pb,
                                          const char* channel,
                                          const char* channel_group)
{
    PUBNUB_ASSERT_OPT(pb_valid_ctx_ptr(pb));

    if ((NULL == channel) && (NULL == channel_group)) {
        /** pubnub_leave(default) releases both saved channels and channel groups */
        pbauto_heartbeat_free_channelInfo(pb);
        return;
    }
    if ((pb->channelInfo.channel != NULL) && (channel != NULL)) {
        pbstr_remove_from_list(pb->channelInfo.channel, channel);
        pbstr_free_if_empty(&pb->channelInfo.channel);
    }
    if ((pb->channelInfo.channel_group != NULL) && (channel_group != NULL)) {
        pbstr_remove_from_list(pb->channelInfo.channel_group, channel_group);
        pbstr_free_if_empty(&pb->channelInfo.channel_group);
    }
}

/** Prepares channel and channel groups to be used in pubnub_leave() url
   request. Checks for default(saved) values if both parameters @p channel nad
   @p channel_group are passed as NULL.
  */
static void check_if_default_channel_and_groups(pubnub_t* p,
                                                char const* channel,
                                                char const* channel_group,
                                                char const** prep_channel,
                                                char const** prep_channel_group)
{
    PUBNUB_ASSERT_OPT(prep_channel != NULL);
    PUBNUB_ASSERT_OPT(prep_channel_group != NULL);

    if ((NULL == channel) && (NULL == channel_group)) {
        /** Default read from pubnub context */
        pbauto_heartbeat_read_channelInfo(p, prep_channel, prep_channel_group);
    }
    else {
        *prep_channel = channel;
        *prep_channel_group = channel_group;
    }
}
#else
#define update_channels_and_ch_groups(pb, channel, channel_group)
#define check_if_default_channel_and_groups(                                   \
    p, channel, channel_group, prep_channel, prep_channel_group)               \
    do {                                                                       \
        *(prep_channel)       = (channel);                                     \
        *(prep_channel_group) = (channel_group);                               \
    } while (0)
#endif /* PUBNUB_USE_AUTO_HEARTBEAT */


enum pubnub_res pubnub_leave(pubnub_t* p, const char* channel, const char* channel_group)
{
    enum pubnub_res rslt;
    char const* prep_channel;
    char const* prep_channel_group;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(p));

    pubnub_mutex_lock(p->monitor);
    if (!pbnc_can_start_transaction(p)) {
        pubnub_mutex_unlock(p->monitor);
        return PNR_IN_PROGRESS;
    }
    check_if_default_channel_and_groups(p,
                                        channel,
                                        channel_group,
                                        &prep_channel,
                                        &prep_channel_group);

    rslt = pbcc_leave_prep(&p->core, prep_channel, prep_channel_group);
    if (PNR_STARTED == rslt) {
        p->trans            = PBTT_LEAVE;
        p->core.last_result = PNR_STARTED;
        update_channels_and_ch_groups(p, channel, channel_group);
        pbnc_fsm(p);
        rslt = p->core.last_result;
    }

    pubnub_mutex_unlock(p->monitor);
    return rslt;
}


enum pubnub_res pubnub_time(pubnub_t* p)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(p));

    pubnub_mutex_lock(p->monitor);
    if (!pbnc_can_start_transaction(p)) {
        pubnub_mutex_unlock(p->monitor);
        return PNR_IN_PROGRESS;
    }

    rslt = pbcc_time_prep(&p->core);
    if (PNR_STARTED == rslt) {
        p->trans            = PBTT_TIME;
        p->core.last_result = PNR_STARTED;
        pbnc_fsm(p);
        rslt = p->core.last_result;
    }

    pubnub_mutex_unlock(p->monitor);
    return rslt;
}


enum pubnub_res pubnub_history(pubnub_t*   pb,
                               const char* channel,
                               unsigned    count,
                               bool        include_token)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }

    rslt = pbcc_history_prep(
        &pb->core, channel, count, include_token, pbccNotSet, pbccNotSet, pbccNotSet, NULL, NULL);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_HISTORY;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }

    pubnub_mutex_unlock(pb->monitor);
    return rslt;
}


enum pubnub_res pubnub_heartbeat(pubnub_t*   pb,
                                 const char* channel,
                                 const char* channel_group)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }
    rslt = pbauto_heartbeat_prepare_channels_and_ch_groups(pb, &channel, &channel_group);
    if (rslt != PNR_OK) {
        return rslt;
    }

    rslt = pbcc_heartbeat_prep(&pb->core, channel, channel_group);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_HEARTBEAT;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }

    pubnub_mutex_unlock(pb->monitor);
    return rslt;
}


enum pubnub_res pubnub_here_now(pubnub_t*   pb,
                                const char* channel,
                                const char* channel_group)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }

    rslt = pbcc_here_now_prep(
        &pb->core, channel, channel_group, pbccNotSet, pbccNotSet);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_HERENOW;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }

    pubnub_mutex_unlock(pb->monitor);
    return rslt;
}


enum pubnub_res pubnub_global_here_now(pubnub_t* pb)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }

    rslt = pbcc_here_now_prep(&pb->core, NULL, NULL, pbccNotSet, pbccNotSet);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_GLOBAL_HERENOW;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }

    pubnub_mutex_unlock(pb->monitor);
    return rslt;
}


enum pubnub_res pubnub_where_now(pubnub_t* pb, const char* uuid)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }

    rslt = pbcc_where_now_prep(&pb->core, uuid ? uuid : pbcc_uuid_get(&pb->core));
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_WHERENOW;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }

    pubnub_mutex_unlock(pb->monitor);
    return rslt;
}

size_t json_include(char* json, char* characters) {
    return snprintf(json, strlen(json), "%s", characters);
}

size_t json_include_state_for_each(char* json, const char* elements, char* state)
{
    char* sep = ",";
    char* element;
    char* elements_copy = strdup(elements);
    size_t pos = 0;
    for (element = strtok(elements_copy, sep); element; element = strtok(NULL, sep)) {
        pos += snprintf(json, strlen(json), "\"%s\":%s,", element, state);
    }
    free(elements_copy);
    return pos;
}

bool json_ends_with_separator(char* json, int json_cursor) {
    if (json_cursor > 0 && strncmp(json + json_cursor - 1, (char*)",", 1) == 0) {
        return true;
    } else {
        return false;
    }
}

enum pubnub_res pubnub_set_state(pubnub_t*   pb,
                                 char const* channel,
                                 char const* channel_group,
                                 const char* uuid,
                                 char const* state)
{
    enum pubnub_res rslt;
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }
    rslt = pbcc_set_state_prep(&pb->core,
                               channel,
                               channel_group,
                               uuid ? uuid : pbcc_uuid_get(&pb->core),
                               state);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_SET_STATE;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
        if (rslt == PNR_STARTED) {
            int tot_ch = 0, tot_cg = 0;
            if (channel) {
                tot_ch = 1;
                for (int i = 0; i < (int)strlen(channel); i++) {
                    tot_ch = (channel[i] == ',') ? tot_ch + 1 : tot_ch;
                }
            }
            if (channel_group) {
                tot_cg = 1;
                for (int i = 0; i < (int)strlen(channel_group); i++) {
                    tot_cg = (channel_group[i] == ',') ? tot_cg + 1 : tot_cg;
                }
            }

            int buff_size = ((tot_ch + tot_cg) * strlen(state))
                            + (channel ? strlen(channel) : 1)
                            + (channel_group ? strlen(channel_group) : 1) + 20;
            char* json_state = (char*)malloc(buff_size);
            if (pb->core.state != NULL && buff_size != sizeof(pb->core.state)) {
                pb->core.state = (char*)realloc((char*)pb->core.state, buff_size);
            }
            else if (pb->core.state == NULL) {
                pb->core.state = (char*)malloc(buff_size);
            }
            int json_cursor = 0;
            if (json_state != NULL && pb->core.state != NULL) {
                json_cursor += json_include(json_state + json_cursor, "{");
                if (channel && strncmp(channel, (char*)",", 1) != 0) {
                    json_cursor += json_include_state_for_each(json_state + json_cursor, channel, (char*)state);
                }

                if (channel_group) {
                    json_cursor += json_include_state_for_each(json_state + json_cursor, channel_group, (char*)state);
                }

                if (json_ends_with_separator(json_state, json_cursor)) {
                    json_cursor -= strlen(",");
                }

                json_cursor += json_include(json_state + json_cursor, "}");
                json_state[json_cursor] = '\0';
                PUBNUB_LOG_DEBUG("formatted state is %s\n", json_state);

                strcpy((char*)pb->core.state, (const char*)json_state);
                free(json_state);
                json_state = NULL;
            }
        }
    }

    pubnub_mutex_unlock(pb->monitor);
    return rslt;
}


enum pubnub_res pubnub_state_get(pubnub_t*   pb,
                                 char const* channel,
                                 char const* channel_group,
                                 const char* uuid)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }

    rslt = pbcc_state_get_prep(
        &pb->core, channel, channel_group, uuid ? uuid : pbcc_uuid_get(&pb->core));
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_STATE_GET;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }

    pubnub_mutex_unlock(pb->monitor);
    return rslt;
}


enum pubnub_res pubnub_remove_channel_group(pubnub_t* pb, char const* channel_group)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }

    rslt = pbcc_remove_channel_group_prep(&pb->core, channel_group);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_REMOVE_CHANNEL_GROUP;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }

    pubnub_mutex_unlock(pb->monitor);
    return rslt;
}


enum pubnub_res pubnub_remove_channel_from_group(pubnub_t*   pb,
                                                 char const* channel,
                                                 char const* channel_group)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }

    rslt = pbcc_channel_registry_prep(&pb->core, channel_group, "remove", channel);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_REMOVE_CHANNEL_FROM_GROUP;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }

    pubnub_mutex_unlock(pb->monitor);
    return rslt;
}


enum pubnub_res pubnub_add_channel_to_group(pubnub_t*   pb,
                                            char const* channel,
                                            char const* channel_group)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }

    rslt = pbcc_channel_registry_prep(&pb->core, channel_group, "add", channel);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_ADD_CHANNEL_TO_GROUP;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }

    pubnub_mutex_unlock(pb->monitor);
    return rslt;
}


enum pubnub_res pubnub_list_channel_group(pubnub_t* pb, char const* channel_group)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }

    rslt = pbcc_channel_registry_prep(&pb->core, channel_group, NULL, NULL);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_LIST_CHANNEL_GROUP;
        pb->core.last_result = PNR_STARTED;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }

    pubnub_mutex_unlock(pb->monitor);
    return rslt;
}


bool pubnub_can_start_transaction(pubnub_t* pb)
{
    bool rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    rslt = pbnc_can_start_transaction(pb);
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}

/** Should be called only if server reported an error */
int pubnub_last_http_response_body(pubnub_t* pb, pubnub_chamebl_t* o_msg)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return -1;
    }
    o_msg->size = pb->core.http_buf_len;
    o_msg->ptr = pb->core.http_reply;

    pubnub_mutex_unlock(pb->monitor);
    return 0;
}
