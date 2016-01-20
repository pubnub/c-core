/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PBPAL_MUTEX
#define      INC_PBPAL_MUTEX

/** @file pbpal_mutex.h
    PAL for mutex(es) on FreeRTOS
 */

#include "semphr.h"

typedef SemaphoreHandle_t pbpal_mutex_t;

#define pbpal_mutex_init(m) m = xSemaphoreCreateMutex();
#define pbpal_mutex_lock(m) xSemaphoreTake(m, portMAX_DELAY)
#define pbpal_mutex_unlock(m) xSemaphoreGive(m)
#define pbpal_mutex_destroy(m) vSemaphoreDelete(m)
#define pbpal_mutex_decl_and_init(m) SemaphoreHandle_t m = xSemaphoreCreateMutex()
#define pbpal_mutex_static_decl_and_init(m) static SemaphoreHandle_t m; static int m_init_##m
#define pbpal_mutex_init_static(m) do { \
    taskENTER_CRITICAL(); \
    if (0 == m_init_##m) { \
        m = xSemaphoreCreateMutex(); \
        m_init_##m = 1; \
    } \
    taskEXIT_CRITICAL(); \
} while(0)


#endif /*!defined INC_PBPAL_MUTEX*/

