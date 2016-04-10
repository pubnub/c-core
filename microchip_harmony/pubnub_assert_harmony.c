/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_assert.h"

#include "system/common/sys_module.h"   // SYS function prototypes
#include "system_definitions.h" // for SYS_CONSOLE_PRINT

#include <stdlib.h>


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
    SYS_CONSOLE_PRINT("Pubnub assert failed '%s', file '%s', line %ld\n", s, file, line);
}


void pubnub_assert_handler_loop(char const *s, char const *file, long line)
{
    report(s, file, line);
    for (;;) continue;
}


void pubnub_assert_handler_abort(char const *s, char const *file, long line)
{
    report(s, file, line);

    /* There is no real `abort()` in Harmony, we shall (try to) enter the debugger.
     */
    asm("break");
}

void pubnub_assert_handler_printf(char const *s, char const *file, long line)
{
    report(s, file, line);
}
