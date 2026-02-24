/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pbcc_logger_manager.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#if defined(_WIN32)
#include <windows.h>
#elif defined(__unix__) || defined(__APPLE__) || defined(ESP_PLATFORM)
#include <sys/time.h>
#endif

#include "core/pubnub_logger_internal.h"
#include "pubnub_internal.h"
#include "core/pubnub_logger.h"
#include "core/pubnub_mutex.h"


// ----------------------------------------------
//              Internal structures
// ----------------------------------------------

/** Logger manager structure. */
struct pbcc_logger_manager {
    /** Unique identifier of PubNub context which will use this logger. */
    char pubnub_id[9];
    /** Minimum messages log level to be logged. */
    enum pubnub_log_level minimum_level;
#if PUBNUB_USE_DEFAULT_LOGGER
    /** Default logger. */
    pubnub_logger_t* default_logger;
#endif // #if PUBNUB_USE_DEFAULT_LOGGER
    /**
     * @brief List of additional loggers which should be used along with
     *        user-provided custom loggers.
     */
    pubnub_logger_t* loggers;
    /** Shared resources access lock. */
    pubnub_mutex_t mutw;
};


// ----------------------------------------------
//              Function prototypes
// ----------------------------------------------

static void pbcc_logger_manager_log_message(
    const pbcc_logger_manager_t* manager,
    const pubnub_log_message_t*  message);

static pubnub_log_message_t pbcc_logger_manager_message_init(
    const pbcc_logger_manager_t*       manager,
    const enum pubnub_log_message_type message_type,
    const enum pubnub_log_level        level,
    char const*                        location);


// ----------------------------------------------
//                   Functions
// ----------------------------------------------

pbcc_logger_manager_t* pbcc_logger_manager_alloc(const char* pubnub_id)
{
    pbcc_logger_manager_t* manager =
        (pbcc_logger_manager_t*)malloc(sizeof(pbcc_logger_manager_t));
    if (NULL == manager) {
        printf(
            "[PubNub] Logger manager allocation error. Insufficient memory "
            "error.\n");
        return NULL;
    }

    manager->minimum_level = (enum pubnub_log_level)PUBNUB_LOG_MIN_LEVEL_VALUE_;
#if PUBNUB_USE_DEFAULT_LOGGER
    manager->default_logger = NULL;
#endif // #if PUBNUB_USE_DEFAULT_LOGGER
    manager->loggers = NULL;

    // Compressing PubNub context identifier to 8 bytes.
    if (NULL == pubnub_id || '\0' == pubnub_id[0])
        snprintf(manager->pubnub_id, sizeof(manager->pubnub_id), "00000000");
    else {
        uint32_t h = 0x811c9dc5u;
        while (*pubnub_id) {
            h ^= (unsigned char)*pubnub_id++;
            h *= 16777619u;
        }
        snprintf(manager->pubnub_id, sizeof(manager->pubnub_id), "%08x", h);
    }

    pubnub_mutex_init(manager->mutw);

    return manager;
}

void pbcc_logger_manager_free(pbcc_logger_manager_t* manager)
{
    if (NULL == manager) return;

    // Remove all additional loggers.
    pbcc_logger_manager_logger_remove_all(manager);

    pubnub_mutex_lock(manager->mutw);
    manager->loggers = NULL;
#if PUBNUB_USE_DEFAULT_LOGGER
    if (NULL != manager->default_logger)
        pubnub_logger_free(&manager->default_logger);
#endif // #if PUBNUB_USE_DEFAULT_LOGGER
    pubnub_mutex_unlock(manager->mutw);
    pubnub_mutex_destroy(manager->mutw);

    free(manager);
}

#if PUBNUB_USE_DEFAULT_LOGGER
void pbcc_logger_manager_set_default_logger(
    pbcc_logger_manager_t* manager,
    pubnub_logger_t*       logger)
{
    pubnub_mutex_lock(manager->mutw);
    manager->default_logger = logger;
    pubnub_mutex_unlock(manager->mutw);
}
#endif // #if PUBNUB_USE_DEFAULT_LOGGER

int pbcc_logger_manager_logger_add(
    pbcc_logger_manager_t* manager,
    pubnub_logger_t*       logger)
{
    if (NULL == manager || NULL == logger) return -1;

    pubnub_mutex_lock(manager->mutw);
    const pubnub_logger_t* current = manager->loggers;
    while (NULL != current) {
        if (current == logger) {
            pubnub_mutex_unlock(manager->mutw);
            return -1;
        }
        current = current->next;
    }
    logger->next     = manager->loggers;
    manager->loggers = logger;
#if PUBNUB_USE_DEFAULT_LOGGER
    // Make default logger first in a line for messages.
    manager->default_logger->next = manager->loggers;
#endif // #if PUBNUB_USE_DEFAULT_LOGGER
    pubnub_mutex_unlock(manager->mutw);

    return 0;
}

int pbcc_logger_manager_logger_remove(
    pbcc_logger_manager_t* manager,
    pubnub_logger_t*       logger)
{
    if (NULL == manager || NULL == logger) return -1;

    pubnub_mutex_lock(manager->mutw);
    pubnub_logger_t** current = &manager->loggers;
    while (NULL != *current) {
        if (*current == logger) {
            *current     = logger->next;
            logger->next = NULL;
#if PUBNUB_USE_DEFAULT_LOGGER
            // Make default logger first in a line for messages.
            manager->default_logger->next = manager->loggers;
#endif // #if PUBNUB_USE_DEFAULT_LOGGER
            pubnub_mutex_unlock(manager->mutw);
            return 0;
        }
        current = &(*current)->next;
    }
    pubnub_mutex_unlock(manager->mutw);

    return -1;
}

void pbcc_logger_manager_logger_remove_all(pbcc_logger_manager_t* manager)
{
    if (NULL == manager) return;

    pubnub_mutex_lock(manager->mutw);
    pubnub_logger_t* logger = manager->loggers;
    while (NULL != logger) {
        pubnub_logger_t* next_logger = logger->next;
        logger->next                 = NULL;
        logger                       = next_logger;
    }
#if PUBNUB_USE_DEFAULT_LOGGER
    manager->default_logger->next = NULL;
#endif // #if PUBNUB_USE_DEFAULT_LOGGER
    manager->loggers = NULL;
    pubnub_mutex_unlock(manager->mutw);
}

void pbcc_logger_manager_set_log_level(
    pbcc_logger_manager_t*      manager,
    const enum pubnub_log_level level)
{
    if (NULL == manager) return;

    pubnub_mutex_lock(manager->mutw);
    manager->minimum_level = level;
    pubnub_mutex_unlock(manager->mutw);
}

enum pubnub_log_level pbcc_logger_manager_log_level(
    pbcc_logger_manager_t* manager)
{
    if (NULL == manager) return PUBNUB_LOG_LEVEL_NONE;

    pubnub_mutex_lock(manager->mutw);
    const enum pubnub_log_level level = manager->minimum_level;
    pubnub_mutex_unlock(manager->mutw);

    return level;
}

bool pbcc_logger_manager_should_log(
    pbcc_logger_manager_t*      manager,
    const enum pubnub_log_level level)
{
    if (NULL == manager) return false;

    bool will_process = false;
    pubnub_mutex_lock(manager->mutw);
    will_process = level >= manager->minimum_level;
    pubnub_mutex_unlock(manager->mutw);

    return will_process;
}

void pbcc_logger_manager_log_text(
    const pbcc_logger_manager_t* manager,
    const enum pubnub_log_level  level,
    char const*                  location,
    char const*                  message)
{
    /* Check if message should be logged based on minimum level */
    if (NULL == manager || level < manager->minimum_level) return;

    struct pubnub_log_message_text log;
    memset(&log, 0, sizeof(log));
    log.base = pbcc_logger_manager_message_init(
        manager, PUBNUB_LOG_MESSAGE_TYPE_TEXT, level, location);
    log.message = message;

    pbcc_logger_manager_log_message(manager, (const pubnub_log_message_t*)&log);
}

void pbcc_logger_manager_log_text_formatted(
    const pbcc_logger_manager_t* manager,
    const enum pubnub_log_level  level,
    char const*                  location,
    char const*                  format,
    ...)
{
    if (NULL == manager || NULL == format || level < manager->minimum_level)
        return;

    char    buffer[1024];
    va_list args;

    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    pbcc_logger_manager_log_text(manager, level, location, buffer);
}

void pbcc_logger_manager_log_object(
    const pbcc_logger_manager_t* manager,
    const enum pubnub_log_level  level,
    char const*                  location,
    pubnub_log_value_t const*    message,
    char const*                  details)
{
    /* Check if message should be logged based on minimum level */
    if (NULL == manager || level < manager->minimum_level) return;

    struct pubnub_log_message_object log;
    memset(&log, 0, sizeof(log));
    log.base = pbcc_logger_manager_message_init(
        manager, PUBNUB_LOG_MESSAGE_TYPE_OBJECT, level, location);
    log.message = message;
    log.details = details;

    pbcc_logger_manager_log_message(manager, (const pubnub_log_message_t*)&log);
}

void pbcc_logger_manager_log_error(
    const pbcc_logger_manager_t* manager,
    const enum pubnub_log_level  level,
    char const*                  location,
    const int                    error_code,
    char const*                  error_message,
    char const*                  details)
{
    /* Check if message should be logged based on minimum level */
    if (NULL == manager || level < manager->minimum_level) return;
    struct pubnub_log_message_error log;
    memset(&log, 0, sizeof(log));
    log.base = pbcc_logger_manager_message_init(
        manager, PUBNUB_LOG_MESSAGE_TYPE_ERROR, level, location);
    log.error_code    = error_code;
    log.error_message = error_message;
    log.details       = details;

    pbcc_logger_manager_log_message(manager, (const pubnub_log_message_t*)&log);
}

void pbcc_logger_manager_log_network_request(
    const pbcc_logger_manager_t*  manager,
    const enum pubnub_log_level   level,
    char const*                   location,
    const enum pubnub_http_method method,
    char const*                   url,
    pubnub_log_value_t const*     headers,
    char const*                   body,
    char const*                   details,
    const bool                    canceled,
    const bool                    failed)
{
    /* Check if message should be logged based on minimum level */
    if (NULL == manager || level < manager->minimum_level) return;

    struct pubnub_log_message_network_request log;
    memset(&log, 0, sizeof(log));
    log.base = pbcc_logger_manager_message_init(
        manager, PUBNUB_LOG_MESSAGE_TYPE_NETWORK_REQUEST, level, location);
    log.method   = method;
    log.url      = url;
    log.headers  = headers;
    log.body     = body;
    log.details  = details;
    log.canceled = canceled;
    log.failed   = failed;

    pbcc_logger_manager_log_message(manager, (const pubnub_log_message_t*)&log);
}

void pbcc_logger_manager_log_network_response(
    const pbcc_logger_manager_t* manager,
    const enum pubnub_log_level  level,
    char const*                  location,
    char const*                  url,
    const int                    status_code,
    pubnub_log_value_t const*    headers,
    char const*                  body)
{
    /* Check if message should be logged based on minimum level */
    if (NULL == manager || level < manager->minimum_level) return;

    struct pubnub_log_message_network_response log;
    memset(&log, 0, sizeof(log));
    log.base = pbcc_logger_manager_message_init(
        manager, PUBNUB_LOG_MESSAGE_TYPE_NETWORK_RESPONSE, level, location);
    log.url         = url;
    log.status_code = status_code;
    log.headers     = headers;
    log.body        = body;

    pbcc_logger_manager_log_message(manager, (const pubnub_log_message_t*)&log);
}

void pbcc_logger_manager_log_message(
    const pbcc_logger_manager_t* manager,
    const pubnub_log_message_t*  message)
{
#if PUBNUB_USE_DEFAULT_LOGGER
    const pubnub_logger_t* logger = manager->default_logger;
#else  // #if !PUBNUB_USE_DEFAULT_LOGGER
    const pubnub_logger_t* logger = manager->loggers;
#endif // #if PUBNUB_USE_DEFAULT_LOGGER
    while (NULL != logger) {
        if (NULL == logger->vtable) {
            logger = logger->next;
            continue;
        }

        switch (message->level) {
        case PUBNUB_LOG_LEVEL_TRACE:
            if (NULL != logger->vtable->trace)
                logger->vtable->trace(logger, message);
            break;
        case PUBNUB_LOG_LEVEL_DEBUG:
            if (NULL != logger->vtable->debug)
                logger->vtable->debug(logger, message);
            break;
        case PUBNUB_LOG_LEVEL_INFO:
            if (NULL != logger->vtable->info)
                logger->vtable->info(logger, message);
            break;
        case PUBNUB_LOG_LEVEL_WARNING:
            if (NULL != logger->vtable->warn)
                logger->vtable->warn(logger, message);
            break;
        case PUBNUB_LOG_LEVEL_ERROR:
            if (NULL != logger->vtable->error)
                logger->vtable->error(logger, message);
            break;
        default:
            /* Nothing to log */
            break;
        }

        logger = logger->next;
    }
}

pubnub_log_message_t pbcc_logger_manager_message_init(
    const pbcc_logger_manager_t*       manager,
    const enum pubnub_log_message_type message_type,
    const enum pubnub_log_level        level,
    char const*                        location)
{
    time_t   sec;
    uint32_t msec;

#if defined(_WIN32)
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);

    ULARGE_INTEGER uli;
    uli.LowPart                  = ft.dwLowDateTime;
    uli.HighPart                 = ft.dwHighDateTime;
    uint64_t       ms_since_1601 = uli.QuadPart / 10000ULL;
    const uint64_t EPOCH_DIFF_MS = 11644473600000ULL;
    uint64_t       unix_ms       = ms_since_1601 - EPOCH_DIFF_MS;
    sec                          = (time_t)(unix_ms / 1000ULL);
    msec                         = (uint32_t)(unix_ms % 1000ULL);
#elif defined(__unix__) || defined(__APPLE__) || defined(ESP_PLATFORM)
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0) {
        sec  = 0;
        msec = 0;
    }
    else {
        sec  = tv.tv_sec;
        msec = (uint32_t)(tv.tv_usec / 1000UL);
    }
#else
    /* Generic FreeRTOS / other: no wall-clock time source available.
       The platform logger will use its own fallback (e.g. tick count). */
    sec  = 0;
    msec = 0;
#endif

    pubnub_log_message_t message;
    memset(&message, 0, sizeof(message));
    message.message_type          = message_type;
    message.level                 = level;
    message.pubnub_id             = manager->pubnub_id;
    message.timestamp.seconds     = sec;
    message.timestamp.milliseconds = msec;
    message.location              = location;
    message.minimum_level         = manager->minimum_level;

    return message;
}