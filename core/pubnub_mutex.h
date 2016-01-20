/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_MUTEX
#define      INC_PUBNUB_MUTEX


#if PUBNUB_THREADSAFE
#include "pbpal_mutex.h"


#define pubnub_mutex_t pbpal_mutex_t
#define pubnub_mutex_init(m) pbpal_mutex_init(m)
#define pubnub_mutex_lock(m) pbpal_mutex_lock(m)
#define pubnub_mutex_unlock(m) pbpal_mutex_unlock(m)
#define pubnub_mutex_destroy(m) pbpal_mutex_destroy(m)
#define pubnub_mutex_decl_and_init(m) pbpal_mutex_decl_and_init(m)
#define pubnub_mutex_static_decl_and_init(m) pbpal_mutex_static_decl_and_init(m)
#define pubnub_mutex_init_static(m) pbpal_mutex_init_static(m)

#else

typedef struct { int dummy; } pubnub_mutex_t;
#define pubnub_mutex_init(m)
#define pubnub_mutex_lock(m)
#define pubnub_mutex_unlock(m)
#define pubnub_mutex_destroy(m)
#define pubnub_mutex_decl_and_init(m)
#define pubnub_mutex_static_decl_and_init(m)
#define pubnub_mutex_init_static(m)

#endif

#endif /* !defined  INC_PUBNUB_MUTEX */
