/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PBPAL_MUTEX
#define      INC_PBPAL_MUTEX

/** @file pbpal_mutex.h
    PAL for mutex(es) on OpenSSL.
 */

#if defined(_WIN32)

#include <windows.h>

typedef CRITICAL_SECTION pbpal_mutex_t;

#define pbpal_mutex_init(m) InitializeCriticalSection(&(m))
#define pbpal_mutex_lock(m) EnterCriticalSection(&(m))
#define pbpal_mutex_unlock(m) LeaveCriticalSection(&(m))
#define pbpal_mutex_destroy(m) DeleteCriticalSection(&(m))
__inline int pubnub_InitCriticalSection(_Out_ LPCRITICAL_SECTION lpCS) {
  InitializeCriticalSection(lpCS);
  return 0;
}

#define pbpal_mutex_decl_and_init(m) CRITICAL_SECTION m; const int M_dummy_##m = pubnub_InitCriticalSection(&m)

#define pbpal_mutex_static_decl_and_init(m) static CRITICAL_SECTION m; static volatile LONG m_init_##m

#define pbpal_mutex_init_static(m) do { if (0 == InterlockedExchange(&m_init_##m, 1)) InitializeCriticalSection(&m); } while(0)
#define pbpal_thread_id() DONT_CALL_ME_ON_WINDOWS_
#define pbpal_mutex_init_std(m) InitializeCriticalSection(&(m))

#else

#include <pthread.h>

typedef pthread_mutex_t pbpal_mutex_t;

#define pbpal_mutex_init(m) do {                                     \
        pthread_mutexattr_t M_attr;                                  \
        pthread_mutexattr_init(&M_attr);                             \
        pthread_mutexattr_settype(&M_attr, PTHREAD_MUTEX_RECURSIVE); \
        pthread_mutex_init(&(m), &M_attr);                           \
    } while (0)
#define pbpal_mutex_lock(m) pthread_mutex_lock(&(m))
#define pbpal_mutex_unlock(m) pthread_mutex_unlock(&(m))
#define pbpal_mutex_destroy(m) pthread_mutex_destroy(&(m))
#define pbpal_mutex_decl_and_init(m) pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER
#define pbpal_mutex_static_decl_and_init(m) static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER
#define pbpal_mutex_init_static(m)
#define pbpal_thread_id() pthread_self()
#define pbpal_mutex_init_std(m)  pthread_mutex_init(&(m), NULL)

#endif /* defined(_WIN32) */


#endif /*!defined INC_PBPAL_MUTEX*/

