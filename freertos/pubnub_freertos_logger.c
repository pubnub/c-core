/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

/**
 * @file pubnub_freertos_logger.c
 * @brief FreeRTOS-compatible logger implementation for PubNub C-core SDK.
 *
 * Drop-in replacement for pubnub_stdio_logger.c on FreeRTOS targets.
 * Implements the same pubnub_default_logger_alloc() API using
 * FreeRTOS-compatible primitives:
 *
 *  - printf() instead of fprintf(FILE*, ...) — output goes wherever the
 *    platform routes printf (typically UART or semihosting).
 *  - <time.h> (standard C / newlib) for wall-clock timestamp formatting
 *    when available; falls back to xTaskGetTickCount() when the system
 *    clock has not been set (time_t == 0).
 *  - No dependency on <sys/time.h> or other POSIX headers.
 *
 * @note All log levels are routed to printf(). There is no stdout/stderr
 *       distinction since most FreeRTOS targets have a single output channel.
 */

#include "pubnub_default_logger.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include <FreeRTOS.h>
#include <task.h>

#if PUBNUB_RECEIVE_GZIP_RESPONSE || PUBNUB_USE_GZIP_COMPRESSION
#include "lib/miniz/miniz_tinfl.h"
#endif


// ----------------------------------------------
//                   Constants
// ----------------------------------------------

/**
 * @brief How many bytes needed to represent date for entry (including trailing
 *        space and `\0`).
 *
 * Format: `0000-00-00T00:00:00.000Z `
 *
 * Used for the temporary buffer in freertos_logger_write_timestamp_()
 */
#define DATE_TIME_LENGTH 26


// ----------------------------------------------
//              Function prototypes
// ----------------------------------------------

static void freertos_logger_trace_(const pubnub_logger_t*      logger,
                                   const pubnub_log_message_t* message);
static void freertos_logger_debug_(const pubnub_logger_t*      logger,
                                   const pubnub_log_message_t* message);
static void freertos_logger_info_(const pubnub_logger_t*      logger,
                                  const pubnub_log_message_t* message);
static void freertos_logger_warn_(const pubnub_logger_t*      logger,
                                  const pubnub_log_message_t* message);
static void freertos_logger_error_(const pubnub_logger_t*      logger,
                                   const pubnub_log_message_t* message);
static void freertos_logger_destroy_(pubnub_logger_t* logger);

static void freertos_logger_print_text_message_(
    const pubnub_log_message_text_t* message);
static void freertos_logger_print_object_message_(
    const pubnub_log_message_object_t* message);
static void freertos_logger_print_error_message_(
    const pubnub_log_message_error_t* message);
static void freertos_logger_print_network_request_message_(
    const pubnub_log_message_network_request_t* message);
static void freertos_logger_print_network_response_message_(
    const pubnub_log_message_network_response_t* message);

static void freertos_logger_write_timestamp_(
    const pubnub_log_message_t* message);
static void freertos_logger_write_log_level_(enum pubnub_log_level level);
static void freertos_logger_write_entry_prefix_(
    const pubnub_log_message_t* message);
static void freertos_logger_stringify_value_(
    pubnub_log_value_t const* value,
    char const*               indent,
    int                       depth);
static char const* freertos_logger_http_method_to_string_(
    enum pubnub_http_method method);
static bool freertos_logger_is_text_content_type_(char const* content_type);
static bool freertos_logger_should_display_body_as_text_(
    pubnub_log_value_t const* headers);
static bool freertos_logger_is_gzip_encoded_(
    pubnub_log_value_t const* headers);
static size_t freertos_logger_get_content_length_(
    pubnub_log_value_t const* headers);
static void freertos_logger_print_body_(pubnub_log_value_t const* headers,
                                        char const*               body);


// ----------------------------------------------
//                     Types
// ----------------------------------------------

static struct pubnub_logger_interface const g_freertos_logger_vtable = {
    freertos_logger_trace_,
    freertos_logger_debug_,
    freertos_logger_info_,
    freertos_logger_warn_,
    freertos_logger_error_,
    freertos_logger_destroy_
};


// ----------------------------------------------
//                   Functions
// ----------------------------------------------

pubnub_logger_t* pubnub_default_logger_alloc(void)
{
    return pubnub_logger_alloc(&g_freertos_logger_vtable, NULL);
}


void freertos_logger_write_timestamp_(const pubnub_log_message_t* message)
{
    if (0 == message->timestamp.seconds) {
        /* No wall-clock time available — use FreeRTOS tick count. */
        TickType_t ticks = xTaskGetTickCount();
        uint32_t   ms    = (uint32_t)(ticks * portTICK_PERIOD_MS);
        uint32_t   secs  = ms / 1000u;
        uint32_t   frac  = ms % 1000u;

        printf("%lu.%03luZ ", (unsigned long)secs, (unsigned long)frac);
        return;
    }

    const uint32_t msec = message->timestamp.milliseconds;
    const time_t   sec  = message->timestamp.seconds;
    struct tm      tm_utc;
    bool           success = false;

#if defined(ESP_PLATFORM)
    success = (gmtime_r(&sec, &tm_utc) != NULL);
#else
    {
        struct tm* ptm = gmtime(&sec);
        if (ptm) {
            tm_utc  = *ptm;
            success = true;
        }
    }
#endif

    if (!success) {
        /* Fallback: raw timestamp. */
        printf("%lu.%03uZ ",
               (unsigned long)message->timestamp.seconds,
               (unsigned)msec);
        return;
    }

    char         buf[DATE_TIME_LENGTH];
    const size_t date_written =
        strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tm_utc);

    if (date_written == 0) {
        printf("%lu.%03uZ ",
               (unsigned long)message->timestamp.seconds,
               (unsigned)msec);
        return;
    }

    printf("%s.%03uZ ", buf, (unsigned)msec);
}


void freertos_logger_write_log_level_(const enum pubnub_log_level level)
{
    switch (level) {
    case PUBNUB_LOG_LEVEL_TRACE:
        printf("TRACE ");
        break;
    case PUBNUB_LOG_LEVEL_DEBUG:
        printf("DEBUG ");
        break;
    case PUBNUB_LOG_LEVEL_INFO:
        printf("INFO  ");
        break;
    case PUBNUB_LOG_LEVEL_WARNING:
        printf("WARN  ");
        break;
    case PUBNUB_LOG_LEVEL_ERROR:
        printf("ERROR ");
        break;
    case PUBNUB_LOG_LEVEL_NONE:
    default:
        printf("NONE  ");
        break;
    }
}

void freertos_logger_write_entry_prefix_(const pubnub_log_message_t* message)
{
    if (NULL == message) { return; }

    freertos_logger_write_timestamp_(message);
    printf("PubNub-%s ", message->pubnub_id);
    freertos_logger_write_log_level_(message->level);

    if (NULL != message->location) {
        printf("%s ", message->location);
    }
}


void freertos_logger_print_text_message_(
    const pubnub_log_message_text_t* message)
{
    freertos_logger_write_entry_prefix_(&message->base);
    printf("%s\n", message->message ? message->message : "(null)");

}

void freertos_logger_stringify_value_(pubnub_log_value_t const* value,
                                      char const*               indent,
                                      const int                 depth)
{
    const int MAX_DEPTH = 10;

    if (NULL == value) {
        printf("%snull\n", indent);
        return;
    }

    if (depth > MAX_DEPTH) {
        printf("%s...\n", indent);
        return;
    }

    const enum pubnub_log_value_type type = pubnub_log_value_type(value);

    switch (type) {
    case PUBNUB_LOG_VALUE_NULL:
        printf("%snull\n", indent);
        break;
    case PUBNUB_LOG_VALUE_BOOL: {
        bool val;

        if (pubnub_log_value_get_bool(value, &val) == 0) {
            printf("%s%s\n", indent, val ? "true" : "false");
        }
        break;
    }
    case PUBNUB_LOG_VALUE_NUMBER: {
        double val;

        if (pubnub_log_value_get_number(value, &val) == 0) {
            if (val == (long long)val)
                printf("%s%lld\n", indent, (long long)val);
            else
                printf("%s%g\n", indent, val);
        }
        break;
    }
    case PUBNUB_LOG_VALUE_STRING: {
        const char* str = pubnub_log_value_get_string(value);

        if (str)
            printf("%s%s\n", indent, str);
        else
            printf("%s(null string)\n", indent);
        break;
    }
    case PUBNUB_LOG_VALUE_ARRAY: {
        const pubnub_log_value_t* element = pubnub_log_value_first(value);

        if (NULL == element) {
            printf("%s[]\n", indent);
            break;
        }

        char new_indent[256];
        snprintf(new_indent, sizeof(new_indent), "%s  ", indent);

        while (element) {
            char elem_indent[256];
            snprintf(elem_indent, sizeof(elem_indent), "%s- ", new_indent);

            freertos_logger_stringify_value_(element, elem_indent, depth + 1);
            element = pubnub_log_value_next(element);
        }
        break;
    }
    case PUBNUB_LOG_VALUE_MAP: {
        const pubnub_log_value_t* element = pubnub_log_value_first(value);

        if (NULL == element) {
            printf("%s{}\n", indent);
            break;
        }

        char new_indent[256];
        snprintf(new_indent, sizeof(new_indent), "%s  ", indent);

        while (element) {
            const char* key = pubnub_log_value_key(element);

            if (key) {
                const enum pubnub_log_value_type val_type =
                    pubnub_log_value_type(element);

                if (val_type == PUBNUB_LOG_VALUE_MAP
                    || val_type == PUBNUB_LOG_VALUE_ARRAY) {
                    printf("%s%s:\n", new_indent, key);
                    freertos_logger_stringify_value_(
                        element, new_indent, depth + 1);
                }
                else {
                    char kv_indent[256];
                    snprintf(
                        kv_indent, sizeof(kv_indent), "%s%s: ", new_indent, key);
                    freertos_logger_stringify_value_(
                        element, kv_indent, depth + 1);
                }
            }

            element = pubnub_log_value_next(element);
        }
        break;
    }
    default:
        printf("%s(unknown type)\n", indent);
        break;
    }
}

void freertos_logger_print_object_message_(
    const pubnub_log_message_object_t* message)
{
    freertos_logger_write_entry_prefix_(&message->base);
    if (message->details) {
        printf("%s\n", message->details);
    }

    if (NULL == message->message) {
        printf("(null)\n");
    
        return;
    }

    freertos_logger_stringify_value_(message->message, "", 0);

}

void freertos_logger_print_error_message_(
    const pubnub_log_message_error_t* message)
{
    freertos_logger_write_entry_prefix_(&message->base);
    printf("Error: %s\n",
           message->error_message ? message->error_message : "(no message)");

    if (message->error_code != 0)
        printf("  Code: %d\n", message->error_code);
    if (message->details)
        printf("  Details: %s\n", message->details);

}


char const* freertos_logger_http_method_to_string_(
    const enum pubnub_http_method method)
{
    switch (method) {
    case PUBNUB_HTTP_METHOD_GET:
        return "GET";
    case PUBNUB_HTTP_METHOD_POST:
        return "POST";
    case PUBNUB_HTTP_METHOD_DELETE:
        return "DELETE";
    case PUBNUB_HTTP_METHOD_PATCH:
        return "PATCH";
    case PUBNUB_HTTP_METHOD_PUT:
        return "PUT";
    default:
        return "UNKNOWN";
    }
}


bool freertos_logger_is_text_content_type_(char const* content_type)
{
    if (NULL == content_type) { return false; }

    const char* text_markers[] = { "json", "javascript",
                                   "xml",  "html",
                                   "text", "application/x-www-form-urlencoded",
                                   NULL };

    for (int i = 0; text_markers[i] != NULL; i++) {
        if (strstr(content_type, text_markers[i]) != NULL) { return true; }
    }

    return false;
}

bool freertos_logger_should_display_body_as_text_(
    pubnub_log_value_t const* headers)
{
    if (NULL == headers) return false;

    pubnub_log_value_t const* content_type =
        pubnub_log_value_map_get(headers, "Content-Type");
    if (NULL == content_type) return false;

    char const* content_type_str = pubnub_log_value_get_string(content_type);
    return freertos_logger_is_text_content_type_(content_type_str);
}

bool freertos_logger_is_gzip_encoded_(pubnub_log_value_t const* headers)
{
    if (NULL == headers) return false;

    pubnub_log_value_t const* encoding =
        pubnub_log_value_map_get(headers, "Content-Encoding");
    if (NULL == encoding) return false;

    char const* value = pubnub_log_value_get_string(encoding);
    return value != NULL && strstr(value, "gzip") != NULL;
}

size_t freertos_logger_get_content_length_(pubnub_log_value_t const* headers)
{
    if (NULL == headers) return 0;

    pubnub_log_value_t const* cl =
        pubnub_log_value_map_get(headers, "Content-Length");
    if (NULL == cl) return 0;

    char const* str = pubnub_log_value_get_string(cl);
    if (NULL == str) return 0;

    return (size_t)strtoul(str, NULL, 10);
}

void freertos_logger_print_body_(pubnub_log_value_t const* headers,
                                  char const*               body)
{
    if (NULL == body) return;

#if PUBNUB_RECEIVE_GZIP_RESPONSE || PUBNUB_USE_GZIP_COMPRESSION
    if (freertos_logger_is_gzip_encoded_(headers)) {
        const size_t body_length = freertos_logger_get_content_length_(headers);
        if (0 == body_length) {
            printf("  Body: <gzip-compressed, unknown length>\n");
            return;
        }

        const size_t         GZIP_HDR = 10;
        const size_t         GZIP_FTR = 8;
        const unsigned char* raw      = (const unsigned char*)body;

        if (body_length > GZIP_HDR + GZIP_FTR
            && raw[0] == 0x1f && raw[1] == 0x8b) {
            size_t inflated_len = 0;
            void*  inflated     = tinfl_decompress_mem_to_heap(
                raw + GZIP_HDR,
                body_length - GZIP_HDR - GZIP_FTR,
                &inflated_len,
                0);

            if (inflated && inflated_len > 0) {
                printf("  Body:\n");
                printf("    %.*s\n", (int)inflated_len, (char const*)inflated);
                free(inflated);
                return;
            }
            free(inflated);
        }
        printf("  Body: <%zu bytes gzip-compressed>\n", body_length);
        return;
    }
#endif /* PUBNUB_RECEIVE_GZIP_RESPONSE || PUBNUB_USE_GZIP_COMPRESSION */

    if (freertos_logger_should_display_body_as_text_(headers)) {
        printf("  Body:\n");
        printf("    %s\n", body);
    }
}

void freertos_logger_print_network_request_message_(
    const pubnub_log_message_network_request_t* message)
{
    const bool show_trace =
        message->base.minimum_level <= PUBNUB_LOG_LEVEL_TRACE;
    const bool  show_minimal = message->canceled || message->failed;
    const char* action =
        !show_minimal ? "Sending" : (message->canceled ? "Canceled" : "Failed");

    freertos_logger_write_entry_prefix_(&message->base);

    printf("%s", action);
    if (show_minimal && NULL != message->details)
        printf(" (%s)", message->details);

    printf(" HTTP request:\n");
    printf("  Method: %s\n",
           freertos_logger_http_method_to_string_(message->method));
    printf("  URL: %s\n", message->url ? message->url : "(no URL)");

    if (message->headers && show_trace && !show_minimal) {
        printf("  Headers:\n");

        pubnub_log_value_t const* element =
            pubnub_log_value_first(message->headers);
        while (element) {
            char const* key = pubnub_log_value_key(element);

            if (key) {
                char const* value = pubnub_log_value_get_string(element);
                if (value)
                    printf("    - %s: %s\n", key, value);
            }

            element = pubnub_log_value_next(element);
        }
    }

    if (message->body && show_trace && !show_minimal) {
        freertos_logger_print_body_(message->headers, message->body);
    }

}

void freertos_logger_print_network_response_message_(
    const pubnub_log_message_network_response_t* message)
{
    const bool show_trace =
        message->base.minimum_level <= PUBNUB_LOG_LEVEL_TRACE;

    freertos_logger_write_entry_prefix_(&message->base);
    printf("Received HTTP response:\n");
    printf("  Status: %d\n", message->status_code);

    if (message->headers && show_trace) {
        printf("  Headers:\n");

        pubnub_log_value_t const* element =
            pubnub_log_value_first(message->headers);
        while (element) {
            char const* key = pubnub_log_value_key(element);

            if (key) {
                char const* val_str = pubnub_log_value_get_string(element);
                if (val_str) {
                    printf("    - %s: %s\n", key, val_str);
                }
            }

            element = pubnub_log_value_next(element);
        }
    }

    if (message->body && show_trace) {
        freertos_logger_print_body_(message->headers, message->body);
    }

}


/* Dispatch message to the appropriate printer. */
static void print_log_message(const pubnub_log_message_t* message)
{
    switch (message->message_type) {
    case PUBNUB_LOG_MESSAGE_TYPE_TEXT:
        freertos_logger_print_text_message_(
            (struct pubnub_log_message_text const*)message);
        break;
    case PUBNUB_LOG_MESSAGE_TYPE_OBJECT:
        freertos_logger_print_object_message_(
            (struct pubnub_log_message_object const*)message);
        break;
    case PUBNUB_LOG_MESSAGE_TYPE_ERROR:
        freertos_logger_print_error_message_(
            (struct pubnub_log_message_error const*)message);
        break;
    case PUBNUB_LOG_MESSAGE_TYPE_NETWORK_REQUEST:
        freertos_logger_print_network_request_message_(
            (struct pubnub_log_message_network_request const*)message);
        break;
    case PUBNUB_LOG_MESSAGE_TYPE_NETWORK_RESPONSE:
        freertos_logger_print_network_response_message_(
            (struct pubnub_log_message_network_response const*)message);
        break;
    default:
        printf("Unknown message type: %d\n", message->message_type);
        break;
    }
}

static void freertos_logger_trace_(const pubnub_logger_t*      logger,
                                   const pubnub_log_message_t* message)
{
    (void)logger;
    print_log_message(message);
}

static void freertos_logger_debug_(const pubnub_logger_t*      logger,
                                   const pubnub_log_message_t* message)
{
    (void)logger;
    print_log_message(message);
}

static void freertos_logger_info_(const pubnub_logger_t*      logger,
                                  const pubnub_log_message_t* message)
{
    (void)logger;
    print_log_message(message);
}

static void freertos_logger_warn_(const pubnub_logger_t*      logger,
                                  const pubnub_log_message_t* message)
{
    (void)logger;
    print_log_message(message);
}

static void freertos_logger_error_(const pubnub_logger_t*      logger,
                                   const pubnub_log_message_t* message)
{
    (void)logger;
    print_log_message(message);
}

static void freertos_logger_destroy_(pubnub_logger_t* logger)
{
    (void)logger;
}
