/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"
#include "pubnub_assert.h"

#include "pbpal.h"


static struct pubnub_ m_aCtx[PUBNUB_CTX_MAX];


bool pb_valid_ctx_ptr(pubnub_t const *pb)
{
    return ((pb >= m_aCtx) && (pb < m_aCtx + PUBNUB_CTX_MAX));
}


pubnub_t *pballoc_get_ctx(unsigned idx)
{
    if (idx >= PUBNUB_CTX_MAX) {
        return NULL;
    }
    return m_aCtx + idx;
}


pubnub_t *pubnub_alloc(void)
{
    size_t i;

    for (i = 0; i < PUBNUB_CTX_MAX; ++i) {
        pubnub_mutex_lock(m_aCtx[i].monitor);
        if (m_aCtx[i].state == PBS_NULL) {
            m_aCtx[i].state = PBS_IDLE;
            pubnub_mutex_unlock(m_aCtx[i].monitor);
            return m_aCtx + i;
        }
        pubnub_mutex_unlock(m_aCtx[i].monitor);
    }

    return NULL;
}


void pballoc_free_at_last(pubnub_t *pb)
{
    PUBNUB_ASSERT_OPT(pb != NULL);

    pubnub_mutex_lock(pb->monitor);

    PUBNUB_ASSERT_OPT(pb->state == PBS_NULL);

    pbcc_deinit(&pb->core);
    pbpal_free(pb);
    pubnub_mutex_unlock(pb->monitor);
    pubnub_mutex_destroy(pb->monitor);
}


int pubnub_free(pubnub_t *pb)
{
    int result = -1;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (PBS_IDLE == pb->state) {
        if (PUBNUB_CALLBACK_API) {
            pbntf_requeue_for_processing(pb);
            pubnub_mutex_unlock(pb->monitor);
        }
        else {
            pubnub_mutex_unlock(pb->monitor);
            pballoc_free_at_last(pb);
        }
        result = 0;
    }
    else {
        pubnub_mutex_unlock(pb->monitor);
    }

    return result;
}
