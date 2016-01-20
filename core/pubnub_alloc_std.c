/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"
#include "pubnub_assert.h"

#include <stdlib.h>
#include <string.h>


#if defined PUBNUB_ASSERT_LEVEL_EX
static pubnub_t **m_allocated;
static unsigned m_n;
static unsigned m_cap;
pubnub_mutex_static_decl_and_init(m_lock);
#endif


static void save_allocated(pubnub_t *pb)
{
#if defined PUBNUB_ASSERT_LEVEL_EX
    pubnub_mutex_init_static(m_lock);
    pubnub_mutex_lock(m_lock);
    if (m_n == m_cap) {
        pubnub_t **npalloc = (pubnub_t**)realloc(m_allocated, sizeof m_allocated[0] * (m_n+1));
        if (NULL == npalloc) {
            pubnub_mutex_unlock(m_lock);
            return;
        }
        m_allocated = npalloc;
        m_allocated[m_n++] = pb;
        m_cap = m_n;
    }
    else {
        m_allocated[m_n++] = pb;
    }
    pubnub_mutex_unlock(m_lock);
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


static bool check_ctx_ptr(pubnub_t const *pb)
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


bool pb_valid_ctx_ptr(pubnub_t const *pb)
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
    int result = -1;

    pubnub_mutex_lock(pb->monitor);
    pubnub_mutex_init_static(m_lock);
    pubnub_mutex_lock(m_lock);
    PUBNUB_ASSERT(check_ctx_ptr(pb));
    if (PBS_IDLE == pb->state) {
        pbcc_deinit(&pb->core);
        remove_allocated(pb);
        pubnub_mutex_unlock(m_lock);
        pubnub_mutex_unlock(pb->monitor);
        pubnub_mutex_destroy(pb->monitor);
        free(pb);
        result = 0;
    }
    else {
        pubnub_mutex_unlock(m_lock);
        pubnub_mutex_unlock(pb->monitor);
    }

    return result;
}
