/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PBPAL_MUTEX
#define      INC_PBPAL_MUTEX

/** @file pbpal_mutex.h
    PAL for mutex(es) on FreeRTOS
 */


typedef SemaphoreHandle_t pbpal_mutex_t;

#define pbpal_mutex_init(m) m = xSemaphoreCreateMutex();
#define pbpal_mutex_lock(m) xSemaphoreTake(m, portMAX_DELAY)
#define pbpal_mutex_unlock(m) xSemaphoreGive(m)
#define pbpal_mutex_destroy(m) vSemaphoreDelete(m)
#define pbpal_mutex_decl_and_init(m) SemaphoreHandle_t m = xSemaphoreCreateMutex()
#define pbpal_mutex_static_decl_and_init(m) static SemaphoreHandle_t m = xSemaphoreCreateMutex()
#define pbpal_mutex_init_static(m)


#endif /*!defined INC_PBPAL_MUTEX*/

