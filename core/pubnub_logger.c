/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */


/**
 * @file  pubnub_logger.c
 * @brief Implementation of the PubNub Logger system
 */


#include "pubnub_logger.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "core/pubnub_logger_internal.h"
#include "pubnub_internal.h"


// ----------------------------------------------
//                   Functions
// ----------------------------------------------

pubnub_logger_t* pubnub_logger_alloc(
    struct pubnub_logger_interface const* vtable,
    void*                                 user_data)
{
    if (NULL == vtable) return NULL;

    pubnub_logger_t* logger = (pubnub_logger_t*)malloc(sizeof(pubnub_logger_t));
    if (NULL == logger) { return NULL; }

    logger->user_data = user_data;
    logger->vtable    = vtable;
    logger->next      = NULL;

    return logger;
}

void pubnub_logger_free(pubnub_logger_t** logger)
{
    if (NULL == logger || NULL == *logger) return;

    if (NULL != (*logger)->vtable && NULL != (*logger)->vtable->destroy)
        (*logger)->vtable->destroy(*logger);

    free(*logger);
    *logger = NULL;
}

void* pubnub_logger_user_data(pubnub_logger_t* logger)
{
    if (NULL == logger) return NULL;

    return logger->user_data;
}

int pubnub_logger_add(const pubnub_t* pb, struct pubnub_logger* logger)
{
    if (NULL == pb || NULL == logger) return -1;

    return pbcc_logger_manager_logger_add(pb->core.logger_manager, logger);
}

int pubnub_logger_remove(const pubnub_t* pb, struct pubnub_logger* logger)
{
    if (NULL == pb || NULL == logger) return -1;

    return pbcc_logger_manager_logger_remove(pb->core.logger_manager, logger);
}

void pubnub_logger_remove_all(const pubnub_t* pb)
{
    if (NULL == pb) return;

    return pbcc_logger_manager_logger_remove_all(pb->core.logger_manager);
}

void pubnub_logger_set_log_level(
    const pubnub_t*             pb,
    const enum pubnub_log_level level)
{
    if (NULL == pb) return;

    pbcc_logger_manager_set_log_level(pb->core.logger_manager, level);
}

enum pubnub_log_level pubnub_logger_log_level(pubnub_t const* pb)
{
    if (NULL == pb) return PUBNUB_LOG_LEVEL_NONE;

    return pbcc_logger_manager_log_level(pb->core.logger_manager);
}

bool pubnub_logger_should_log(
    const pubnub_t*             pb,
    const enum pubnub_log_level level)
{
    if (NULL == pb) return false;

    return pbcc_logger_manager_should_log(pb->core.logger_manager, level);
}

void pubnub_log_text(
    const pubnub_t*             pb,
    const enum pubnub_log_level level,
    char const*                 location,
    char const*                 message)
{
    if (NULL == pb) return;

    pbcc_logger_manager_log_text(
        pb->core.logger_manager, level, location, message);
}

void pubnub_log_text_formatted(
    const pubnub_t*             pb,
    const enum pubnub_log_level level,
    char const*                 location,
    char const*                 format,
    ...)
{
    if (NULL == pb || NULL == format) return;

    char    buffer[1024];
    va_list args;

    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    pbcc_logger_manager_log_text(
        pb->core.logger_manager, level, location, buffer);
}

void pubnub_log_object(
    const pubnub_t*             pb,
    const enum pubnub_log_level level,
    char const*                 location,
    pubnub_log_value_t const*   message,
    char const*                 details)
{
    if (NULL == pb) return;

    pbcc_logger_manager_log_object(
        pb->core.logger_manager, level, location, message, details);
}

void pubnub_log_error(
    const pubnub_t* pb,
    char const*     location,
    const int       error_code,
    char const*     error_message,
    char const*     details)
{
    if (NULL == pb) return;

    pbcc_logger_manager_log_error(
        pb->core.logger_manager,
        PUBNUB_LOG_LEVEL_ERROR,
        location,
        error_code,
        error_message,
        details);
}

void pubnub_log_network_request(
    const pubnub_t*               pb,
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
    if (NULL == pb) return;

    pbcc_logger_manager_log_network_request(
        pb->core.logger_manager,
        level,
        location,
        method,
        url,
        headers,
        body,
        details,
        canceled,
        failed);
}

void pubnub_log_network_response(
    const pubnub_t*             pb,
    const enum pubnub_log_level level,
    char const*                 location,
    char const*                 url,
    const int                   status_code,
    pubnub_log_value_t const*   headers,
    char const*                 body)
{
    if (NULL == pb) return;

    pbcc_logger_manager_log_network_response(
        pb->core.logger_manager,
        level,
        location,
        url,
        status_code,
        headers,
        body);
}