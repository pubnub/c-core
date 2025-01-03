/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_log.h"
#include <stddef.h>

void pubnub_setLogCallback(void (*callback)(enum pubnub_log_level log_level, const char* message))
{
    pubnub_logCallback = callback;
}

void (*pubnub_logCallback)(enum pubnub_log_level log_level, const char* message) = NULL;
