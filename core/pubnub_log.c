/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_log.h"
#include <stddef.h>

void pubnub_set_log_callback(void (*callback)(enum pubnub_log_level log_level, const char* message))
{
    pubnub_log_callback = callback;
}

void (*pubnub_log_callback)(enum pubnub_log_level log_level, const char* message) = NULL;
