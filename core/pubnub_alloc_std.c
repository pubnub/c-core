/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"
#include "pubnub_assert.h"

#include <stdlib.h>
#include <string.h>


#if defined PUBNUB_ASSERT_LEVEL_EX
static pubnub_t **m_allocated;
static unsigned m_n;
static unsigned m_cap;
#endif



static void save_allocated(pubnub_t *pb)
{
#if defined PUBNUB_ASSERT_LEVEL_EX
    if (m_n == m_cap) {
        pubnub_t **npalloc = (pubnub_t**)realloc(m_allocated, sizeof m_allocated[0] * (m_n+1));
        if (NULL == npalloc) {
            return;
        }
        m_allocated = npalloc;
        m_allocated[m_n++] = pb;
        m_cap = m_n;
    }
    else {
        m_allocated[m_n++] = pb;
    }
#endif
}


static void remove_allocated(pubnub_t *pb)
{
#if defined PUBNUB_ASSERT_LEVEL_EX
    size_t i;
    for (i = 0; i < m_n; ++i) {
        if (m_allocated[i] == pb) {
            if (i != m_n - 1) {
                memmove(m_allocated + i, m_allocated + i + 1, sizeof m_allocated[0] * (m_n - i - 1));
            }
            --m_n;
            break;
        }
    }
#endif
}


bool pb_valid_ctx_ptr(pubnub_t const *pb)
{
#if defined PUBNUB_ASSERT_LEVEL_EX
    size_t i;
    for (i = 0; i < m_n; ++i) {
        if (m_allocated[i] == pb) {
            return true;
        }
    }
    return false;
#else
    return pb != NULL; 
#endif
}


pubnub_t *pubnub_alloc(void)
{
    pubnub_t *pb = (pubnub_t*)malloc(sizeof(pubnub_t));
    if (pb != NULL) {
        save_allocated(pb);
    }
    return pb;
}


int pubnub_free(pubnub_t *pb)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    if (PBS_IDLE == pb->state) {
        remove_allocated(pb);
        free(pb);
        return 0;
    }
    return -1;
}
