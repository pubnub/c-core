/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PBPAL_MUTEX
#define      INC_PBPAL_MUTEX

/** @file pbpal_mutex.h
    PAL for mutex(es) on POSIX / pthreads.
 */

#include <pthread.h>

typedef pthread_mutex_t pbpal_mutex_t;

#define pbpal_mutex_init(m) pthread_mutex_init(&(m), NULL)
#define pbpal_mutex_lock(m) pthread_mutex_lock(&(m))
#define pbpal_mutex_unlock(m) pthread_mutex_unlock(&(m))
#define pbpal_mutex_destroy(m) pthread_mutex_destroy(&(m))
#define pbpal_mutex_decl_and_init(m) pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER
#define pbpal_mutex_static_decl_and_init(m) static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER
#define pbpal_mutex_init_static(m)


#endif /*!defined INC_PBPAL_MUTEX*/

