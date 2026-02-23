/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

/**
 * @file pubnub_harmony_logger.c
 * @brief Microchip Harmony logger implementation for PubNub C-core SDK.
 *
 * Drop-in replacement for pubnub_stdio_logger.c on Harmony targets.
 * Implements the same pubnub_default_logger_alloc() API using
 * Harmony-compatible primitives:
 *
 *  - SYS_CONSOLE_PRINT() instead of fprintf(FILE*, ...) — output goes
 *    to the Harmony Console Service.
 *  - SYS_TMR_TickCountGet() for elapsed-time timestamps when wall-clock
 *    time is unavailable.
 *  - No dependency on <sys/time.h>, <stdio.h>, stdout, stderr, or other
 *    POSIX / desktop headers.
 */

#include "pubnub_default_logger.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "system/common/sys_module.h"
#include "system_definitions.h"

#if PUBNUB_RECEIVE_GZIP_RESPONSE || PUBNUB_USE_GZIP_COMPRESSION
#include "lib/miniz/miniz_tinfl.h"
#endif


// ----------------------------------------------
//                   Constants
// ----------------------------------------------

#define DATE_TIME_LENGTH 26


// ----------------------------------------------
//              Function prototypes
// ----------------------------------------------

static void harmony_logger_trace_(const pubnub_logger_t*      logger,
                                  const pubnub_log_message_t* message);
static void harmony_logger_debug_(const pubnub_logger_t*      logger,
                                  const pubnub_log_message_t* message);
static void harmony_logger_info_(const pubnub_logger_t*      logger,
                                 const pubnub_log_message_t* message);
static void harmony_logger_warn_(const pubnub_logger_t*      logger,
                                 const pubnub_log_message_t* message);
static void harmony_logger_error_(const pubnub_logger_t*      logger,
                                  const pubnub_log_message_t* message);
static void harmony_logger_destroy_(pubnub_logger_t* logger);

static void harmony_logger_print_text_message_(
    const pubnub_log_message_text_t* message);
static void harmony_logger_print_object_message_(
    const pubnub_log_message_object_t* message);
static void harmony_logger_print_error_message_(
    const pubnub_log_message_error_t* message);
static void harmony_logger_print_network_request_message_(
    const pubnub_log_message_network_request_t* message);
static void harmony_logger_print_network_response_message_(
    const pubnub_log_message_network_response_t* message);

static void harmony_logger_write_timestamp_(
    const pubnub_log_message_t* message);
static void harmony_logger_write_log_level_(enum pubnub_log_level level);
static void harmony_logger_write_entry_prefix_(
    const pubnub_log_message_t* message);
static void harmony_logger_stringify_value_(
    pubnub_log_value_t const* value,
    char const*               indent,
    int                       depth);
static char const* harmony_logger_http_method_to_string_(
    enum pubnub_http_method method);
static bool harmony_logger_is_text_content_type_(char const* content_type);
static bool harmony_logger_should_display_body_as_text_(
    pubnub_log_value_t const* headers);
static bool harmony_logger_is_gzip_encoded_(
    pubnub_log_value_t const* headers);
static size_t harmony_logger_get_content_length_(
    pubnub_log_value_t const* headers);
static void harmony_logger_print_body_(pubnub_log_value_t const* headers,
                                       char const*               body);


// ----------------------------------------------
//                     Types
// ----------------------------------------------

static struct pubnub_logger_interface const g_harmony_logger_vtable = {
    harmony_logger_trace_,
    harmony_logger_debug_,
    harmony_logger_info_,
    harmony_logger_warn_,
    harmony_logger_error_,
    harmony_logger_destroy_
};


// ----------------------------------------------
//                   Functions
// ----------------------------------------------

pubnub_logger_t* pubnub_default_logger_alloc(void)
{
    return pubnub_logger_alloc(&g_harmony_logger_vtable, NULL);
}


void harmony_logger_write_timestamp_(const pubnub_log_message_t* message)
{
    if (0 == message->timestamp.seconds) {
        /* No wall-clock time — use Harmony system timer tick count. */
        uint32_t ticks = SYS_TMR_TickCountGet();
        uint32_t freq  = SYS_TMR_TickCounterFrequencyGet();
        uint32_t ms    = (freq != 0) ? (uint32_t)((uint64_t)ticks * 1000 / freq) : 0;
        uint32_t secs  = ms / 1000u;
        uint32_t frac  = ms % 1000u;

        SYS_CONSOLE_PRINT("%lu.%03luZ ", (unsigned long)secs, (unsigned long)frac);
        return;
    }

    /* Wall-clock time available — format as ISO 8601.
       Harmony does not provide gmtime / strftime, so we format the
       raw Unix timestamp numerically. */
    SYS_CONSOLE_PRINT("%lu.%03uZ ",
                      (unsigned long)message->timestamp.seconds,
                      (unsigned)message->timestamp.milliseconds);
}


void harmony_logger_write_log_level_(const enum pubnub_log_level level)
{
    switch (level) {
    case PUBNUB_LOG_LEVEL_TRACE:
        SYS_CONSOLE_PRINT("TRACE ");
        break;
    case PUBNUB_LOG_LEVEL_DEBUG:
        SYS_CONSOLE_PRINT("DEBUG ");
        break;
    case PUBNUB_LOG_LEVEL_INFO:
        SYS_CONSOLE_PRINT("INFO  ");
        break;
    case PUBNUB_LOG_LEVEL_WARNING:
        SYS_CONSOLE_PRINT("WARN  ");
        break;
    case PUBNUB_LOG_LEVEL_ERROR:
        SYS_CONSOLE_PRINT("ERROR ");
        break;
    case PUBNUB_LOG_LEVEL_NONE:
    default:
        SYS_CONSOLE_PRINT("NONE  ");
        break;
    }
}

void harmony_logger_write_entry_prefix_(const pubnub_log_message_t* message)
{
    if (NULL == message) { return; }

    harmony_logger_write_timestamp_(message);
    SYS_CONSOLE_PRINT("PubNub-%s ", message->pubnub_id);
    harmony_logger_write_log_level_(message->level);

    if (NULL != message->location) {
        SYS_CONSOLE_PRINT("%s ", message->location);
    }
}


void harmony_logger_print_text_message_(
    const pubnub_log_message_text_t* message)
{
    harmony_logger_write_entry_prefix_(&message->base);
    SYS_CONSOLE_PRINT("%s\n", message->message ? message->message : "(null)");
}

void harmony_logger_stringify_value_(pubnub_log_value_t const* value,
                                     char const*               indent,
                                     const int                 depth)
{
    const int MAX_DEPTH = 10;

    if (NULL == value) {
        SYS_CONSOLE_PRINT("%snull\n", indent);
        return;
    }

    if (depth > MAX_DEPTH) {
        SYS_CONSOLE_PRINT("%s...\n", indent);
        return;
    }

    const enum pubnub_log_value_type type = pubnub_log_value_type(value);

    switch (type) {
    case PUBNUB_LOG_VALUE_NULL:
        SYS_CONSOLE_PRINT("%snull\n", indent);
        break;
    case PUBNUB_LOG_VALUE_BOOL: {
        bool val;

        if (pubnub_log_value_get_bool(value, &val) == 0) {
            SYS_CONSOLE_PRINT("%s%s\n", indent, val ? "true" : "false");
        }
        break;
    }
    case PUBNUB_LOG_VALUE_NUMBER: {
        double val;

        if (pubnub_log_value_get_number(value, &val) == 0) {
            if (val == (long long)val)
                SYS_CONSOLE_PRINT("%s%lld\n", indent, (long long)val);
            else
                SYS_CONSOLE_PRINT("%s%g\n", indent, val);
        }
        break;
    }
    case PUBNUB_LOG_VALUE_STRING: {
        const char* str = pubnub_log_value_get_string(value);

        if (str)
            SYS_CONSOLE_PRINT("%s%s\n", indent, str);
        else
            SYS_CONSOLE_PRINT("%s(null string)\n", indent);
        break;
    }
    case PUBNUB_LOG_VALUE_ARRAY: {
        const pubnub_log_value_t* element = pubnub_log_value_first(value);

        if (NULL == element) {
            SYS_CONSOLE_PRINT("%s[]\n", indent);
            break;
        }

        char new_indent[256];
        snprintf(new_indent, sizeof(new_indent), "%s  ", indent);

        while (element) {
            char elem_indent[256];
            snprintf(elem_indent, sizeof(elem_indent), "%s- ", new_indent);

            harmony_logger_stringify_value_(element, elem_indent, depth + 1);
            element = pubnub_log_value_next(element);
        }
        break;
    }
    case PUBNUB_LOG_VALUE_MAP: {
        const pubnub_log_value_t* element = pubnub_log_value_first(value);

        if (NULL == element) {
            SYS_CONSOLE_PRINT("%s{}\n", indent);
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
                    SYS_CONSOLE_PRINT("%s%s:\n", new_indent, key);
                    harmony_logger_stringify_value_(
                        element, new_indent, depth + 1);
                }
                else {
                    char kv_indent[256];
                    snprintf(
                        kv_indent, sizeof(kv_indent), "%s%s: ", new_indent, key);
                    harmony_logger_stringify_value_(
                        element, kv_indent, depth + 1);
                }
            }

            element = pubnub_log_value_next(element);
        }
        break;
    }
    default:
        SYS_CONSOLE_PRINT("%s(unknown type)\n", indent);
        break;
    }
}

void harmony_logger_print_object_message_(
    const pubnub_log_message_object_t* message)
{
    harmony_logger_write_entry_prefix_(&message->base);
    if (message->details) {
        SYS_CONSOLE_PRINT("%s\n", message->details);
    }

    if (NULL == message->message) {
        SYS_CONSOLE_PRINT("(null)\n");
        return;
    }

    harmony_logger_stringify_value_(message->message, "", 0);
}

void harmony_logger_print_error_message_(
    const pubnub_log_message_error_t* message)
{
    harmony_logger_write_entry_prefix_(&message->base);
    SYS_CONSOLE_PRINT(
        "Error: %s\n",
        message->error_message ? message->error_message : "(no message)");

    if (message->error_code != 0)
        SYS_CONSOLE_PRINT("  Code: %d\n", message->error_code);
    if (message->details)
        SYS_CONSOLE_PRINT("  Details: %s\n", message->details);
}


char const* harmony_logger_http_method_to_string_(
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


bool harmony_logger_is_text_content_type_(char const* content_type)
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

bool harmony_logger_should_display_body_as_text_(
    pubnub_log_value_t const* headers)
{
    if (NULL == headers) return false;

    pubnub_log_value_t const* content_type =
        pubnub_log_value_map_get(headers, "Content-Type");
    if (NULL == content_type) return false;

    char const* content_type_str = pubnub_log_value_get_string(content_type);
    return harmony_logger_is_text_content_type_(content_type_str);
}

bool harmony_logger_is_gzip_encoded_(pubnub_log_value_t const* headers)
{
    if (NULL == headers) return false;

    pubnub_log_value_t const* encoding =
        pubnub_log_value_map_get(headers, "Content-Encoding");
    if (NULL == encoding) return false;

    char const* value = pubnub_log_value_get_string(encoding);
    return value != NULL && strstr(value, "gzip") != NULL;
}

size_t harmony_logger_get_content_length_(pubnub_log_value_t const* headers)
{
    if (NULL == headers) return 0;

    pubnub_log_value_t const* cl =
        pubnub_log_value_map_get(headers, "Content-Length");
    if (NULL == cl) return 0;

    char const* str = pubnub_log_value_get_string(cl);
    if (NULL == str) return 0;

    return (size_t)strtoul(str, NULL, 10);
}

void harmony_logger_print_body_(pubnub_log_value_t const* headers,
                                char const*               body)
{
    if (NULL == body) return;

#if PUBNUB_RECEIVE_GZIP_RESPONSE || PUBNUB_USE_GZIP_COMPRESSION
    if (harmony_logger_is_gzip_encoded_(headers)) {
        const size_t body_length = harmony_logger_get_content_length_(headers);
        if (0 == body_length) {
            SYS_CONSOLE_PRINT("  Body: <gzip-compressed, unknown length>\n");
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
                SYS_CONSOLE_PRINT("  Body:\n");
                SYS_CONSOLE_PRINT(
                    "    %.*s\n", (int)inflated_len, (char const*)inflated);
                free(inflated);
                return;
            }
            free(inflated);
        }
        SYS_CONSOLE_PRINT("  Body: <%zu bytes gzip-compressed>\n", body_length);
        return;
    }
#endif /* PUBNUB_RECEIVE_GZIP_RESPONSE || PUBNUB_USE_GZIP_COMPRESSION */

    if (harmony_logger_should_display_body_as_text_(headers)) {
        SYS_CONSOLE_PRINT("  Body:\n");
        SYS_CONSOLE_PRINT("    %s\n", body);
    }
}

void harmony_logger_print_network_request_message_(
    const pubnub_log_message_network_request_t* message)
{
    const bool show_trace =
        message->base.minimum_level <= PUBNUB_LOG_LEVEL_TRACE;
    const bool  show_minimal = message->canceled || message->failed;
    const char* action =
        !show_minimal ? "Sending" : (message->canceled ? "Canceled" : "Failed");

    harmony_logger_write_entry_prefix_(&message->base);

    SYS_CONSOLE_PRINT("%s", action);
    if (show_minimal && NULL != message->details)
        SYS_CONSOLE_PRINT(" (%s)", message->details);

    SYS_CONSOLE_PRINT(" HTTP request:\n");
    SYS_CONSOLE_PRINT(
        "  Method: %s\n",
        harmony_logger_http_method_to_string_(message->method));
    SYS_CONSOLE_PRINT(
        "  URL: %s\n", message->url ? message->url : "(no URL)");

    if (message->headers && show_trace && !show_minimal) {
        SYS_CONSOLE_PRINT("  Headers:\n");

        pubnub_log_value_t const* element =
            pubnub_log_value_first(message->headers);
        while (element) {
            char const* key = pubnub_log_value_key(element);

            if (key) {
                char const* value = pubnub_log_value_get_string(element);
                if (value)
                    SYS_CONSOLE_PRINT("    - %s: %s\n", key, value);
            }

            element = pubnub_log_value_next(element);
        }
    }

    if (message->body && show_trace && !show_minimal) {
        harmony_logger_print_body_(message->headers, message->body);
    }
}

void harmony_logger_print_network_response_message_(
    const pubnub_log_message_network_response_t* message)
{
    const bool show_trace =
        message->base.minimum_level <= PUBNUB_LOG_LEVEL_TRACE;

    harmony_logger_write_entry_prefix_(&message->base);
    SYS_CONSOLE_PRINT("Received HTTP response:\n");
    SYS_CONSOLE_PRINT("  Status: %d\n", message->status_code);

    if (message->headers && show_trace) {
        SYS_CONSOLE_PRINT("  Headers:\n");

        pubnub_log_value_t const* element =
            pubnub_log_value_first(message->headers);
        while (element) {
            char const* key = pubnub_log_value_key(element);

            if (key) {
                char const* val_str = pubnub_log_value_get_string(element);
                if (val_str) {
                    SYS_CONSOLE_PRINT("    - %s: %s\n", key, val_str);
                }
            }

            element = pubnub_log_value_next(element);
        }
    }

    if (message->body && show_trace) {
        harmony_logger_print_body_(message->headers, message->body);
    }
}


/* Dispatch message to the appropriate printer. */
static void print_log_message(const pubnub_log_message_t* message)
{
    switch (message->message_type) {
    case PUBNUB_LOG_MESSAGE_TYPE_TEXT:
        harmony_logger_print_text_message_(
            (struct pubnub_log_message_text const*)message);
        break;
    case PUBNUB_LOG_MESSAGE_TYPE_OBJECT:
        harmony_logger_print_object_message_(
            (struct pubnub_log_message_object const*)message);
        break;
    case PUBNUB_LOG_MESSAGE_TYPE_ERROR:
        harmony_logger_print_error_message_(
            (struct pubnub_log_message_error const*)message);
        break;
    case PUBNUB_LOG_MESSAGE_TYPE_NETWORK_REQUEST:
        harmony_logger_print_network_request_message_(
            (struct pubnub_log_message_network_request const*)message);
        break;
    case PUBNUB_LOG_MESSAGE_TYPE_NETWORK_RESPONSE:
        harmony_logger_print_network_response_message_(
            (struct pubnub_log_message_network_response const*)message);
        break;
    default:
        SYS_CONSOLE_PRINT("Unknown message type: %d\n", message->message_type);
        break;
    }
}

static void harmony_logger_trace_(const pubnub_logger_t*      logger,
                                  const pubnub_log_message_t* message)
{
    (void)logger;
    print_log_message(message);
}

static void harmony_logger_debug_(const pubnub_logger_t*      logger,
                                  const pubnub_log_message_t* message)
{
    (void)logger;
    print_log_message(message);
}

static void harmony_logger_info_(const pubnub_logger_t*      logger,
                                 const pubnub_log_message_t* message)
{
    (void)logger;
    print_log_message(message);
}

static void harmony_logger_warn_(const pubnub_logger_t*      logger,
                                 const pubnub_log_message_t* message)
{
    (void)logger;
    print_log_message(message);
}

static void harmony_logger_error_(const pubnub_logger_t*      logger,
                                  const pubnub_log_message_t* message)
{
    (void)logger;
    print_log_message(message);
}

static void harmony_logger_destroy_(pubnub_logger_t* logger)
{
    (void)logger;
}
