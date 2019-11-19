/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "posix/monotonic_clock_get_time.h"
#include "posix/pbtimespec_elapsed_ms.h"

#include "pubnub_internal.h"
#include "core/pubnub_alloc.h"
#include "core/pubnub_ccore_limits.h"
#include "core/pubnub_pubsubapi.h"
#include "core/pubnub_coreapi.h"
#include "core/pubnub_free_with_timeout.h"
#include "lib/pb_strnlen_s.h"
#include "core/pubnub_assert.h"
#include "core/pubnub_log.h"
#include "core/pbpal.h"
#include "core/pubnub_helper.h"

#include <pthread.h>

#include <stdlib.h>
#include <string.h>


#define PUBNUB_MIN_HEARTBEAT_PERIOD (PUBNUB_MIN_TRANSACTION_TIMER / UNIT_IN_MILLI)

struct pubnub_heartbeat_data {
    pubnub_t* pb;
    pubnub_t* heartbeat_pb;
    size_t period_sec;
};

struct HeartbeatWatcherData {
    struct pubnub_heartbeat_data heartbeat_data[PUBNUB_MAX_HEARTBEAT_THUMPERS] pubnub_guarded_by(mutw);
    unsigned thumpers_in_use pubnub_guarded_by(mutw);
    /** Times left for each of the thumper timers in progress */
    size_t heartbeat_timers[PUBNUB_MAX_HEARTBEAT_THUMPERS] pubnub_guarded_by(timerlock);
    /** Array of thumper indexes whos auto heartbeat timers are active and running */
    unsigned timer_index_array[PUBNUB_MAX_HEARTBEAT_THUMPERS] pubnub_guarded_by(timerlock);
    /** Number of active thumper timers */
    unsigned active_timers pubnub_guarded_by(timerlock);
    bool stop_heartbeat_watcher_thread pubnub_guarded_by(stoplock);
    pthread_mutex_t mutw;
    pthread_mutex_t timerlock;
    pthread_mutex_t stoplock;
    pthread_t       thread_id;
};


static struct HeartbeatWatcherData m_watcher;


static void start_heartbeat_timer(unsigned thumper_index)
{
    size_t period_sec;
    
    PUBNUB_ASSERT_OPT(thumper_index < PUBNUB_MAX_HEARTBEAT_THUMPERS);
    
    PUBNUB_LOG_TRACE("--->start_heartbeat_timer(%u).\n", thumper_index);
    pthread_mutex_lock(&m_watcher.mutw);
    period_sec = m_watcher.heartbeat_data[thumper_index].period_sec;
    pthread_mutex_unlock(&m_watcher.mutw);
    
    pthread_mutex_lock(&m_watcher.timerlock);
    PUBNUB_ASSERT_OPT(m_watcher.active_timers < PUBNUB_MAX_HEARTBEAT_THUMPERS);
    m_watcher.heartbeat_timers[thumper_index] = period_sec * UNIT_IN_MILLI;
    m_watcher.timer_index_array[m_watcher.active_timers++] = thumper_index;
    pthread_mutex_unlock(&m_watcher.timerlock);
}


static int copy_context_settings(pubnub_t* pb_clone, pubnub_t const* pb)
{
    PUBNUB_ASSERT_OPT(pb_valid_ctx_ptr(pb_clone));
    PUBNUB_ASSERT_OPT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb_clone->monitor);
    pb_clone->core.auth = pb->core.auth;
    strcpy(pb_clone->core.uuid, pb->core.uuid);
    if (PUBNUB_ORIGIN_SETTABLE) {
        pb_clone->origin = pb->origin;
    }
#if PUBNUB_BLOCKING_IO_SETTABLE
    pb_clone->options.use_blocking_io = pb->options.use_blocking_io;
#endif /* PUBNUB_BLOCKING_IO_SETTABLE */
    pb_clone->options.use_http_keep_alive = pb->options.use_http_keep_alive;
#if PUBNUB_USE_IPV6 && defined(PUBNUB_CALLBACK_API)
    pb_clone->options.ipv6_connectivity = pb->options.ipv6_connectivity;
#endif
#if PUBNUB_ADVANCED_KEEP_ALIVE
    pb_clone->keep_alive.max = pb->keep_alive.max;
    pb_clone->keep_alive.timeout = pb->keep_alive.timeout;
#endif
#if PUBNUB_PROXY_API
    pb_clone->proxy_type = pb->proxy_type;
    strcpy(pb_clone->proxy_hostname, pb->proxy_hostname);
#if defined(PUBNUB_CALLBACK_API)
    memcpy(&(pb_clone->proxy_ipv4_address),
           &(pb->proxy_ipv4_address),
           sizeof pb->proxy_ipv4_address);
#if PUBNUB_USE_IPV6
    memcpy(&(pb_clone->proxy_ipv6_address),
           &(pb->proxy_ipv6_address),
           sizeof pb->proxy_ipv6_address);
#endif
#endif /* defined(PUBNUB_CALLBACK_API) */
    pb_clone->proxy_port        = pb->proxy_port;
    pb_clone->proxy_auth_scheme = pb->proxy_auth_scheme;
    pb_clone->proxy_auth_username = pb->proxy_auth_username;
    pb_clone->proxy_auth_password = pb->proxy_auth_password;
    strcpy(pb_clone->realm, pb->realm);
#endif /* PUBNUB_PROXY_API */
    pubnub_mutex_unlock(pb_clone->monitor);

    return 0;
}


static bool pubsub_keys_changed(pubnub_t const* pb_clone, pubnub_t const* pb)
{
    return (pb_clone->core.publish_key != pb->core.publish_key) ||
           (pb_clone->core.subscribe_key != pb->core.subscribe_key);
}


static void heartbeat_thump(pubnub_t* pb, pubnub_t* heartbeat_pb)
{
    char const* channel;
    char const* channel_group;
    bool keys_changed;

    PUBNUB_ASSERT_OPT(pb_valid_ctx_ptr(pb));
    PUBNUB_ASSERT_OPT(pb_valid_ctx_ptr(heartbeat_pb));

    pubnub_mutex_lock(pb->monitor);    
    pubnub_mutex_lock(heartbeat_pb->monitor);
    keys_changed = pubsub_keys_changed(heartbeat_pb, pb);
    pubnub_mutex_unlock(heartbeat_pb->monitor);
        
    if (keys_changed) {
        pubnub_mutex_unlock(pb->monitor);
        pubnub_cancel(heartbeat_pb);        
        return;
    }
    channel = pb->channelInfo.channel;
    channel_group = pb->channelInfo.channel_group;
    if (((channel != NULL) && (pb_strnlen_s(channel, PUBNUB_MAX_OBJECT_LENGTH) > 0)) ||
        ((channel_group != NULL) && (pb_strnlen_s(channel_group, PUBNUB_MAX_OBJECT_LENGTH) > 0))) {
        enum pubnub_res res;
        PUBNUB_LOG_TRACE("--->heartbeat_thump(pb=%p, heartbeat_pb=%p).\n", pb, heartbeat_pb);
        copy_context_settings(heartbeat_pb, pb);
        res = pubnub_heartbeat(heartbeat_pb, channel, channel_group);
        if (res != PNR_STARTED) {
            PUBNUB_LOG_ERROR("heartbeat_thump(pb=%p, heartbeat_pb) - "
                             "pubnub_heartbeat(heartbeat_pb=%p) returned unexpected: %d('%s')\n",
                             pb,
                             heartbeat_pb,
                             res,
                             pubnub_res_2_string(res));
        }
    }
    pubnub_mutex_unlock(pb->monitor);
}


static void auto_heartbeat_callback(pubnub_t*         heartbeat_pb,
                                    enum pubnub_trans trans,
                                    enum pubnub_res   result,
                                    void*             user_data)
{
    unsigned thumper_index = heartbeat_pb->thumperIndex;
    
    PUBNUB_ASSERT_OPT((PBTT_HEARTBEAT == trans) || (PNR_CANCELLED == result));
    
    /* Maybe something should be done with this */
    pubnub_get(heartbeat_pb);
    
    if (PNR_OK == result) {
        /* Start heartbeat timer */
        start_heartbeat_timer(thumper_index);
    }
    else {
        pubnub_t* pb;
        pthread_mutex_lock(&m_watcher.mutw);
        pb = m_watcher.heartbeat_data[thumper_index].pb;
        pthread_mutex_unlock(&m_watcher.mutw);

        if (result != PNR_CANCELLED) {
            PUBNUB_LOG_WARNING("punbub_heartbeat(heartbeat_pb=%p) failed with code: %d('%s') - "
                               "will try again.\n",
                               heartbeat_pb,
                               result,
                               pubnub_res_2_string(result));
            /* Depending on the kind of error try thumping again */
            heartbeat_thump(pb, heartbeat_pb);
        }
        else if (pb != NULL) {
            pubnub_mutex_lock(pb->monitor);    
            if (pubsub_keys_changed(heartbeat_pb, pb)) {
                pubnub_init(heartbeat_pb, pb->core.publish_key, pb->core.subscribe_key);            
                heartbeat_thump(pb, heartbeat_pb);
            }
            pubnub_mutex_unlock(pb->monitor);
        }
    }
}


static pubnub_t* init_new_thumper_pb(pubnub_t* pb, unsigned i)
{
    pubnub_t* pb_new;

    PUBNUB_ASSERT_OPT(pb_valid_ctx_ptr(pb));

    pb_new = pubnub_alloc();
    if (NULL == pb_new) {
        PUBNUB_LOG_ERROR("init_new_thumper_pb(pb = %p) - "
                         "Failed to allocate clone heartbeat context!\n",
                         pb);
        return NULL;
    }
    pubnub_mutex_lock(pb->monitor);
    pubnub_init(pb_new, pb->core.publish_key, pb->core.subscribe_key);
    pubnub_mutex_unlock(pb->monitor);
    
    pubnub_mutex_lock(pb_new->monitor);
    pb_new->thumperIndex = i;
    pubnub_mutex_unlock(pb_new->monitor);
        
    return pb_new;
}


static void take_the_timer_out(unsigned* indexes, unsigned i, unsigned* active_timers)
{
    unsigned* timer_out = indexes + i;
    --*active_timers;
    memmove(timer_out, timer_out + 1, (*active_timers - i)*sizeof(unsigned));
}


static void handle_heartbeat_timers(int elapsed_ms)
{
    unsigned i;
    unsigned* indexes = m_watcher.timer_index_array;
    unsigned active_timers = m_watcher.active_timers;
    
    for (i = 0; i < active_timers;) {
        unsigned thumper_index = indexes[i];
        int remains_ms = m_watcher.heartbeat_timers[thumper_index] - elapsed_ms;
        if (remains_ms <= 0) {
            struct pubnub_heartbeat_data* thumper = &m_watcher.heartbeat_data[thumper_index];
            pubnub_t* pb;
            pubnub_t* heartbeat_pb;
            
            /* Taking out one that has expired */
            take_the_timer_out(indexes, i, &active_timers);

            pthread_mutex_lock(&m_watcher.mutw);
            pb = thumper->pb;
            heartbeat_pb = thumper->heartbeat_pb;
            pthread_mutex_unlock(&m_watcher.mutw);

            if (NULL == heartbeat_pb) {
                heartbeat_pb = init_new_thumper_pb(pb, thumper_index);
                if (NULL == heartbeat_pb) {
                    continue;
                }
                pubnub_register_callback(heartbeat_pb, auto_heartbeat_callback, NULL);

                pthread_mutex_lock(&m_watcher.mutw);
                thumper->heartbeat_pb = heartbeat_pb;
                pthread_mutex_unlock(&m_watcher.mutw);
            }
            /* Heartbeat thump */
            heartbeat_thump(pb, heartbeat_pb);
        }
        else {
            m_watcher.heartbeat_timers[thumper_index] = (size_t)remains_ms;
            i++;
        }
    }
    m_watcher.active_timers = active_timers;
}


static void* heartbeat_watcher_thread(void* arg)
{
    struct timespec const sleep_time = { 0, 1000000 };
    struct timespec prev_timspec;
    monotonic_clock_get_time(&prev_timspec);

    for (;;) {
        struct timespec timspec;
        int elapsed;
        bool stop_thread;
        
        pthread_mutex_lock(&m_watcher.stoplock);
        stop_thread = m_watcher.stop_heartbeat_watcher_thread;
        pthread_mutex_unlock(&m_watcher.stoplock);
        if (stop_thread) {
            break;
        }
        
        nanosleep(&sleep_time, NULL);
        monotonic_clock_get_time(&timspec);

        elapsed = pbtimespec_elapsed_ms(prev_timspec, timspec);
        if (elapsed > 0) {
            pthread_mutex_lock(&m_watcher.timerlock);
            handle_heartbeat_timers(elapsed);
            pthread_mutex_unlock(&m_watcher.timerlock);
            prev_timspec = timspec;
        }
    }

    return NULL;
}


static int create_heartbeat_watcher_thread(void)
{
    int rslt;
    
#if defined(PUBNUB_CALLBACK_THREAD_STACK_SIZE_KB)                              \
    && (PUBNUB_CALLBACK_THREAD_STACK_SIZE_KB > 0)
    {
        pthread_attr_t thread_attr;

        rslt = pthread_attr_init(&thread_attr);
        if (rslt != 0) {
            PUBNUB_LOG_ERROR(
                "create_heartbeat_watcher_thread() - "
                "Failed to initialize thread attributes, error code: %d\n",
                rslt);
            return -1;
        }
        rslt = pthread_attr_setstacksize(
            &thread_attr, PUBNUB_CALLBACK_THREAD_STACK_SIZE_KB * 1024);
        if (rslt != 0) {
            PUBNUB_LOG_ERROR(
                "create_heartbeat_watcher_thread() - "
                "Failed to set thread stack size to %d kb, error code: %d\n",
                PUBNUB_CALLBACK_THREAD_STACK_SIZE_KB,
                rslt);
            pthread_attr_destroy(&thread_attr);
            return -1;
        }
        rslt = pthread_create(
            &m_watcher.thread_id, &thread_attr, heartbeat_watcher_thread, NULL);
        if (rslt != 0) {
            PUBNUB_LOG_ERROR(
                "create_heartbeat_watcher_thread() - "
                "Failed to create the auto heartbeat watcher thread, error code: %d\n",
                rslt);
            pthread_attr_destroy(&thread_attr);
            return -1;
        }
    }
#else
    rslt =
        pthread_create(&m_watcher.thread_id, NULL, heartbeat_watcher_thread, NULL);
    if (rslt != 0) {
        PUBNUB_LOG_ERROR(
            "create_heartbeat_watcher_thread() - "
            "Failed to create the auto heartbeat watcher thread, error code: %d\n",
            rslt);
        return -1;
    }
#endif

    return 0;
}


static int auto_heartbeat_init(void)
{
    int                 rslt;
    pthread_mutexattr_t attr;

    rslt = pthread_mutexattr_init(&attr);
    if (rslt != 0) {
        PUBNUB_LOG_ERROR(
            "auto_heartbeat_init() - Failed to initialize mutex attributes, error code: %d\n",
            rslt);
        return -1;
    }
    rslt = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    if (rslt != 0) {
        PUBNUB_LOG_ERROR(
            "auto_heartbeat_init() - Failed to set mutex attribute type, error code: %d\n",
            rslt);
        pthread_mutexattr_destroy(&attr);
        return -1;
    }
    rslt = pthread_mutex_init(&m_watcher.stoplock, &attr);
    if (rslt != 0) {
        PUBNUB_LOG_ERROR(
            "auto_heartbeat_init() - Failed to initialize 'stoplock' mutex, error code: %d\n",
            rslt);
        pthread_mutexattr_destroy(&attr);
        return -1;
    }
    rslt = pthread_mutex_init(&m_watcher.mutw, &attr);
    if (rslt != 0) {
        PUBNUB_LOG_ERROR(
            "auto_heartbeat_init() - Failed to initialize mutex, error code: %d\n",
            rslt);
        pthread_mutexattr_destroy(&attr);
        pthread_mutex_destroy(&m_watcher.stoplock);
        return -1;
    }
    rslt = pthread_mutex_init(&m_watcher.timerlock, &attr);
    if (rslt != 0) {
        PUBNUB_LOG_ERROR(
            "auto_heartbeat_init() - Failed to initialize mutex for heartbeat timers, "
            "error code: %d\n",
            rslt);
        pthread_mutexattr_destroy(&attr);
        pthread_mutex_destroy(&m_watcher.mutw);
        pthread_mutex_destroy(&m_watcher.stoplock);
        return -1;
    }
    m_watcher.stop_heartbeat_watcher_thread = false;
    rslt = create_heartbeat_watcher_thread();
    if (rslt != 0) {
        PUBNUB_LOG_ERROR(
            "auto_heartbeat_init() - create_heartbeat_watcher_thread() failed, "
            "error code: %d\n",
            rslt);
        pthread_mutexattr_destroy(&attr);
        pthread_mutex_destroy(&m_watcher.mutw);
        pthread_mutex_destroy(&m_watcher.timerlock);
        pthread_mutex_destroy(&m_watcher.stoplock);
        return -1;
    }

    return 0;
}


/** Initializes auto heartbeat thumper for @p pb context and if its called for the first
    time starts auto heartbeat watcher thread.
  */
static int form_heartbeat_thumper(pubnub_t* pb)
{
    unsigned i;
    static bool s_began = false;
    struct pubnub_heartbeat_data* heartbeat_data = m_watcher.heartbeat_data;

    PUBNUB_ASSERT_OPT(pb_valid_ctx_ptr(pb));

    if (!s_began) {
        auto_heartbeat_init();
        s_began = true;
    }
    
    pthread_mutex_lock(&m_watcher.mutw);
    if (m_watcher.thumpers_in_use >= PUBNUB_MAX_HEARTBEAT_THUMPERS) {
        PUBNUB_LOG_WARNING("form_heartbeat_thumper(pb=%p) - No more heartbeat thumpers left: "
                           "PUBNUB_MAX_HEARTBEAT_THUMPERS = %d\n"
                           "thumpers_in_use = %u\n",
                           pb,
                           PUBNUB_MAX_HEARTBEAT_THUMPERS,
                           m_watcher.thumpers_in_use);
        pthread_mutex_unlock(&m_watcher.mutw);

        return -1;
    }
    for (i = 0; i < PUBNUB_MAX_HEARTBEAT_THUMPERS; i++) {
        struct pubnub_heartbeat_data* thumper = &heartbeat_data[i];
        if (NULL == thumper->pb) {
            pubnub_t* heartbeat_pb = thumper->heartbeat_pb;
            if (NULL == heartbeat_pb) {
                heartbeat_pb = init_new_thumper_pb(pb, i);
                if (NULL == heartbeat_pb) {
                    pthread_mutex_unlock(&m_watcher.mutw);
                    return -1;
                }
                pubnub_register_callback(heartbeat_pb, auto_heartbeat_callback, NULL);
                thumper->heartbeat_pb = heartbeat_pb;
            }
            pb->thumperIndex = i;
            thumper->pb = pb;
            thumper->period_sec = PUBNUB_MIN_HEARTBEAT_PERIOD;
            m_watcher.thumpers_in_use++;
            break;
        }
    }
    pthread_mutex_unlock(&m_watcher.mutw);

    return 0;
}


int pubnub_set_heartbeat_period(pubnub_t* pb, size_t period_sec)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_ASSERT(period_sec > 0);

    pubnub_mutex_lock(pb->monitor);
    if (UNASSIGNED == pb->thumperIndex) {
        pubnub_mutex_unlock(pb->monitor);
        PUBNUB_LOG_ERROR("Error: pubnub_set_heartbeat_period(pb=%p) - "
                         "Auto heartbeat is desabled.\n",
                         pb);
        return -1;
    }
    pthread_mutex_lock(&m_watcher.mutw);
    m_watcher.heartbeat_data[pb->thumperIndex].period_sec =
        period_sec < PUBNUB_MIN_HEARTBEAT_PERIOD ? PUBNUB_MIN_HEARTBEAT_PERIOD : period_sec;
    pubnub_mutex_unlock(pb->monitor);
    pthread_mutex_unlock(&m_watcher.mutw);

    return 0;
}


int pubnub_enable_auto_heartbeat(pubnub_t* pb, size_t period_sec)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    
    pubnub_mutex_lock(pb->monitor);
    if (UNASSIGNED == pb->thumperIndex) {
        if (form_heartbeat_thumper(pb) != 0) {
            pubnub_mutex_unlock(pb->monitor);
            
            return -1;
        }
    }
    pubnub_mutex_unlock(pb->monitor);

    return pubnub_set_heartbeat_period(pb, period_sec);
}


static void auto_heartbeat_stop_timer(unsigned thumper_index)
{
    unsigned i;
    unsigned* indexes;
    unsigned active_timers;

    PUBNUB_ASSERT_OPT(thumper_index < PUBNUB_MAX_HEARTBEAT_THUMPERS);

    pthread_mutex_lock(&m_watcher.timerlock);
    active_timers = m_watcher.active_timers;
    for (i = 0, indexes = m_watcher.timer_index_array; i < active_timers; i++) {
        if (indexes[i] == thumper_index) {
            /* Taking timer out */
            take_the_timer_out(indexes, i, &active_timers);
            m_watcher.active_timers = active_timers;
            break;
        }
    }
    pthread_mutex_unlock(&m_watcher.timerlock);
}


static void stop_heartbeat(pubnub_t* heartbeat_pb, unsigned thumper_index)
{
    PUBNUB_ASSERT_OPT(pb_valid_ctx_ptr(heartbeat_pb));

    pubnub_mutex_lock(heartbeat_pb->monitor);
    if (pbnc_can_start_transaction(heartbeat_pb)) {
        auto_heartbeat_stop_timer(thumper_index);
    }
    else {
        pubnub_cancel(heartbeat_pb);
    }
    pubnub_mutex_unlock(heartbeat_pb->monitor);
}


static void release_thumper(unsigned thumper_index)
{
    if (thumper_index < PUBNUB_MAX_HEARTBEAT_THUMPERS) {
        struct pubnub_heartbeat_data* thumper;
        pubnub_t* heartbeat_pb;
        
        pthread_mutex_lock(&m_watcher.mutw);
        thumper = &m_watcher.heartbeat_data[thumper_index];
        heartbeat_pb = thumper->heartbeat_pb;
        thumper->pb = NULL;
        --m_watcher.thumpers_in_use;
        pthread_mutex_unlock(&m_watcher.mutw);
        
        stop_heartbeat(heartbeat_pb, thumper_index);
    }
}

/** If it is a thumper pubnub context, it is exempted from some usual procedures. */
static bool is_exempted(pubnub_t const* pb, unsigned thumper_index)
{
    pubnub_t const* pb_exempted;

    pthread_mutex_lock(&m_watcher.mutw);
    pb_exempted = m_watcher.heartbeat_data[thumper_index].heartbeat_pb;
    pthread_mutex_unlock(&m_watcher.mutw);
   
    return (pb == pb_exempted);
}


void pubnub_disable_auto_heartbeat(pubnub_t* pb)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    
    pubnub_mutex_lock(pb->monitor);
    if (is_exempted(pb, pb->thumperIndex)) {
        pubnub_mutex_unlock(pb->monitor);
        return;
    }
    release_thumper(pb->thumperIndex);
    pb->thumperIndex = UNASSIGNED;
    pubnub_mutex_unlock(pb->monitor);
}


bool pubnub_is_auto_heartbeat_enabled(pubnub_t* pb)
{
    bool rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    rslt = (pb->thumperIndex != UNASSIGNED);
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}


void pbauto_heartbeat_free_channelInfo(pubnub_t* pb)
{
    PUBNUB_ASSERT_OPT(pb_valid_ctx_ptr(pb));

    if (pb->channelInfo.channel != NULL) {
        free(pb->channelInfo.channel);
        pb->channelInfo.channel = NULL;
    }
    if (pb->channelInfo.channel_group != NULL) {
        free(pb->channelInfo.channel_group);
        pb->channelInfo.channel_group = NULL;
    }
}


void pbauto_heartbeat_read_channelInfo(pubnub_t const* pb,
                                       char const** channel,
                                       char const** channel_group)
{
    PUBNUB_ASSERT_OPT(channel != NULL);
    PUBNUB_ASSERT_OPT(channel_group != NULL);

    *channel = pb->channelInfo.channel;
    *channel_group = pb->channelInfo.channel_group;
}


static enum pubnub_res write_auto_heartbeat_channelInfo(pubnub_t* pb,
                                                        char const* channel,
                                                        char const* channel_group)
{
    PUBNUB_ASSERT_OPT((channel != NULL) || (channel_group != NULL));

    pbauto_heartbeat_free_channelInfo(pb);    
    if (channel != NULL) {
        pb->channelInfo.channel = strndup(channel, PUBNUB_MAX_OBJECT_LENGTH);
        if (NULL == pb->channelInfo.channel) {
            PUBNUB_LOG_ERROR("Error: write_auto_heartbeat_info(pb=%p) - "
                             "Failed to allocate memory for channel string duplicate: "
                             "channel = '%s'\n",
                             pb,
                             channel);
            return PNR_OUT_OF_MEMORY;
        }
    }
    if (channel_group != NULL) {
        pb->channelInfo.channel_group = strndup(channel_group, PUBNUB_MAX_OBJECT_LENGTH);
        if (NULL == pb->channelInfo.channel_group) {
            PUBNUB_LOG_ERROR("Error: write_auto_heartbeat_info(pb=%p) - "
                             "Failed to allocate memory for channel_group string duplicate: "
                             "channel_group = '%s'\n",
                             pb,
                             channel_group);
            if (channel != NULL) {
                free(pb->channelInfo.channel);
                pb->channelInfo.channel = NULL;
            }
            return PNR_OUT_OF_MEMORY;
        }
    }
    
    return PNR_OK;
}


enum pubnub_res pbauto_heartbeat_prepare_channels_and_ch_groups(pubnub_t* pb,
                                                                char const** channel,
                                                                char const** channel_group)
{
    PUBNUB_ASSERT_OPT(channel != NULL);
    PUBNUB_ASSERT_OPT(channel_group != NULL);

    if ((NULL == *channel) && (NULL == *channel_group)) {
        pbauto_heartbeat_read_channelInfo(pb, channel, channel_group);
        return PNR_OK;
    }
    
    return write_auto_heartbeat_channelInfo(pb, *channel, *channel_group);
}


void pbauto_heartbeat_start_timer(pubnub_t const* pb)
{
    PUBNUB_ASSERT_OPT(pb_valid_ctx_ptr(pb));

    if (pb->thumperIndex != UNASSIGNED) {
        switch (pb->trans) {
        case PBTT_HEARTBEAT :
            if (is_exempted(pb, pb->thumperIndex)) {
                break;
            }
            /*FALLTHRU*/
        case PBTT_SUBSCRIBE :
        case PBTT_SUBSCRIBE_V2 :
            start_heartbeat_timer(pb->thumperIndex);
            break;
        default:
            break;
        }
    }
}


void pubnub_heartbeat_free_thumpers(void)
{
    unsigned i;
    struct pubnub_heartbeat_data* heartbeat_data = m_watcher.heartbeat_data;
    struct timespec const sleep_time = { 1, 0 };

    for (i = 0; i < PUBNUB_MAX_HEARTBEAT_THUMPERS; i++) {
        pubnub_t* heartbeat_pb;
        
        pthread_mutex_lock(&m_watcher.mutw);
        heartbeat_pb = heartbeat_data[i].heartbeat_pb;
        pthread_mutex_unlock(&m_watcher.mutw);
        
        if (heartbeat_pb != NULL) {
            if (pubnub_free_with_timeout(heartbeat_pb, 1000) != 0) {
                PUBNUB_LOG_ERROR("Error: pbauto_heartbeat_free_thumpers() - "
                                 "Failed to free the Pubnub heartbeat %u. thumper context: "
                                 "heartbeat_pb=%p\n",
                                 i,
                                 heartbeat_pb);
            }
            else {
                PUBNUB_LOG_TRACE("pubnub_heartbeat_free_thumpers() - "
                                 "%u. thumper(heartbeat_pb=%p) freed.\n",
                                 i+1,
                                 heartbeat_pb);
                pthread_mutex_lock(&m_watcher.mutw);
                heartbeat_data[i].heartbeat_pb = NULL;
                pthread_mutex_unlock(&m_watcher.mutw);
            }
        }
    }
    /* Waits until the contexts are released from the processing queue */
    nanosleep(&sleep_time, NULL);
}


static void stop_auto_heartbeat(unsigned thumper_index)
{
    pubnub_t* heartbeat_pb;

    pthread_mutex_lock(&m_watcher.mutw);
    heartbeat_pb = m_watcher.heartbeat_data[thumper_index].heartbeat_pb;
    pthread_mutex_unlock(&m_watcher.mutw);

    stop_heartbeat(heartbeat_pb, thumper_index);
}


void pbauto_heartbeat_transaction_ongoing(pubnub_t const* pb)
{
    PUBNUB_ASSERT_OPT(pb_valid_ctx_ptr(pb));

    if (pb->thumperIndex != UNASSIGNED) {
        switch (pb->trans) {
        case PBTT_HEARTBEAT :
            if (is_exempted(pb, pb->thumperIndex)) {
                break;
            }
            /*FALLTHRU*/
        case PBTT_SUBSCRIBE :
        case PBTT_SUBSCRIBE_V2 :
            stop_auto_heartbeat(pb->thumperIndex);
            break;
        default:
            break;
        }
    }
}


void pbauto_heartbeat_stop(void)
{
    pthread_mutex_lock(&m_watcher.stoplock);
    m_watcher.stop_heartbeat_watcher_thread = true;
    pthread_mutex_unlock(&m_watcher.stoplock);
}
