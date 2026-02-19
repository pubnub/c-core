/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#ifndef PBCC_LOGGER_MANAGER_H
#define PBCC_LOGGER_MANAGER_H


/**
 * @file  pbcc_logger_manager.h
 * @brief PubNub Logger system manager implementation.
 */


#include "core/pubnub_api_types.h"
#include "core/pubnub_logger_internal.h"


// ----------------------------------------------
//                Types forwarding
// ----------------------------------------------

/** Logger manager definition. */
struct pbcc_logger_manager;
typedef struct pbcc_logger_manager pbcc_logger_manager_t;


// ----------------------------------------------
//                   Functions
// ----------------------------------------------

/**
 * @brief Create logger manager.
 *
 * @param pubnub_id Unique identifier of PubNub context which will use this
 *                  logger.
 * @return Pointer to the ready-to-use logger manager or `NULL` in case of
 *         insufficient memory error. The returned pointer must be passed to the
 *         `pbcc_logger_manager_free` to avoid a memory leak.
 *
 * @see pbcc_logger_manager_free
 */
pbcc_logger_manager_t* pbcc_logger_manager_alloc(const char* pubnub_id);

/**
 * @brief Dispose and free up resources used by the logger manager.
 *
 * @note Only the default logger will be destroyed automatically.
 *
 * @param manager Pointer to the logger manager which should be disposed.
 */
void pbcc_logger_manager_free(pbcc_logger_manager_t* manager);

#if PUBNUB_USE_DEFAULT_LOGGER
/**
 * @brief Setup default / main logger, which will be called before any other
 *        loggers.
 *
 * @param manager Pointer to the logger manager for which default logger is
 *                registered.
 * @param logger  Pointer to the custom logger that should be used before any
 *                user-provided loggers.
 */
void pbcc_logger_manager_set_default_logger(pbcc_logger_manager_t* manager,
                                            pubnub_logger_t*       logger);
#endif // PUBNUB_USE_DEFAULT_LOGGER

/**
 * @brief Add a custom logger to the logger manager.
 *
 * Registers a custom logger object with the logger manager. Multiple loggers
 * can be registered, and all will receive log messages of appropriate levels.
 *
 * @note Pointer to the custom logger object should be valid while used by the
 *       logger manager. It is the user's responsibility to
 *       call `pubnub_logger_free`.
 *
 * @param manager Pointer to the logger manager with which custom logger should
 *                be registered.
 * @param logger  Pointer to the custom logger to add (must not be `NULL`).
 * @retval 0 on success.
 * @retval -1 on failure (`NULL` pointer or already added).
 *
 * @see pubnub_logger_alloc
 * @see pubnub_logger_free
 * @see pbcc_logger_manager_logger_remove
 */
int pbcc_logger_manager_logger_add(pbcc_logger_manager_t* manager,
                                   pubnub_logger_t*       logger);

/**
 * @brief Remove a custom logger from the PubNub context.
 *
 * @param manager Pointer to the logger manager from which custom logger should
 *                be removed.
 * @param logger  Pointer to the custom logger to remove.
 * @retval 0 on success.
 * @retval -1 on failure (`NULL` pointer or not found).
 *
 * @see pbcc_logger_manager_logger_add
 */
int pbcc_logger_manager_logger_remove(pbcc_logger_manager_t* manager,
                                      pubnub_logger_t*       logger);

/**
 * @brief Remove all custom loggers from the PubNub logger manager.
 *
 * @param manager      Pointer to the logger manager from which all registered
 *                     custom loggers should be removed.
 */
void pbcc_logger_manager_logger_remove_all(pbcc_logger_manager_t* manager);

/**
 * @brief Set the minimum log level for logger manager.
 *
 * Only messages with level >= minimum_level will be dispatched to loggers.
 * This allows filtering log messages at the source before they reach any
 * logger.
 *
 * @param manager Pointer to the logger manager from which minimal log level
 *                should be changed.
 * @param level   The minimum log level.
 */
void pbcc_logger_manager_set_log_level(pbcc_logger_manager_t* manager,
                                       enum pubnub_log_level  level);

/**
 * @brief Get the current minimum log level for the logger manager.
 *
 * @param manager Pointer to the logger manager from which minimal log level
 *                should be retrieved.
 * @return The current minimum log level, or `PUBNUB_LOG_LEVEL_NONE` if
 *         @p manager is `NULL`.
 *
 * @see pbcc_logger_manager_set_log_level
 */
enum pubnub_log_level pbcc_logger_manager_log_level(pbcc_logger_manager_t* manager);

/**
 * @brief Check whether logger manager will process log entry or not.
 *
 * This function should be used before preparing the payload for a logger
 * manager to not waste resources for messages that wouldn't be processed.
 *
 * @param manager Pointer to the logger manager for which minimum log level will
 *                be checked.
 * @param level   Target log entry level.
 * @return Whether logger will be able to process log entry with specified log
 *         level (if minimum log level allows).
 */
bool pbcc_logger_manager_should_log(pbcc_logger_manager_t* manager,
                                    enum pubnub_log_level  level);

/**
 * @brief Log a text log entry.
 *
 * @param manager  Pointer to the logger manager.
 * @param level    Log entry log level.
 * @param location Call site location.
 * @param message  Text message to log.
 */
void pbcc_logger_manager_log_text(const pbcc_logger_manager_t* manager,
                                  enum pubnub_log_level        level,
                                  char const*                  location,
                                  char const*                  message);

/**
 * @brief Log a formatted text log entry (printf-style).
 *
 * @param manager  Pointer to the logger manager.
 * @param level    Log entry log level.
 * @param location Call site location.
 * @param format   Printf-style format string.
 * @param ...      Format arguments.
 */
void pbcc_logger_manager_log_text_formatted(const pbcc_logger_manager_t* manager,
                                            enum pubnub_log_level level,
                                            char const*           location,
                                            char const*           format,
                                            ...);

/**
 * @brief Log a structured object/data log entry.
 *
 * @param manager  Pointer to the logger manager.
 * @param level    Log entry log level.
 * @param location Call site location.
 * @param message  Structured value to log (map, array, or primitive).
 * @param details  Optional description (can be `NULL`).
 */
void pbcc_logger_manager_log_object(const pbcc_logger_manager_t* manager,
                                    enum pubnub_log_level        level,
                                    char const*                  location,
                                    pubnub_log_value_t const*    message,
                                    char const*                  details);

/**
 * @brief Log an error log entry.
 *
 * @param manager       Pointer to the logger manager.
 * @param level         Log entry log level.
 * @param location      Call site location.
 * @param error_code    Error code.
 * @param error_message Error description.
 * @param details       Additional error details (can be `NULL`).
 */
void pbcc_logger_manager_log_error(const pbcc_logger_manager_t* manager,
                                   enum pubnub_log_level        level,
                                   char const*                  location,
                                   int                          error_code,
                                   char const*                  error_message,
                                   char const*                  details);

/**
 * @brief Log a network request log entry.
 *
 * @param manager  Pointer to the logger manager.
 * @param level    Log entry log level.
 * @param location Call site location.
 * @param method   HTTP method.
 * @param url      Request URL.
 * @param headers  Request headers as a map (can be `NULL`).
 * @param body     Request body (can be `NULL`).
 * @param details  Additional details (can be `NULL`).
 * @param canceled Whether request was canceled.
 * @param failed   Whether request failed.
 */
void pbcc_logger_manager_log_network_request(const pbcc_logger_manager_t* manager,
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
 * @brief Log a network response log entry.
 *
 * @param manager     Pointer to the logger manager.
 * @param level       Log entry log level.
 * @param location    Call site location.
 * @param url         Request URL that this response is for.
 * @param status_code HTTP status code.
 * @param headers     Response headers as a map (can be `NULL`).
 * @param body        Response body (can be `NULL`).
 */
void pbcc_logger_manager_log_network_response(const pbcc_logger_manager_t* manager,
                                              enum pubnub_log_level level,
                                              char const*           location,
                                              char const*           url,
                                              int                   status_code,
                                              pubnub_log_value_t const* headers,
                                              char const*               body);


// ----------------------------------------------
//           Convenient logging macro
// ----------------------------------------------

#if PUBNUB_LOG_ENABLED(TRACE)
/**
 * @brief Log a formatted text log entry with `trace` log level (printf-style).
 *
 * @param mgr Pointer to the logger manager.
 * @param ... Format arguments.
 */
#define PBCC_LOG_TRACE(mgr, ...)                                                  \
    do {                                                                          \
        if (pbcc_logger_manager_should_log((mgr), PUBNUB_LOG_LEVEL_TRACE))        \
            pbcc_logger_manager_log_text_formatted(                               \
                (mgr), PUBNUB_LOG_LEVEL_TRACE, PUBNUB_LOG_LOCATION, __VA_ARGS__); \
    } while (0)
#else // !PUBNUB_LOG_ENABLED(TRACE)
/**
 * @brief Log a formatted text log entry with `trace` log level (printf-style).
 *
 * @note This is a placeholder for compile-time excluded log level function
 *       calls.
 *
 * @param mgr Pointer to the logger manager.
 * @param ... Format arguments.
 */
#define PBCC_LOG_TRACE(mgr, ...) ((void)(mgr))
#endif // PUBNUB_LOG_ENABLED(TRACE)

#if PUBNUB_LOG_ENABLED(DEBUG)
/**
 * @brief Log a formatted text log entry with `debug` log level (printf-style).
 *
 * @param mgr Pointer to the logger manager.
 * @param ... Format arguments.
 */
#define PBCC_LOG_DEBUG(mgr, ...)                                                  \
    do {                                                                          \
        if (pbcc_logger_manager_should_log((mgr), PUBNUB_LOG_LEVEL_DEBUG))        \
            pbcc_logger_manager_log_text_formatted(                               \
                (mgr), PUBNUB_LOG_LEVEL_DEBUG, PUBNUB_LOG_LOCATION, __VA_ARGS__); \
    } while (0)
#else // !PUBNUB_LOG_ENABLED(DEBUG)
/**
 * @brief Log a formatted text log entry with `debug` log level (printf-style).
 *
 * @note This is a placeholder for compile-time excluded log level function
 *       calls.
 *
 * @param mgr Pointer to the logger manager.
 * @param ... Format arguments.
 */
#define PBCC_LOG_DEBUG(mgr, ...) ((void)(mgr))
#endif // PUBNUB_LOG_ENABLED(DEBUG)

#if PUBNUB_LOG_ENABLED(INFO)
/**
 * @brief Log a formatted text log entry with `info` log level (printf-style).
 *
 * @param mgr Pointer to the logger manager.
 * @param ... Format arguments.
 */
#define PBCC_LOG_INFO(mgr, ...)                                                  \
    do {                                                                         \
        if (pbcc_logger_manager_should_log((mgr), PUBNUB_LOG_LEVEL_INFO))        \
            pbcc_logger_manager_log_text_formatted(                              \
                (mgr), PUBNUB_LOG_LEVEL_INFO, PUBNUB_LOG_LOCATION, __VA_ARGS__); \
    } while (0)
#else // !PUBNUB_LOG_ENABLED(INFO)
/**
 * @brief Log a formatted text log entry with `info` log level (printf-style).
 *
 * @note This is a placeholder for compile-time excluded log level function
 *       calls.
 *
 * @param mgr Pointer to the logger manager.
 * @param ... Format arguments.
 */
#define PBCC_LOG_INFO(mgr, ...) ((void)(mgr))
#endif // PUBNUB_LOG_ENABLED(INFO)

#if PUBNUB_LOG_ENABLED(WARNING)
/**
 * @brief Log a formatted text log entry with `warning` log
 *        level (printf-style).
 *
 * @param mgr Pointer to the logger manager.
 * @param ... Format arguments.
 */
#define PBCC_LOG_WARNING(mgr, ...)                                                  \
    do {                                                                            \
        if (pbcc_logger_manager_should_log((mgr), PUBNUB_LOG_LEVEL_WARNING))        \
            pbcc_logger_manager_log_text_formatted(                                 \
                (mgr), PUBNUB_LOG_LEVEL_WARNING, PUBNUB_LOG_LOCATION, __VA_ARGS__); \
    } while (0)
#else // !PUBNUB_LOG_ENABLED(WARNING)
/**
 * @brief Log a formatted text log entry with `warning` log
 *        level (printf-style).
 *
 * @note This is a placeholder for compile-time excluded log level function
 *       calls.
 *
 * @param mgr Pointer to the logger manager.
 * @param ... Format arguments.
 */
#define PBCC_LOG_WARNING(mgr, ...) ((void)(mgr))
#endif // PUBNUB_LOG_ENABLED(WARNING)

#if PUBNUB_LOG_ENABLED(ERROR)
/**
 * @brief Log a formatted text log entry with `error` log level (printf-style).
 *
 * @param mgr Pointer to the logger manager.
 * @param ... Format arguments.
 */
#define PBCC_LOG_ERROR(mgr, ...)                                                  \
    do {                                                                          \
        if (pbcc_logger_manager_should_log((mgr), PUBNUB_LOG_LEVEL_ERROR))        \
            pbcc_logger_manager_log_text_formatted(                               \
                (mgr), PUBNUB_LOG_LEVEL_ERROR, PUBNUB_LOG_LOCATION, __VA_ARGS__); \
    } while (0)
#else // !PUBNUB_LOG_ENABLED(ERROR)
/**
 * @brief Log a formatted text log entry with `error` log level (printf-style).
 *
 * @note This is a placeholder for compile-time excluded log level function
 *       calls.
 *
 * @param mgr Pointer to the logger manager.
 * @param ... Format arguments.
 */
#define PBCC_LOG_ERROR(mgr, ...) ((void)(mgr))
#endif // PUBNUB_LOG_ENABLED(ERROR)


#endif // #ifndef PBCC_LOGGER_MANAGER_H
