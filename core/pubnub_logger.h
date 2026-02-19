/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#ifndef PUBNUB_LOGGER_H
#define PUBNUB_LOGGER_H


/**
 * @file pubnub_logger.h
 * @brief PubNub Logger API - Flexible logging system for PubNub C SDK.
 *
 * This module provides a flexible logging system that can be customized by
 * users.
 * Multiple loggers can be registered to a PubNub context, and all registered
 * loggers will receive log messages of appropriate levels.
 */


#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include "core/pubnub_api_types.h"
#include "core/pubnub_log_value.h"
#include "lib/pb_extern.h"


// ----------------------------------------------
//                Types forwarding
// ----------------------------------------------

/** Custom logger definition. */
struct pubnub_logger;
typedef struct pubnub_logger pubnub_logger_t;

/** Log message entry definition. */
struct pubnub_log_message;
typedef struct pubnub_log_message pubnub_log_message_t;


// ----------------------------------------------
//                     Types
// ----------------------------------------------

/**
 * @brief Compile-time `trace` log level value.
 *
 * @note Value used along with pre-processing macro to strip unused `trace` log
 *        calls.
 */
#define PUBNUB_LOG_LEVEL_TRACE_VALUE_ (1 << 0)
/**
 * @brief Compile-time `debug` log level value.
 *
 * @note Value used along with pre-processing macro to strip unused `debug` log
 *        calls.
 */
#define PUBNUB_LOG_LEVEL_DEBUG_VALUE_ (1 << 1)
/**
 * @brief Compile-time `info` log level value.
 *
 * @note Value used along with pre-processing macro to strip unused `info` log
 *        calls.
 */
#define PUBNUB_LOG_LEVEL_INFO_VALUE_ (1 << 2)
/**
 * @brief Compile-time `warn` log level value.
 *
 * @note Value used along with pre-processing macro to strip unused `warn` log
 *        calls.
 */
#define PUBNUB_LOG_LEVEL_WARNING_VALUE_ (1 << 3)
/**
 * @brief Compile-time `error` log level value.
 *
 * @note Value used along with pre-processing macro to strip unused `error` log
 *        calls.
 */
#define PUBNUB_LOG_LEVEL_ERROR_VALUE_ (1 << 4)
/**
 * @brief Compile-time `none` log level value.
 *
 * @note Value used along with pre-processing macro to strip out all log calls.
 */
#define PUBNUB_LOG_LEVEL_NONE_VALUE_ (1 << 5)

/** Available Log levels. */
enum pubnub_log_level {
    /**
     * @brief The most verbose log level.
     *
     * Used to notify about every last detail:
     * - function calls,
     * - full payloads,
     * - internal variables,
     * - state-machine hops.
     */
    PUBNUB_LOG_LEVEL_TRACE = PUBNUB_LOG_LEVEL_TRACE_VALUE_,
    /**
     * @brief Debugging messages log level.
     *
     * Used to notify about broad strokes of your SDK’s logic:
     * - inputs/outputs to public methods,
     * - network request,
     * - network response,
     * - decision branches.
     */
    PUBNUB_LOG_LEVEL_DEBUG = PUBNUB_LOG_LEVEL_DEBUG_VALUE_,
    /**
     * @brief Informational messages log level.
     *
     * Used to notify summary of what the SDK is doing under the hood:
     * - initialized,
     * - connected,
     * - entity created.
     */
    PUBNUB_LOG_LEVEL_INFO = PUBNUB_LOG_LEVEL_INFO_VALUE_,
    /**
     * @brief Warnings messages log level.
     *
     * Used to notify about non-fatal events:
     * - deprecations,
     * - request retries.
     */
    PUBNUB_LOG_LEVEL_WARNING = PUBNUB_LOG_LEVEL_WARNING_VALUE_,
    /**
     * @brief Errors messages log level.
     *
     * Used to notify about:
     * - exceptions,
     * - HTTP failures,
     * - invalid states.
     */
    PUBNUB_LOG_LEVEL_ERROR = PUBNUB_LOG_LEVEL_ERROR_VALUE_,
    /** Logging disabled. */
    PUBNUB_LOG_LEVEL_NONE = PUBNUB_LOG_LEVEL_NONE_VALUE_,
};

/** Logged message types. */
enum pubnub_log_message_type {
    /** Plain text message. */
    PUBNUB_LOG_MESSAGE_TYPE_TEXT,
    /** Structured object/data message. */
    PUBNUB_LOG_MESSAGE_TYPE_OBJECT,
    /** Error/exception message. */
    PUBNUB_LOG_MESSAGE_TYPE_ERROR,
    /** Network request information. */
    PUBNUB_LOG_MESSAGE_TYPE_NETWORK_REQUEST,
    /** Network response information. */
    PUBNUB_LOG_MESSAGE_TYPE_NETWORK_RESPONSE
};

/** HTTP methods for network requests. */
enum pubnub_http_method {
    /** HTTP `GET` method. */
    PUBNUB_HTTP_METHOD_GET,
    /** HTTP `POST` method. */
    PUBNUB_HTTP_METHOD_POST,
    /** HTTP `DELETE` method. */
    PUBNUB_HTTP_METHOD_DELETE,
    /** HTTP `PATCH` method. */
    PUBNUB_HTTP_METHOD_PATCH,
    /** HTTP `PUT` method. */
    PUBNUB_HTTP_METHOD_PUT
};

/** Base log entry definition. */
struct pubnub_log_message {
    /** Message type discriminator. */
    enum pubnub_log_message_type message_type;
    /** Target log message level. */
    enum pubnub_log_level level;
    /** Unique identifier of the PubNub client instance. */
    char const* pubnub_id;
    /** Date and time when the log message was generated. */
    struct {
        /**
         * @brief Unix-timestamp.
         *
         * @note Value could be `0` if code wasn't able to access this
         *       information.
         */
        time_t seconds;
        /**
         * @brief Extended log entry time precision with milliseconds.
         *
         * @note Value could be `0` if code wasn't able to access this
         *       information.
         */
        uint32_t milliseconds;
    } timestamp;
    /** Call site from which log message was sent. */
    char const* location;
    /** Minimum log level configured for the PubNub context. */
    enum pubnub_log_level minimum_level;
};

/** Plain text message log entry definition. */
struct pubnub_log_message_text {
    /** Base log entry fields. */
    pubnub_log_message_t base;
    /** Textual message that has been logged. */
    char const* message;
};
typedef struct pubnub_log_message_text pubnub_log_message_text_t;

/** Structured (including nested data structures) log entry definition. */
struct pubnub_log_message_object {
    /** Base log entry fields. */
    pubnub_log_message_t base;
    /** Structured value being logged (can be a map, array, or primitive). */
    pubnub_log_value_t const* message;
    /** Additional details describing the data (optional, can be `NULL`). */
    char const* details;
};
typedef struct pubnub_log_message_object pubnub_log_message_object_t;

/** Error log entry definition. */
struct pubnub_log_message_error {
    /** Base log entry fields. */
    pubnub_log_message_t base;
    /** Error code. */
    int error_code;
    /** Error message/description. */
    char const* error_message;
    /** Additional error details (optional, can be `NULL`). */
    char const* details;
};
typedef struct pubnub_log_message_error pubnub_log_message_error_t;

/** Network request log entry definition. */
struct pubnub_log_message_network_request {
    /** Base log entry fields. */
    pubnub_log_message_t base;
    /** HTTP method. */
    enum pubnub_http_method method;
    /** Full request URL. */
    char const* url;
    /** Request headers as a map (optional, can be `NULL`). */
    pubnub_log_value_t const* headers;
    /** Request body (optional, can be `NULL`; may be binary if compressed). */
    char const* body;
    /** Additional details (optional, can be `NULL`). */
    char const* details;
    /** Whether the request was canceled. */
    bool canceled;
    /** Whether the request failed. */
    bool failed;
};
typedef struct pubnub_log_message_network_request pubnub_log_message_network_request_t;

/** Network response log entry definition. */
struct pubnub_log_message_network_response {
    /** Base log entry fields. */
    pubnub_log_message_t base;
    /** Full request URL that this response is for. */
    char const* url;
    /** HTTP status code. */
    int status_code;
    /** Response headers as a map (optional, can be `NULL`). */
    pubnub_log_value_t const* headers;
    /** Response body (optional, can be `NULL`). */
    char const* body;
};
typedef struct pubnub_log_message_network_response pubnub_log_message_network_response_t;

/**
 * @brief Logger interface - function pointers for custom logger implementations.
 *
 * Users can create their own logger by implementing these functions and
 * registering the logger with a PubNub context using `pubnub_logger_add`.
 *
 * All function pointers are optional (can be `NULL`). If a function pointer is
 * `NULL`, that log level will not be processed by this logger.
 *
 * @see pubnub_logger_add
 */
struct pubnub_logger_interface {
    /**
     * @brief Process a `trace` level message.
     *
     * @param logger  Pointer to the custom logger object.
     * @param message Pointer to the log message (type depends on `message_type`
     *                field).
     */
    void (*trace)(const pubnub_logger_t*      logger,
                  const pubnub_log_message_t* message);
    /**
     * @brief Process a `debug` level message.
     *
     * @param logger  Pointer to the custom logger object.
     * @param message Pointer to the log message (type depends on `message_type`
     *                field).
     */
    void (*debug)(const pubnub_logger_t*      logger,
                  const pubnub_log_message_t* message);
    /**
     * @brief Process an `info` level message.
     *
     * @param logger  Pointer to the custom logger object.
     * @param message Pointer to the log message (type depends on `message_type`
     *                field).
     */
    void (*info)(const pubnub_logger_t* logger, const pubnub_log_message_t* message);
    /**
     * @brief Process a `warning` level message.
     *
     * @param logger  Pointer to the custom logger object.
     * @param message Pointer to the log message (type depends on `message_type`
     *                field).
     */
    void (*warn)(const pubnub_logger_t* logger, const pubnub_log_message_t* message);
    /**
     * @brief Process an `error` level message.
     *
     * @param logger  Pointer to the custom logger object.
     * @param message Pointer to the log message (type depends on `message_type`
     *                field).
     */
    void (*error)(const pubnub_logger_t*      logger,
                  const pubnub_log_message_t* message);
    /**
     * @brief Destroy/cleanup the logger instance (optional, can be `NULL`).
     *
     * Called when the logger is removed or when the PubNub context is freed.
     *
     * @param logger Pointer to the custom logger instance to destroy.
     */
    void (*destroy)(pubnub_logger_t* logger);
};


// ----------------------------------------------
//                   Functions
// ----------------------------------------------

/**
 * @brief Create a custom logger.
 *
 * @note Pointer to the custom logger object should be valid while used by the
 *       logger manager. It is the user's responsibility to
 *       call `pubnub_logger_free`.
 *
 * @param vtable    Table with log-level specific log function pointers.
 * @param user_data User-provided context data (optional, can be `NULL`).
 * @return Pointer to the ready-to-use custom logger or `NULL` in case of
 *         insufficient memory error. The returned pointer must be passed to the
 *         `pubnub_logger_free` to avoid a memory leak.
 *
 * @code
 * static void my_trace(const pubnub_logger_t*      logger,
 *                      const pubnub_log_message_t* message) {
 *     // Cast to the concrete message type based on message->message_type.
 *     if (message->message_type == PUBNUB_LOG_MESSAGE_TYPE_TEXT) {
 *         const pubnub_log_message_text_t* text =
 *             (const pubnub_log_message_text_t*)message;
 *         printf("[%s] %s\n", message->pubnub_id, text->message);
 *     }
 * }
 *
 * // Build a vtable with only the levels you care about (others can be NULL).
 * static const struct pubnub_logger_interface my_vtable = {
 *     .trace   = my_trace,
 *     .debug   = NULL,
 *     .info    = NULL,
 *     .warn    = NULL,
 *     .error   = NULL,
 *     .destroy = NULL,
 * };
 *
 * pubnub_logger_t* logger = pubnub_logger_alloc(&my_vtable, NULL);
 * pubnub_logger_add(pb, logger);
 * @endcode
 *
 * @see pubnub_logger_free
 * @see pubnub_logger_interface
 */
PUBNUB_EXTERN pubnub_logger_t*
pubnub_logger_alloc(struct pubnub_logger_interface const* vtable, void* user_data);

/**
 * @brief Free resources used by the custom logger.
 *
 * @note The logger's destroy function **WILL** be called (if provided).
 *
 * @param logger Pointer to the pointer to the custom logger which should be
 *               disposed. The pointer will be set to `NULL` after freeing.
 *
 * @code
 * pubnub_logger_t* logger = pubnub_logger_alloc(&my_vtable, NULL);
 * // ... use the logger ...
 * pubnub_logger_free(&logger);
 * // logger is now NULL.
 * @endcode
 *
 * @see pubnub_logger_alloc
 */
PUBNUB_EXTERN void pubnub_logger_free(pubnub_logger_t** logger);

/**
 * @brief Retrieve user data associated with the custom logger.
 *
 * @param logger Pointer to the custom logger for which user-provided data
 *               should be retrieved.
 * @return User data associated with the custom logger, or `NULL` if @p logger
 *         is `NULL` or no user data was provided.
 *
 * @code
 * typedef struct {
 *     FILE* file;
 *     int   entry_count;
 * } my_logger_context;
 *
 * static void my_info(const pubnub_logger_t*      logger,
 *                     const pubnub_log_message_t* message) {
 *     my_logger_context* ctx =
 *         (my_logger_context*)pubnub_logger_user_data(
 *             (pubnub_logger_t*)logger);
 *     ctx->entry_count++;
 *     // ... write to ctx->file ...
 * }
 * @endcode
 *
 * @see pubnub_logger_alloc
 */
PUBNUB_EXTERN void* pubnub_logger_user_data(pubnub_logger_t* logger);

/**
 * @brief Add a custom logger to the PubNub context logging subsystem.
 *
 * Registers a custom logger with the PubNub context logging subsystem. Multiple
 * loggers can be registered, and all will receive log messages of appropriate
 * levels.
 *
 * @note Pointer to the custom logger object should be valid while used by the
 *       logging subsystem. It is the user's responsibility to
 *       call `pubnub_logger_free`.
 *
 * @param pb     Pointer to the PubNub context.
 * @param logger Pointer to the custom logger to add (must not be `NULL`).
 * @retval 0 on success.
 * @retval -1 on failure (`NULL` pointer or already added).
 *
 * @code
 * pubnub_logger_t* logger = pubnub_logger_alloc(&my_vtable, NULL);
 * pubnub_logger_add(pb, logger);
 * // ... use PubNub context ...
 * pubnub_logger_remove(pb, logger);
 * pubnub_logger_free(&logger);
 * @endcode
 *
 * @see pubnub_logger_alloc
 * @see pubnub_logger_remove
 * @see pubnub_logger_free
 */
PUBNUB_EXTERN int pubnub_logger_add(const pubnub_t*       pb,
                                    struct pubnub_logger* logger);

/**
 * @brief Unregister a custom logger from the PubNub context logging subsystem.
 *
 * @note It is the user's responsibility to call `pubnub_logger_free` after
 *       removing the logger.
 *
 * @param pb     Pointer to the PubNub context.
 * @param logger Pointer to the custom logger to remove.
 * @retval 0 on success.
 * @retval -1 on failure (logger not found or `NULL` pointer).
 *
 * @code
 * pubnub_logger_remove(pb, logger);
 * pubnub_logger_free(&logger);
 * @endcode
 *
 * @see pubnub_logger_add
 * @see pubnub_logger_free
 */
PUBNUB_EXTERN int pubnub_logger_remove(const pubnub_t*       pb,
                                       struct pubnub_logger* logger);

/**
 * @brief Remove all custom loggers from the PubNub context logging subsystem.
 *
 * Unregisters all custom loggers from the logging subsystem. It is the user's
 * responsibility to call `pubnub_logger_free` for each logger after this call.
 *
 * @param pb Pointer to the PubNub context.
 *
 * @code
 * pubnub_logger_remove_all(pb);
 * // Custom loggers are unregistered but NOT freed — user must free them.
 * pubnub_logger_free(&logger1);
 * pubnub_logger_free(&logger2);
 * @endcode
 *
 * @see pubnub_logger_add
 * @see pubnub_logger_free
 */
PUBNUB_EXTERN void pubnub_logger_remove_all(const pubnub_t* pb);

/**
 * @brief Set the minimum log level for the PubNub context logging subsystem.
 *
 * Only messages with level >= minimum_level will be dispatched to custom
 * loggers. This allows filtering log messages at the source before they reach
 * any custom logger.
 *
 * @param pb    Pointer to the PubNub context.
 * @param level The minimum log level.
 *
 * @code
 * // Only receive warning and error messages.
 * pubnub_logger_set_log_level(pb, PUBNUB_LOG_LEVEL_WARNING);
 * @endcode
 *
 * @see pubnub_logger_log_level
 */
PUBNUB_EXTERN void pubnub_logger_set_log_level(const pubnub_t*       pb,
                                               enum pubnub_log_level level);

/**
 * @brief Get the current minimum log level for the PubNub context logging
 *        subsystem.
 *
 * @b Default: `PUBNUB_LOG_LEVEL_DEBUG`.
 *
 * @param pb Pointer to the PubNub context.
 * @return The current minimum log level, or `PUBNUB_LOG_LEVEL_NONE` if
 *         @p pb is `NULL`.
 *
 * @code
 * enum pubnub_log_level level = pubnub_logger_log_level(pb);
 * @endcode
 *
 * @see pubnub_logger_set_log_level
 */
PUBNUB_EXTERN enum pubnub_log_level pubnub_logger_log_level(pubnub_t const* pb);

/**
 * @brief Log a text log entry.
 *
 * @param pb       Pointer to the PubNub context.
 * @param level    Log entry log level.
 * @param location Call site location.
 * @param message  Text message to log.
 *
 * @code
 * pubnub_log_text(pb, PUBNUB_LOG_LEVEL_INFO, PUBNUB_LOG_LOCATION,
 *                 "Subscription connected");
 * @endcode
 */
PUBNUB_EXTERN void pubnub_log_text(const pubnub_t*       pb,
                                   enum pubnub_log_level level,
                                   char const*           location,
                                   char const*           message);

/**
 * @brief Log a formatted text log entry (printf-style).
 *
 * @param pb       Pointer to the PubNub context.
 * @param level    Log entry log level.
 * @param location Call site location.
 * @param format   Printf-style format string.
 * @param ...      Format arguments.
 *
 * @code
 * pubnub_log_text_formatted(pb, PUBNUB_LOG_LEVEL_DEBUG, PUBNUB_LOG_LOCATION,
 *                           "Publishing to channel '%s'", channel);
 * @endcode
 */
PUBNUB_EXTERN void pubnub_log_text_formatted(const pubnub_t*       pb,
                                             enum pubnub_log_level level,
                                             char const*           location,
                                             char const*           format,
                                             ...);

/**
 * @brief Log a structured object/data log entry.
 *
 * @note The value is not freed by this function — caller retains ownership.
 *
 * @param pb       Pointer to the PubNub context.
 * @param level    Log entry log level.
 * @param location Call site location.
 * @param message  Structured value to log (`map`, `array`, or `primitive`).
 * @param details  Optional description for passed structured data (can be
 *                 `NULL`).
 *
 * @code
 * pubnub_log_value_t params = pubnub_log_value_map_init();
 * PUBNUB_LOG_MAP_SET_STRING(&params, channel)
 * PUBNUB_LOG_MAP_SET_NUMBER(&params, opts.ttl, ttl)
 * pubnub_log_object(pb, PUBNUB_LOG_LEVEL_DEBUG, PUBNUB_LOG_LOCATION,
 *                   &params, "publish parameters");
 * @endcode
 */
PUBNUB_EXTERN void pubnub_log_object(const pubnub_t*           pb,
                                     enum pubnub_log_level     level,
                                     char const*               location,
                                     pubnub_log_value_t const* message,
                                     char const*               details);

/**
 * @brief Log an error log entry.
 *
 * @note The log level is always `PUBNUB_LOG_LEVEL_ERROR`.
 *
 * @param pb            Pointer to the PubNub context.
 * @param location      Call site location.
 * @param error_code    Error code.
 * @param error_message Error description.
 * @param details       Additional error details (can be `NULL`).
 *
 * @code
 * pubnub_log_error(pb, PUBNUB_LOG_LOCATION, -1,
 *                  "Connection failed", "Timeout after 30s");
 * @endcode
 */
PUBNUB_EXTERN void pubnub_log_error(const pubnub_t* pb,
                                    char const*     location,
                                    int             error_code,
                                    char const*     error_message,
                                    char const*     details);
#endif // PUBNUB_LOGGER_H