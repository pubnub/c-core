/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_default_logger.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#if PUBNUB_RECEIVE_GZIP_RESPONSE || PUBNUB_USE_GZIP_COMPRESSION
#include "lib/miniz/miniz_tinfl.h"
#endif
#if defined(_WIN32)
#include <windows.h>
#if !defined(strdup)
#define strdup _strdup
#endif
#else
#include <sys/time.h>
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
 * Used for the temporary buffer in stdio_logger_write_timestamp_()
 */
#define DATE_TIME_LENGTH 26


// ----------------------------------------------
//              Function prototypes
// ----------------------------------------------

/* Forward declarations of logger functions */
static void stdio_logger_trace_(const pubnub_logger_t*      logger,
                                const pubnub_log_message_t* message);
static void stdio_logger_debug_(const pubnub_logger_t*      logger,
                                const pubnub_log_message_t* message);
static void stdio_logger_info_(const pubnub_logger_t*      logger,
                               const pubnub_log_message_t* message);
static void stdio_logger_warn_(const pubnub_logger_t*      logger,
                               const pubnub_log_message_t* message);
static void stdio_logger_error_(const pubnub_logger_t*      logger,
                                const pubnub_log_message_t* message);
static void stdio_logger_destroy_(pubnub_logger_t* logger);

/**
 * @brief Write plain text message to the @p stream.
 *
 * @param stream  Target stream for @p message entry output.
 * @param message Log entry which should be written to the @p stream.
 */
static void stdio_logger_print_text_message_(FILE* stream,
                                             const pubnub_log_message_text_t* message);

/**
 * @brief Write object / structured message to the @p stream.
 *
 * @param stream  Target stream for @p message entry output.
 * @param message Log entry which should be written to the @p stream.
 */
static void stdio_logger_print_object_message_(FILE* stream,
                                               const pubnub_log_message_object_t* message);

/**
 * @brief Write error message to the @p stream.
 *
 * @param stream  Target stream for @p message entry output.
 * @param message Log entry which should be written to the @p stream.
 */
static void stdio_logger_print_error_message_(FILE* stream,
                                              const pubnub_log_message_error_t* message);

/**
 * @brief Write network request message to the @p stream.
 *
 * @param stream  Target stream for @p message entry output.
 * @param message Log entry which should be written to the @p stream.
 */
static void stdio_logger_print_network_request_message_(
    FILE*                                       stream,
    const pubnub_log_message_network_request_t* message);

/**
 * @brief Write network response message to the @p stream.
 *
 * @param stream  Target stream for @p message entry output.
 * @param message Log entry which should be written to the @p stream.
 */
static void stdio_logger_print_network_response_message_(
    FILE*                                        stream,
    const pubnub_log_message_network_response_t* message);

/**
 * @brief Write log entry timestamp directly to the stream.
 *
 * @param stream  Target stream for timestamp output.
 * @param message Log entry for which timestamp should be written.
 */
static void stdio_logger_write_timestamp_(FILE*                       stream,
                                          const pubnub_log_message_t* message);

/**
 * @brief Write log entry log level directly to the stream.
 *
 * @param stream Target stream for log level output.
 * @param level  One of `pubnub_log_level` enum message log levels.
 *
 * @see pubnub_log_level
 */
static void stdio_logger_write_log_level_(FILE* stream, enum pubnub_log_level level);

/**
 * @brief Write log entry prefix directly to the stream.
 *
 * @param stream  Target stream for prefix output.
 * @param message Log entry for which prefix should be written.
 */
static void stdio_logger_write_entry_prefix_(FILE* stream,
                                             const pubnub_log_message_t* message);

/**
 * @brief Recursively stringify a log value with indentation.
 *
 * @param stream  Target stream for output.
 * @param value   The value to stringify.
 * @param indent  Current indentation string.
 * @param depth   Current nesting depth (to limit recursion).
 */
static void stdio_logger_stringify_value_(FILE*                     stream,
                                          pubnub_log_value_t const* value,
                                          char const*               indent,
                                          int                       depth);

/**
 * @brief Convert HTTP method enum to string.
 *
 * @param method HTTP method enum.
 * @return String representation of the method.
 */
static char const* stdio_logger_http_method_to_string_(enum pubnub_http_method method);

/**
 * @brief Check if Content-Type indicates text-based content.
 *
 * Checks if the `Content-Type` header value contains markers indicating
 * text-based content (`json`, `javascript`, `xml`, `html`, `text`, etc.).
 *
 * @param content_type Content-Type header value (can be `NULL`).
 * @return true if content type indicates text-based content, false otherwise.
 */
static bool pubnub_log_value_is_text_content_type(char const* content_type);

/**
 * @brief Check if body should be displayed as text based on Content-Type.
 *
 * @param headers Headers map to check for Content-Type.
 * @return true if body should be displayed as text, false otherwise.
 */
static bool stdio_logger_should_display_body_as_text_(pubnub_log_value_t const* headers);

/**
 * @brief Check if Content-Encoding header indicates gzip compression.
 *
 * @param headers Headers map to check.
 * @return true if Content-Encoding is gzip, false otherwise.
 */
static bool stdio_logger_is_gzip_encoded_(pubnub_log_value_t const* headers);

/**
 * @brief Get body length from Content-Length header.
 *
 * @param headers Headers map to read Content-Length from.
 * @return Body length, or 0 if header is absent or invalid.
 */
static size_t stdio_logger_get_content_length_(pubnub_log_value_t const* headers);

/**
 * @brief Print body to stream, inflating gzip-compressed data if needed.
 *
 * If Content-Encoding is gzip, reads Content-Length from headers and attempts
 * to decompress the body before printing. Falls back to printing byte count
 * on decompression failure.
 *
 * @param stream  Target stream for output.
 * @param headers Headers map (checked for Content-Encoding and Content-Length).
 * @param body    Body data (may be binary if gzip-compressed).
 */
static void stdio_logger_print_body_(FILE*                     stream,
                                     pubnub_log_value_t const* headers,
                                     char const*               body);


// ----------------------------------------------
//                     Types
// ----------------------------------------------

/* Default logger vtable */
static struct pubnub_logger_interface const g_stdio_logger_vtable = {
    .trace   = stdio_logger_trace_,
    .debug   = stdio_logger_debug_,
    .info    = stdio_logger_info_,
    .warn    = stdio_logger_warn_,
    .error   = stdio_logger_error_,
    .destroy = stdio_logger_destroy_
};


// ----------------------------------------------
//                   Functions
// ----------------------------------------------

pubnub_logger_t* pubnub_default_logger_alloc(void)
{
    return pubnub_logger_alloc(&g_stdio_logger_vtable, NULL);
}

void stdio_logger_write_timestamp_(FILE* stream, const pubnub_log_message_t* message)
{
    static char const* timestamp_placeholder = "0000-00-00T00:00:00.000Z ";
    if (0 == message->timestamp.seconds) {
        fprintf(stream, "%s", timestamp_placeholder);
        return;
    }

    const uint32_t msec = message->timestamp.milliseconds;
    const time_t   sec  = message->timestamp.seconds;
    struct tm      tm_utc;
    bool           success = false;

#if defined(_WIN32)
    success = (gmtime_s(&tm_utc, &sec) == 0);
#else /* POSIX / ESP32 */
#if defined(__unix__) || defined(__APPLE__) || defined(__ORBIS__)              \
    || defined(__psp2__)
    success = (gmtime_r(&sec, &tm_utc) != NULL);
#else
    /* Fallback (very rare) */
    struct tm* ptm = gmtime(&sec);
    if (ptm) {
        tm_utc  = *ptm;
        success = true;
    }
#endif
#endif

    if (!success) {
        fprintf(stream, "%s", timestamp_placeholder);
        return;
    }

    /* Format date and time */
    char         buf[DATE_TIME_LENGTH];
    const size_t date_written =
        strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tm_utc);

    if (date_written == 0) {
        fprintf(stream, "%s", timestamp_placeholder);
        return;
    }

    fprintf(stream, "%s.%03uZ ", buf, msec);
}


void stdio_logger_write_log_level_(FILE* stream, const enum pubnub_log_level level)
{
    switch (level) {
    case PUBNUB_LOG_LEVEL_TRACE:
        fprintf(stream, "TRACE ");
        break;
    case PUBNUB_LOG_LEVEL_DEBUG:
        fprintf(stream, "DEBUG ");
        break;
    case PUBNUB_LOG_LEVEL_INFO:
        fprintf(stream, "INFO  ");
        break;
    case PUBNUB_LOG_LEVEL_WARNING:
        fprintf(stream, "WARN  ");
        break;
    case PUBNUB_LOG_LEVEL_ERROR:
        fprintf(stream, "ERROR ");
        break;
    case PUBNUB_LOG_LEVEL_NONE:
    default:
        fprintf(stream, "NONE  ");
        break;
    }
}

void stdio_logger_write_entry_prefix_(FILE*                       stream,
                                      const pubnub_log_message_t* message)
{
    /* Guard against NULL message */
    if (NULL == message) {
        return;
    }

    /* Write timestamp */
    stdio_logger_write_timestamp_(stream, message);

    /* Write PubNub context identifier */
    fprintf(stream, "PubNub-%s ", message->pubnub_id);

    /* Write log level */
    stdio_logger_write_log_level_(stream, message->level);

    /* Write location information */
    if (NULL != message->location) {
        fprintf(stream, "%s ", message->location);
    }
}


void stdio_logger_print_text_message_(FILE*                            stream,
                                      const pubnub_log_message_text_t* message)
{
    stdio_logger_write_entry_prefix_(stream, &message->base);
    fprintf(stream, "%s\n", message->message ? message->message : "(null)");
    fflush(stream);
}

void stdio_logger_stringify_value_(FILE*                     stream,
                                   pubnub_log_value_t const* value,
                                   char const*               indent,
                                   const int                 depth)
{
    const int MAX_DEPTH = 10;

    if (NULL == value) {
        fprintf(stream, "%snull\n", indent);
        return;
    }

    if (depth > MAX_DEPTH) {
        fprintf(stream, "%s...\n", indent);
        return;
    }

    const enum pubnub_log_value_type type = pubnub_log_value_type(value);

    switch (type) {
    case PUBNUB_LOG_VALUE_NULL:
        fprintf(stream, "%snull\n", indent);
        break;
    case PUBNUB_LOG_VALUE_BOOL: {
        bool val;

        if (pubnub_log_value_get_bool(value, &val) == 0) {
            fprintf(stream, "%s%s\n", indent, val ? "true" : "false");
        }
        break;
    }
    case PUBNUB_LOG_VALUE_NUMBER: {
        double val;

        if (pubnub_log_value_get_number(value, &val) == 0) {
            if (val == (long long)val)
                fprintf(stream, "%s%lld\n", indent, (long long)val);
            else
                fprintf(stream, "%s%g\n", indent, val);
        }
        break;
    }
    case PUBNUB_LOG_VALUE_STRING: {
        const char* str = pubnub_log_value_get_string(value);

        if (str)
            fprintf(stream, "%s%s\n", indent, str);
        else
            fprintf(stream, "%s(null string)\n", indent);
        break;
    }
    case PUBNUB_LOG_VALUE_ARRAY: {
        const pubnub_log_value_t* element = pubnub_log_value_first(value);

        if (NULL == element) {
            fprintf(stream, "%s[]\n", indent);
            break;
        }

        char new_indent[256];
        snprintf(new_indent, sizeof(new_indent), "%s  ", indent);

        while (element) {
            char elem_indent[256];
            snprintf(elem_indent, sizeof(elem_indent), "%s- ", new_indent);

            stdio_logger_stringify_value_(stream, element, elem_indent, depth + 1);
            element = pubnub_log_value_next(element);
        }
        break;
    }
    case PUBNUB_LOG_VALUE_MAP: {
        const pubnub_log_value_t* element = pubnub_log_value_first(value);

        if (NULL == element) {
            fprintf(stream, "%s{}\n", indent);
            break;
        }

        /* Create new indent for nested items (add 2 spaces) */
        char new_indent[256];
        snprintf(new_indent, sizeof(new_indent), "%s  ", indent);

        /* Print each key-value pair */
        while (element) {
            const char* key = pubnub_log_value_key(element);

            if (key) {
                const enum pubnub_log_value_type val_type =
                    pubnub_log_value_type(element);

                if (val_type == PUBNUB_LOG_VALUE_MAP
                    || val_type == PUBNUB_LOG_VALUE_ARRAY) {
                    fprintf(stream, "%s%s:\n", new_indent, key);
                    stdio_logger_stringify_value_(
                        stream, element, new_indent, depth + 1);
                }
                else {
                    char kv_indent[256];
                    snprintf(kv_indent, sizeof(kv_indent), "%s%s: ", new_indent, key);
                    stdio_logger_stringify_value_(
                        stream, element, kv_indent, depth + 1);
                }
            }

            element = pubnub_log_value_next(element);
        }
        break;
    }
    default:
        fprintf(stream, "%s(unknown type)\n", indent);
        break;
    }
}

void stdio_logger_print_object_message_(FILE* stream,
                                        const pubnub_log_message_object_t* message)
{
    /* Print prefix and details on first line */
    stdio_logger_write_entry_prefix_(stream, &message->base);
    if (message->details) {
        fprintf(stream, "%s\n", message->details);
    }

    /* Check for null message */
    if (NULL == message->message) {
        fprintf(stream, "(null)\n");
        fflush(stream);
        return;
    }

    /* Stringify the structured value (no prefix, already printed) */
    stdio_logger_stringify_value_(stream, message->message, "", 0);

    fflush(stream);
}

void stdio_logger_print_error_message_(FILE* stream,
                                       const pubnub_log_message_error_t* message)
{
    /* Print prefix and error header on first line */
    stdio_logger_write_entry_prefix_(stream, &message->base);
    fprintf(stream,
            "Error: %s\n",
            message->error_message ? message->error_message : "(no message)");

    /* Subsequent lines without prefix */
    if (message->error_code != 0)
        fprintf(stream, "  Code: %d\n", message->error_code);
    if (message->details)
        fprintf(stream, "  Details: %s\n", message->details);

    fflush(stream);
}

/* Convert HTTP method enum to string */
char const* stdio_logger_http_method_to_string_(const enum pubnub_http_method method)
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

bool pubnub_log_value_is_text_content_type(char const* content_type)
{
    if (NULL == content_type) {
        return false;
    }

    /* Check for common text-based content types */
    const char* text_markers[] = { "json", "javascript",
                                   "xml",  "html",
                                   "text", "application/x-www-form-urlencoded",
                                   NULL };

    for (int i = 0; text_markers[i] != NULL; i++) {
        if (strstr(content_type, text_markers[i]) != NULL) {
            return true;
        }
    }

    return false;
}

bool stdio_logger_should_display_body_as_text_(pubnub_log_value_t const* headers)
{
    if (NULL == headers)
        return false;

    pubnub_log_value_t const* content_type =
        pubnub_log_value_map_get(headers, "Content-Type");
    if (NULL == content_type)
        return false;

    char const* content_type_str = pubnub_log_value_get_string(content_type);
    return pubnub_log_value_is_text_content_type(content_type_str);
}

bool stdio_logger_is_gzip_encoded_(pubnub_log_value_t const* headers)
{
    if (NULL == headers)
        return false;

    pubnub_log_value_t const* encoding =
        pubnub_log_value_map_get(headers, "Content-Encoding");
    if (NULL == encoding)
        return false;

    char const* value = pubnub_log_value_get_string(encoding);
    return value != NULL && strstr(value, "gzip") != NULL;
}

size_t stdio_logger_get_content_length_(pubnub_log_value_t const* headers)
{
    if (NULL == headers)
        return 0;

    pubnub_log_value_t const* cl =
        pubnub_log_value_map_get(headers, "Content-Length");
    if (NULL == cl)
        return 0;

    char const* str = pubnub_log_value_get_string(cl);
    if (NULL == str)
        return 0;

    return (size_t)strtoul(str, NULL, 10);
}

void stdio_logger_print_body_(FILE*                     stream,
                              pubnub_log_value_t const* headers,
                              char const*               body)
{
    if (NULL == body)
        return;

#if PUBNUB_RECEIVE_GZIP_RESPONSE || PUBNUB_USE_GZIP_COMPRESSION
    if (stdio_logger_is_gzip_encoded_(headers)) {
        const size_t body_length = stdio_logger_get_content_length_(headers);
        if (0 == body_length) {
            fprintf(stream, "  Body: <gzip-compressed, unknown length>\n");
            return;
        }

        /* Gzip format: 10-byte header + deflate data + 8-byte footer.
           tinfl expects raw deflate, so strip the gzip wrapper. */
        const size_t         GZIP_HDR = 10;
        const size_t         GZIP_FTR = 8;
        const unsigned char* raw      = (const unsigned char*)body;

        if (body_length > GZIP_HDR + GZIP_FTR && raw[0] == 0x1f && raw[1] == 0x8b) {
            size_t inflated_len = 0;
            void*  inflated     = tinfl_decompress_mem_to_heap(
                raw + GZIP_HDR, body_length - GZIP_HDR - GZIP_FTR, &inflated_len, 0);

            if (inflated && inflated_len > 0) {
                fprintf(stream, "  Body:\n");
                fprintf(stream, "    %.*s\n", (int)inflated_len, (char const*)inflated);
                free(inflated);
                return;
            }
            free(inflated);
        }
        fprintf(stream, "  Body: <%zu bytes gzip-compressed>\n", body_length);
        return;
    }
#endif /* PUBNUB_RECEIVE_GZIP_RESPONSE || PUBNUB_USE_GZIP_COMPRESSION */

    if (stdio_logger_should_display_body_as_text_(headers)) {
        fprintf(stream, "  Body:\n");
        fprintf(stream, "    %s\n", body);
    }
}

void stdio_logger_print_network_request_message_(
    FILE*                                       stream,
    const pubnub_log_message_network_request_t* message)
{
    const bool show_trace = message->base.minimum_level <= PUBNUB_LOG_LEVEL_TRACE;
    const bool  show_minimal = message->canceled || message->failed;
    const char* action =
        !show_minimal ? "Sending" : (message->canceled ? "Canceled" : "Failed");

    stdio_logger_write_entry_prefix_(stream, &message->base);

    fprintf(stream, "%s", action);
    if (show_minimal && NULL != message->details)
        fprintf(stream, " (%s)", message->details);

    fprintf(stream, " HTTP request:\n");
    fprintf(stream,
            "  Method: %s\n",
            stdio_logger_http_method_to_string_(message->method));
    fprintf(stream, "  URL: %s\n", message->url ? message->url : "(no URL)");

    if (message->headers && show_trace && !show_minimal) {
        fprintf(stream, "  Headers:\n");

        pubnub_log_value_t const* element =
            pubnub_log_value_first(message->headers);
        while (element) {
            char const* key = pubnub_log_value_key(element);

            if (key) {
                char const* value = pubnub_log_value_get_string(element);
                if (value)
                    fprintf(stream, "    - %s: %s\n", key, value);
            }

            element = pubnub_log_value_next(element);
        }
    }

    if (message->body && show_trace && !show_minimal) {
        stdio_logger_print_body_(stream, message->headers, message->body);
    }

    fflush(stream);
}

void stdio_logger_print_network_response_message_(
    FILE*                                        stream,
    const pubnub_log_message_network_response_t* message)
{
    const bool show_trace = message->base.minimum_level <= PUBNUB_LOG_LEVEL_TRACE;

    /* Print prefix and header on first line */
    stdio_logger_write_entry_prefix_(stream, &message->base);
    fprintf(stream, "Received HTTP response:\n");

    /* Subsequent lines without prefix */
    fprintf(stream, "  Status: %d\n", message->status_code);
    if (message->url) {
        fprintf(stream, "  URL: %s\n", message->url);
    }

    /* Print headers if TRACE level */
    if (message->headers && show_trace) {
        fprintf(stream, "  Headers:\n");

        /* Iterate through headers map */
        pubnub_log_value_t const* element =
            pubnub_log_value_first(message->headers);
        while (element) {
            char const* key = pubnub_log_value_key(element);

            if (key) {
                char const* val_str = pubnub_log_value_get_string(element);
                if (val_str) {
                    fprintf(stream, "    - %s: %s\n", key, val_str);
                }
            }

            element = pubnub_log_value_next(element);
        }
    }

    /* Print body if TRACE level */
    if (message->body && show_trace) {
        stdio_logger_print_body_(stream, message->headers, message->body);
    }

    fflush(stream);
}


/* Helper function to dispatch message to appropriate printer */
static void print_log_message(FILE* stream, const pubnub_log_message_t* message)
{
    switch (message->message_type) {
    case PUBNUB_LOG_MESSAGE_TYPE_TEXT:
        stdio_logger_print_text_message_(
            stream, (struct pubnub_log_message_text const*)message);
        break;
    case PUBNUB_LOG_MESSAGE_TYPE_OBJECT:
        stdio_logger_print_object_message_(
            stream, (struct pubnub_log_message_object const*)message);
        break;
    case PUBNUB_LOG_MESSAGE_TYPE_ERROR:
        stdio_logger_print_error_message_(
            stream, (struct pubnub_log_message_error const*)message);
        break;
    case PUBNUB_LOG_MESSAGE_TYPE_NETWORK_REQUEST:
        stdio_logger_print_network_request_message_(
            stream, (struct pubnub_log_message_network_request const*)message);
        break;
    case PUBNUB_LOG_MESSAGE_TYPE_NETWORK_RESPONSE:
        stdio_logger_print_network_response_message_(
            stream, (struct pubnub_log_message_network_response const*)message);
        break;
    default:
        fprintf(stream, "Unknown message type: %d\n", message->message_type);
        fflush(stream);
        break;
    }
}

static void stdio_logger_trace_(const pubnub_logger_t*      logger,
                                const pubnub_log_message_t* message)
{
    (void)logger;
    print_log_message(stdout, message);
}

static void stdio_logger_debug_(const pubnub_logger_t*      logger,
                                const pubnub_log_message_t* message)
{
    (void)logger;
    print_log_message(stdout, message);
}

static void stdio_logger_info_(const pubnub_logger_t*      logger,
                               const pubnub_log_message_t* message)
{
    (void)logger;
    print_log_message(stdout, message);
}

static void stdio_logger_warn_(const pubnub_logger_t*      logger,
                               const pubnub_log_message_t* message)
{
    (void)logger;
    print_log_message(stdout, message);
}

static void stdio_logger_error_(const pubnub_logger_t*      logger,
                                const pubnub_log_message_t* message)
{
    (void)logger;
    print_log_message(stderr, message);
}

static void stdio_logger_destroy_(pubnub_logger_t* logger)
{
    (void)logger;
}