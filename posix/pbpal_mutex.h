/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PBPAL_MUTEX
#define      INC_PBPAL_MUTEX

/** @file pbpal_mutex.h
    PAL for mutex(es) on POSIX / pthreads.
 */

#include <pthread.h>

typedef pthread_mutex_t pbpal_mutex_t;

#define pbpal_mutex_init(m) do {                                     \
        pthread_mutexattr_t M_attr;                                  \
        pthread_mutexattr_init(&M_attr);                             \
        pthread_mutexattr_settype(&M_attr, PTHREAD_MUTEX_RECURSIVE); \
        pthread_mutex_init(&(m), &M_attr);                           \
        pthread_mutexattr_destroy(&M_attr);                          \
    } while (0)
#define pbpal_mutex_lock(m) pthread_mutex_lock(&(m))
#define pbpal_mutex_unlock(m) pthread_mutex_unlock(&(m))
#define pbpal_mutex_destroy(m) pthread_mutex_destroy(&(m))
#define pbpal_mutex_decl_and_init(m) pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER
#define pbpal_mutex_static_decl_and_init(m) static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER
#define pbpal_mutex_init_static(m)

#define pbpal_mutex_static_recursive_decl_and_init(m)       \
    static pthread_mutex_t m;                               \
    static pthread_once_t m##_once_ = PTHREAD_ONCE_INIT;    \
    static void m##_init_fn_(void) {                        \
        pthread_mutexattr_t M_attr_;                        \
        pthread_mutexattr_init(&M_attr_);                   \
        pthread_mutexattr_settype(&M_attr_,                 \
                                  PTHREAD_MUTEX_RECURSIVE); \
        pthread_mutex_init(&(m), &M_attr_);                 \
        pthread_mutexattr_destroy(&M_attr_);                \
    }
#define pbpal_mutex_init_static_recursive(m) \
    pthread_once(&m##_once_, m##_init_fn_)


#endif /*!defined INC_PBPAL_MUTEX*/

