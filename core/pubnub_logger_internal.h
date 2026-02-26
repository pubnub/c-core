/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#ifndef PUBNUB_LOGGER_INTERNAL_H
#define PUBNUB_LOGGER_INTERNAL_H


/**
 * @file  pubnub_logger_internal.h
 * @brief PubNub Logger internal types and functions.
 */


#include "pubnub_logger.h"


// ----------------------------------------------
//                     Types
// ----------------------------------------------

/**
 * @brief Custom logger definition.
 *
 * Represents a custom logger with its implementation functions and user data.
 * Initialize with `pubnub_logger_alloc`, then register with a PubNub context
 * using `pubnub_logger_add`.
 *
 * @see pubnub_logger_alloc
 * @see pubnub_logger_add
 */
struct pubnub_logger {
    /** Logger implementation function pointers. */
    struct pubnub_logger_interface const* vtable;
    /** User-provided context data (optional, can be `NULL`). */
    void* user_data;
    /** Pointer to next logger in linked list (managed by logger manager). */
    struct pubnub_logger* next;
};


// ----------------------------------------------
//                   Functions
// ----------------------------------------------

/**
 * @brief Check whether PubNub context logging subsystem will process a log
 *        entry at the given level (internal).
 *
 * Use before preparing expensive log payloads to avoid wasted work when the
 * runtime minimum log level would filter the message.
 *
 * @param pb    Pointer to the PubNub context.
 * @param level Target log entry level.
 * @return `true` if the logging subsystem will process a message at this level.
 */
bool pubnub_logger_should_log(const pubnub_t* pb, enum pubnub_log_level level);

/**
 * @brief Log a network request (internal).
 *
 * @param pb       Pointer to the PubNub context.
 * @param level    Log entry log level.
 * @param location Call site location.
 * @param method   HTTP method.
 * @param url      Request URL.
 * @param headers  Request headers as a map (can be `NULL`).
 * @param body     Request body (can be `NULL`; may be binary if compressed).
 * @param details  Additional failure / cancellation details (can be `NULL`).
 * @param canceled Whether request was canceled.
 * @param failed   Whether request failed.
 */
void pubnub_log_network_request(
    const pubnub_t*           pb,
    enum pubnub_log_level     level,
    char const*               location,
    enum pubnub_http_method   method,
    char const*               url,
    pubnub_log_value_t const* headers,
    char const*               body,
    char const*               details,
    bool                      canceled,
    bool                      failed);

/**
 * @brief Log a network response (internal).
 *
 * @param pb          Pointer to the PubNub context.
 * @param level       Log entry log level.
 * @param location    Call site location.
 * @param url         Request URL that this response is for.
 * @param status_code HTTP status code.
 * @param headers     Response headers as a map (can be `NULL`).
 * @param body        Response body (can be `NULL`).
 */
void pubnub_log_network_response(
    const pubnub_t*           pb,
    enum pubnub_log_level     level,
    char const*               location,
    char const*               url,
    int                       status_code,
    pubnub_log_value_t const* headers,
    char const*               body);


#if PUBNUB_USE_LOGGER

// ----------------------------------------------
//         Compile-time log level stripping
// ----------------------------------------------

/**
 * @brief `PUBNUB_LOG_MIN_LEVEL` sets the floor for what gets compiled in.
 *
 * Accepts short names: `TRACE`, `DEBUG`, `INFO`, `WARNING`, `ERROR`, `NONE`.
 * Override at build time: `-DPUBNUB_LOG_MIN_LEVEL=WARNING`
 *
 * **Default:** `DEBUG` (`TRACE` stripped, everything else compiled in).
 */
#ifndef PUBNUB_LOG_MIN_LEVEL
#define PUBNUB_LOG_MIN_LEVEL DEBUG
#endif // PUBNUB_LOG_MIN_LEVEL

/* Token-paste helpers (double indirection for macro expansion). */
#define PBLOG_CAT_(a, b) a##b
#define PBLOG_CAT(a, b) PBLOG_CAT_(a, b)

/**
 * @brief Resolve short name.
 *
 * @code
 * PBLOG_LEVEL_VAL_(WARNING) -> PUBNUB_LOG_LEVEL_WARNING_VALUE_ -> (1 << 3)
 * @endcode
 */
#define PBLOG_LEVEL_VAL_(name)                                                 \
    PBLOG_CAT(PUBNUB_LOG_LEVEL_, PBLOG_CAT(name, _VALUE_))

/** Resolved numeric value of the compile-time minimum log level. */
#define PUBNUB_LOG_MIN_LEVEL_VALUE_ PBLOG_LEVEL_VAL_(PUBNUB_LOG_MIN_LEVEL)

/**
 * @brief Compile-time check: is logging at `level_suffix` compiled into this
 *        build?
 * @code
 * #if PUBNUB_LOG_ENABLED(ERROR)
 * // Log entry with error log level.
 * #endif // PUBNUB_LOG_ENABLED(ERROR)
 * @endcode
 */
#define PUBNUB_LOG_ENABLED(level_suffix)                                       \
    (PBLOG_LEVEL_VAL_(level_suffix) >= PUBNUB_LOG_MIN_LEVEL_VALUE_)

/** Stringify macro helper. */
#define PUBNUB_STRINGIFY(x) PUBNUB_STRINGIFY_IMPL(x)
#define PUBNUB_STRINGIFY_IMPL(x) #x

/** Helper to create location string from file and line. */
#define PUBNUB_LOG_LOCATION __FILE__ ":" PUBNUB_STRINGIFY(__LINE__)


// ----------------------------------------------
//           Convenient logging macro
// ----------------------------------------------

#if PUBNUB_LOG_ENABLED(TRACE)
/**
 * @brief Log a formatted text log entry with `trace` log level (printf-style).
 *
 * @param pb  Pointer to the PubNub context.
 * @param ... Format arguments.
 */
#define PUBNUB_LOG_TRACE(pb, ...)                                              \
    do {                                                                       \
        if (pubnub_logger_should_log((pb), PUBNUB_LOG_LEVEL_TRACE))            \
            pubnub_log_text_formatted(                                         \
                (pb),                                                          \
                PUBNUB_LOG_LEVEL_TRACE,                                        \
                PUBNUB_LOG_LOCATION,                                           \
                __VA_ARGS__);                                                  \
    } while (0)
#else // !PUBNUB_LOG_ENABLED(TRACE)
/**
 * @brief Log a formatted text log entry with `trace` log level (printf-style).
 *
 * @note This is a placeholder for compile-time excluded log level function
 *       calls.
 *
 * @param pb  Pointer to the PubNub context.
 * @param ... Format arguments.
 */
#define PUBNUB_LOG_TRACE(pb, ...) ((void)(pb))
#endif // PUBNUB_LOG_ENABLED(TRACE)

#if PUBNUB_LOG_ENABLED(DEBUG)
/**
 * @brief Log a formatted text log entry with `debug` log level (printf-style).
 *
 * @param pb  Pointer to the PubNub context.
 * @param ... Format arguments.
 */
#define PUBNUB_LOG_DEBUG(pb, ...)                                              \
    do {                                                                       \
        if (pubnub_logger_should_log((pb), PUBNUB_LOG_LEVEL_DEBUG))            \
            pubnub_log_text_formatted(                                         \
                (pb),                                                          \
                PUBNUB_LOG_LEVEL_DEBUG,                                        \
                PUBNUB_LOG_LOCATION,                                           \
                __VA_ARGS__);                                                  \
    } while (0)
#else // !PUBNUB_LOG_ENABLED(DEBUG)
/**
 * @brief Log a formatted text log entry with `debug` log level (printf-style).
 *
 * @note This is a placeholder for compile-time excluded log level function
 *       calls.
 *
 * @param pb  Pointer to the PubNub context.
 * @param ... Format arguments.
 */
#define PUBNUB_LOG_DEBUG(pb, ...) ((void)(pb))
#endif // PUBNUB_LOG_ENABLED(DEBUG)

#if PUBNUB_LOG_ENABLED(INFO)
/**
 * @brief Log a formatted text log entry with `info` log level (printf-style).
 *
 * @param pb  Pointer to the PubNub context.
 * @param ... Format arguments.
 */
#define PUBNUB_LOG_INFO(pb, ...)                                               \
    do {                                                                       \
        if (pubnub_logger_should_log((pb), PUBNUB_LOG_LEVEL_INFO))             \
            pubnub_log_text_formatted(                                         \
                (pb),                                                          \
                PUBNUB_LOG_LEVEL_INFO,                                         \
                PUBNUB_LOG_LOCATION,                                           \
                __VA_ARGS__);                                                  \
    } while (0)
#else // !PUBNUB_LOG_ENABLED(INFO)
/**
 * @brief Log a formatted text log entry with `info` log level (printf-style).
 *
 * @note This is a placeholder for compile-time excluded log level function
 *       calls.
 *
 * @param pb  Pointer to the PubNub context.
 * @param ... Format arguments.
 */
#define PUBNUB_LOG_INFO(pb, ...) ((void)(pb))
#endif // PUBNUB_LOG_ENABLED(INFO)

#if PUBNUB_LOG_ENABLED(WARNING)
/**
 * @brief Log a formatted text log entry with `warning` log
 *        level (printf-style).
 *
 * @param pb  Pointer to the PubNub context.
 * @param ... Format arguments.
 */
#define PUBNUB_LOG_WARNING(pb, ...)                                            \
    do {                                                                       \
        if (pubnub_logger_should_log((pb), PUBNUB_LOG_LEVEL_WARNING))          \
            pubnub_log_text_formatted(                                         \
                (pb),                                                          \
                PUBNUB_LOG_LEVEL_WARNING,                                      \
                PUBNUB_LOG_LOCATION,                                           \
                __VA_ARGS__);                                                  \
    } while (0)
#else // !PUBNUB_LOG_ENABLED(WARNING)
/**
 * @brief Log a formatted text log entry with `warning` log
 *        level (printf-style).
 *
 * @note This is a placeholder for compile-time excluded log level function
 *       calls.
 *
 * @param pb  Pointer to the PubNub context.
 * @param ... Format arguments.
 */
#define PUBNUB_LOG_WARNING(pb, ...) ((void)(pb))
#endif // PUBNUB_LOG_ENABLED(WARNING)

#if PUBNUB_LOG_ENABLED(ERROR)
/**
 * @brief Log a formatted text log entry with `error` log level (printf-style).
 *
 * @param pb  Pointer to the PubNub context.
 * @param ... Format arguments.
 */
#define PUBNUB_LOG_ERROR(pb, ...)                                              \
    do {                                                                       \
        if (pubnub_logger_should_log((pb), PUBNUB_LOG_LEVEL_ERROR))            \
            pubnub_log_text_formatted(                                         \
                (pb),                                                          \
                PUBNUB_LOG_LEVEL_ERROR,                                        \
                PUBNUB_LOG_LOCATION,                                           \
                __VA_ARGS__);                                                  \
    } while (0)
#else // !PUBNUB_LOG_ENABLED(ERROR)
/**
 * @brief Log a formatted text log entry with `error` log level (printf-style).
 *
 * @note This is a placeholder for compile-time excluded log level function
 *       calls.
 *
 * @param pb  Pointer to the PubNub context.
 * @param ... Format arguments.
 */
#define PUBNUB_LOG_ERROR(pb, ...) ((void)(pb))
#endif // PUBNUB_LOG_ENABLED(ERROR)

#else // !PUBNUB_USE_LOGGER

/* When logger is disabled, provide no-op fallbacks.
 * Use #ifndef guards to avoid redefinition if pubnub_ccore_pubsub.h already
 * defined them. */
#ifndef PUBNUB_LOG_ENABLED
#define PUBNUB_LOG_ENABLED(level_suffix) 0
#endif
#ifndef PUBNUB_LOG_TRACE
#define PUBNUB_LOG_TRACE(pb, ...)   ((void)(pb))
#endif
#ifndef PUBNUB_LOG_DEBUG
#define PUBNUB_LOG_DEBUG(pb, ...)   ((void)(pb))
#endif
#ifndef PUBNUB_LOG_INFO
#define PUBNUB_LOG_INFO(pb, ...)    ((void)(pb))
#endif
#ifndef PUBNUB_LOG_WARNING
#define PUBNUB_LOG_WARNING(pb, ...) ((void)(pb))
#endif
#ifndef PUBNUB_LOG_ERROR
#define PUBNUB_LOG_ERROR(pb, ...)   ((void)(pb))
#endif

#endif // PUBNUB_USE_LOGGER
#endif // #ifndef PUBNUB_LOGGER_INTERNAL_H
