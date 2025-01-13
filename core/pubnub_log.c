/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_log.h"

#if PUBNUB_USE_LOG_CALLBACK

#include <stddef.h>
#include <stdarg.h>
#include <stdlib.h>

void (*pubnub_log_callback)(enum pubnub_log_level log_level, const char* message) = NULL;

void pubnub_set_log_callback(void (*callback)(enum pubnub_log_level log_level, const char* message)) {
    pubnub_log_callback = callback;
}

void log_with_callback(enum pubnub_log_level log_level, const char* format, ...) {
    //Check needed buffer size for the message
    va_list args;
    va_start(args, format);
    int needed_size = vsnprintf(NULL, 0, format, args) + 1;
    va_end(args);

    if(needed_size <= 0) {return;}

    //Allocate required size to the message
    char* logMessage = (char*)malloc(needed_size);
    if (!logMessage) {return;}

    //Format string with message and all parameters
    va_start(args, format);
    vsnprintf(logMessage, needed_size, format, args);
    va_end(args);

    //Execute the callback
    pubnub_log_callback(log_level, logMessage);
    free(logMessage);
}

#endif /* PUBNUB_USE_LOG_CALLBACK */