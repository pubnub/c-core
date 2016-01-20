/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PBPAL_MUTEX
#define      INC_PBPAL_MUTEX

/** @file pbpal_mutex.h
    PAL for mutex(es) on OpenSSL.
 */

#if defined(_WIN32)

#include <windows.h>

typedef pthread_mutex_t pbpal_mutex_t;

#define pbpal_mutex_init(m) InitializeCriticalSection(&(m))
#define pbpal_mutex_lock(m) EnterCriticalSection(&(m))
#define pbpal_mutex_unlock(m) LeaveCriticalSection(&(m))
#define pbpal_mutex_destroy(m) DeleteCriticalSection(&(m))
static inline int pubnub_InitCriticalSection(_Out_ LPCRITICAL_SECTION lpCS) {
  InitializeCriticalSection(lpCS);
  return 0;
}

#define pbpal_mutex_decl_and_init(m) CRITICAL_SECTION m; const int M_dummy_##m = pubnub_InitCriticalSection(&m)

#define pbpal_mutex_static_decl_and_init(m) static CRITICAL_SECTION m; static const int M_dummy_##m = pubnub_InitCriticalSection(&m)

#else

#include <pthread.h>

typedef pthread_mutex_t pbpal_mutex_t;

#define pbpal_mutex_init(m) pthread_mutex_init(&(m), NULL)
#define pbpal_mutex_lock(m) pthread_mutex_lock(&(m))
#define pbpal_mutex_unlock(m) pthread_mutex_unlock(&(m))
#define pbpal_mutex_destroy(m) pthread_mutex_destroy(&(m))
#define pbpal_mutex_decl_and_init(m) pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER
#define pbpal_mutex_static_decl_and_init(m) static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER

#endif /* defined(_WIN32) */


#endif /*!defined INC_PBPAL_MUTEX*/

