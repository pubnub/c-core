/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_assert.h"

#include "lib/assert.h"

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


void pubnub_assert_handler_loop(char const *s, char const *file, long line)
{
    _xassert(file, line);
    for (;;) continue;
}


void pubnub_assert_handler_abort(char const *s, char const *file, long line)
{
    _xassert(file, line);
    // Can't really abort Contiki...
}

void pubnub_assert_handler_printf(char const *s, char const *file, long line)
{
    _xassert(file, line);
}
