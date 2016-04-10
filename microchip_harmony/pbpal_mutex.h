/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PBPAL_MUTEX
#define      INC_PBPAL_MUTEX

/** @file pbpal_mutex.h
    PAL for mutex(es) on Harmony.
    This should use `OSAL_`, but, for now, we simply don't do anything
    about mutexes on Harmony.
 */


typedef int pbpal_mutex_t;

#define pbpal_mutex_init(m) 
#define pbpal_mutex_lock(m) 
#define pbpal_mutex_unlock(m) 
#define pbpal_mutex_destroy(m) 
#define pbpal_mutex_decl_and_init(m) 
#define pbpal_mutex_static_decl_and_init(m) 
#define pbpal_mutex_init_static(m) 


#endif /*!defined INC_PBPAL_MUTEX*/

