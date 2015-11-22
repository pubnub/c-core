/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_assert.h"

//#include <stdlib.h>
//#include <stdio.h>
#include <FreeRTOS.h>
#include "task.h"
#include "FreeRTOS_IP.h"


static pubnub_assert_handler_t m_handler;


void pubnub_assert_set_handler(pubnub_assert_handler_t handler)
{
    if (handler == NULL) {
        handler = pubnub_assert_handler_abort;
    }
    m_handler = handler;
}


void pubnub_assert_failed(char const *s, char const *file, long line)
{
    if (m_handler == NULL) {
        m_handler = pubnub_assert_handler_abort;
    }
    m_handler(s, file, line);
}


static void report(char const *s, char const *file, long line)
{
    FreeRTOS_printf(("Pubnub assert failed '%s', file '%s', line %ld\n", s, file, line));
}


void pubnub_assert_handler_loop(char const *s, char const *file, long line)
{
    report(s, file, line);
    for (;;) continue;
}


void pubnub_assert_handler_abort(char const *s, char const *file, long line)
{
    report(s, file, line);

    /* There is no real `abort()` in FreeRTOS, so, we just wait in a loop,
        but disable the interrupts first, which will stop all normal
        processing.
        */
    taskDISABLE_INTERRUPTS();
	{
        /** In the debugger, user can change the value of this
            variable to get out of the loop and continue processing.
            */
        volatile int stay_blocked = 1;
        while (stay_blocked) {
#if INCLUDE_vTaskDelay
            vTaskDelay(pdMS_TO_TICKS(1000));
#endif
        }
	}
	taskENABLE_INTERRUPTS();
}

void pubnub_assert_handler_printf(char const *s, char const *file, long line)
{
    report(s, file, line);
}
