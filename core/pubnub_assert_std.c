/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_assert.h"

#include <stdlib.h>
#include <stdio.h>


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
    printf("Pubnub assert failed '%s', file '%s', line %ld\n", s, file, line);
}


void pubnub_assert_handler_loop(char const *s, char const *file, long line)
{
    report(s, file, line);
    for (;;) continue;
}


void pubnub_assert_handler_abort(char const *s, char const *file, long line)
{
    report(s, file, line);
    abort();
}

void pubnub_assert_handler_printf(char const *s, char const *file, long line)
{
    report(s, file, line);
}
