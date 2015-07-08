/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
//#include "pubnub.h"

#include "pubnub_internal.h"
#include "pubnub_assert.h"


static struct pubnub m_aCtx[PUBNUB_CTX_MAX];


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
        if (m_aCtx[i].state == PBS_NULL) {
            m_aCtx[i].state = PBS_IDLE;
            return m_aCtx + i;
        }
    }
    return NULL;
}


int pubnub_free(pubnub_t *pb)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    if (PBS_IDLE == pb->state) {
        pb->state = PBS_NULL;
        return 0;
    }
    return -1;
}
