/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"
#include "pubnub_auto_heartbeat.h"
#include "pubnub_assert.h"
#ifdef PUBNUB_NTF_RUNTIME_SELECTION
#include "pubnub_ntf_enforcement.h"
#endif
#include "pbpal.h"

#include <stdlib.h>
#include <string.h>


#if defined PUBNUB_ASSERT_LEVEL_EX
static pubnub_t** m_allocated;
static unsigned   m_n;
static unsigned   m_cap;
pubnub_mutex_static_decl_and_init(m_lock);
#endif


static void save_allocated(pubnub_t* pb)
{
#if defined PUBNUB_ASSERT_LEVEL_EX
    pubnub_mutex_init_static(m_lock);
    pubnub_mutex_lock(m_lock);
    if (m_n == m_cap) {
        pubnub_t** npalloc =
            (pubnub_t**)realloc(m_allocated, sizeof m_allocated[0] * (m_n + 1));
        if (NULL == npalloc) {
#if PUBNUB_LOG_ENABLED(ERROR)
            pubnub_log_error(
                pb,
                PUBNUB_LOG_LOCATION,
                PNR_OUT_OF_MEMORY,
                "Allocated PubNub context bookkeeping allocation failed",
                "Insufficient memory error");
#endif // PUBNUB_LOG_ENABLED(ERROR)
            pubnub_mutex_unlock(m_lock);
            return;
        }
        m_allocated        = npalloc;
        m_allocated[m_n++] = pb;
        m_cap              = m_n;
    }
    else {
        m_allocated[m_n++] = pb;
    }
    pubnub_mutex_unlock(m_lock);
#endif
}


static void remove_allocated(pubnub_t* pb)
{
#if defined PUBNUB_ASSERT_LEVEL_EX
    size_t i;
    for (i = 0; i < m_n; ++i) {
        if (m_allocated[i] == pb) {
            if (i != m_n - 1) {
                memmove(
                    m_allocated + i,
                    m_allocated + i + 1,
                    sizeof m_allocated[0] * (m_n - i - 1));
            }
            if (0 == --m_n) {
                free(m_allocated);
                m_allocated = NULL;
                m_cap       = 0;
            }
            break;
        }
    }
#endif
}


static bool check_ctx_ptr(pubnub_t const* pb)
{
#if defined PUBNUB_ASSERT_LEVEL_EX
    size_t i;
    for (i = 0; i < m_n; ++i) {
        if (m_allocated[i] == pb) { return true; }
    }
    return false;
#else
    return pb != NULL;
#endif
}


bool pb_valid_ctx_ptr(pubnub_t const* pb)
{
#if defined PUBNUB_ASSERT_LEVEL_EX
    bool result;

    pubnub_mutex_init_static(m_lock);
    pubnub_mutex_lock(m_lock);
    result = check_ctx_ptr(pb);
    pubnub_mutex_unlock(m_lock);

    return result;
#else
    return pb != NULL;
#endif
}


pubnub_t* pubnub_alloc(void)
{
    pubnub_t* pb = (pubnub_t*)malloc(sizeof(pubnub_t));
    if (pb != NULL) { save_allocated(pb); }
    return pb;
}


void pballoc_free_at_last(pubnub_t* pb)
{
    PUBNUB_LOG_TRACE(pb, "Freeing PubNub context (final)");

    PUBNUB_ASSERT_OPT(pb != NULL);

    pubnub_mutex_lock(pb->monitor);
    pubnub_mutex_init_static(m_lock);
    pubnub_mutex_lock(m_lock);

    PUBNUB_ASSERT_OPT(pb->state == PBS_NULL);

    pbcc_deinit(&pb->core);
    pbpal_free(pb);
    remove_allocated(pb);
    pubnub_mutex_unlock(pb->monitor);
    pubnub_mutex_destroy(pb->monitor);
#if !defined(PUBNUB_CALLBACK_API) || defined(PUBNUB_NTF_RUNTIME_SELECTION)
    pubnub_mutex_destroy(pb->cancel_monitor);
#endif
#if PUBNUB_USE_AUTO_HEARTBEAT
    pubnub_mutex_destroy(pb->thumper_monitor);
#endif
    pubnub_mutex_unlock(m_lock);
    free(pb);
}


int pubnub_free(pubnub_t* pb)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    PUBNUB_LOG_TRACE(pb, "Try to free PubNub context");

    if (pb->state == PBS_NULL) {
        PUBNUB_LOG_TRACE(pb, "PubNub context not initialized. Freeing...");
        remove_allocated(pb);
        free(pb);

        return 0;
    }

    pubnub_mutex_lock(pb->monitor);
    pbnc_stop(pb, PNR_CANCELLED);
    if (PBS_IDLE == pb->state) {
        PUBNUB_LOG_TRACE(pb, "PubNub context is in idle state. Freeing...");
        pubnub_disable_auto_heartbeat(pb);
        pbauto_heartbeat_free_channelInfo(pb);
        pb->state = PBS_NULL;
#if PUBNUB_USE_LOGGER
        pb->previous_state     = pb->state;
        pb->previous_io_result = PB_IO_RESULT_NONE;
#endif // #if PUBNUB_USE_LOGGER
#if defined(PUBNUB_NTF_RUNTIME_SELECTION)
        if (PNA_CALLBACK == pb->api_policy) {
            pbntf_requeue_for_processing(pb);
            pubnub_mutex_unlock(pb->monitor);
        }
        else {
            pubnub_mutex_unlock(pb->monitor);
            pballoc_free_at_last(pb);
        }
#elif defined(PUBNUB_CALLBACK_API)
        pbntf_requeue_for_processing(pb);
        pubnub_mutex_unlock(pb->monitor);
#else
        pubnub_mutex_unlock(pb->monitor);
        pballoc_free_at_last(pb);
#endif

        return 0;
    }

    PUBNUB_LOG_TRACE(
        pb,
        "PubNub context is still busy in %s state.,",
        pbcc_state_2_string(pb->state));
    pubnub_mutex_unlock(pb->monitor);

    return -1;
}
