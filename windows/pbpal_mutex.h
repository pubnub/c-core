/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PBPAL_MUTEX
#define      INC_PBPAL_MUTEX

/** @file pbpal_mutex.h
    PAL for mutex(es) on Windows / Critical Section(s)
 */

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


#endif /*!defined INC_PBPAL_MUTEX*/

