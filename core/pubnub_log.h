/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_LOG
#define	INC_PUBNUB_LOG


/** @file pubnub_log.h 
    This is the "Log" API of the Pubnub client library.
    Designed for C-core's own use, but not restricted to it.
    Provides a reasonably-well-featured, yet small and efficient
    API for logging.
*/

/** Available Log levels */
enum pubnub_log_level {
    /** Nothing is logged */
    PUBNUB_LOG_LEVEL_NONE,
    /** Only Errors are logged */
    PUBNUB_LOG_LEVEL_ERROR,
    /** Warnings (and errors) are logged */
    PUBNUB_LOG_LEVEL_WARNING,
    /** Informational messages (and warnings and errors) are logged */
    PUBNUB_LOG_LEVEL_INFO,
    /** Debugging messages (and info and warning and errors) are logged */
    PUBNUB_LOG_LEVEL_DEBUG,
    /** Tracing messages (and debug and info and warning and errors)
     * are logged */
    PUBNUB_LOG_LEVEL_TRACE
};


#if !defined PUBNUB_LOG_LEVEL
/** User should define the log level to use, by defining the
    `PUBNUB_LOG_LEVEL` symbol. If PUBNUB_LOG_LEVEL is not defined,
    this default is used */
#define PUBNUB_LOG_LEVEL PUBNUB_LOG_LEVEL_INFO
#endif

#if !defined PUBNUB_LOG_PRINTF
#include <stdio.h>
/** User should define a printf-like function that will do the actual
    logging. If it is not defined, we'll use printf().
*/
#define PUBNUB_LOG_PRINTF(...) printf(__VA_ARGS__)
#endif

/** Generic logging macro, logs to given @a LVL using a printf-like
    interface
*/
#define PUBNUB_LOG(LVL, ...) do { if (LVL <= PUBNUB_LOG_LEVEL) PUBNUB_LOG_PRINTF(__VA_ARGS__); } while(0)

/** Helper macro to log an error message */
#define PUBNUB_LOG_ERROR(...) PUBNUB_LOG(PUBNUB_LOG_LEVEL_ERROR, __VA_ARGS__)

/** Helper macro to log a warning message */
#define PUBNUB_LOG_WARNING(...) PUBNUB_LOG(PUBNUB_LOG_LEVEL_WARNING, __VA_ARGS__)

/** Helper macro to log an informational message */
#define PUBNUB_LOG_INFO(...) PUBNUB_LOG(PUBNUB_LOG_LEVEL_INFO, __VA_ARGS__)

/** Helper macro to log a debug message */
#define PUBNUB_LOG_DEBUG(...) PUBNUB_LOG(PUBNUB_LOG_LEVEL_DEBUG, __VA_ARGS__)

/** Helper macro to log a tracing message */
#define PUBNUB_LOG_TRACE(...) PUBNUB_LOG(PUBNUB_LOG_LEVEL_TRACE, __VA_ARGS__)

/** Generic macro to print a value of a (scalar) variable.
 */
#define WATCH_VAL(X,T,FMT) do { (void)(1?&X:(T*)0); PUBNUB_LOG(PUBNUB_LOG_LEVEL_DEBUG, __FILE__ "(%d) in %s: `" #X "` = " FMT "\n", __LINE__, __FUNCTION__, X); } while (0)

/** Helper macro to "watch" an integer */
#define WATCH_INT(X) WATCH_VAL(X, int, "%d")

/** Helper macro to "watch" an unsigned integer */
#define WATCH_UINT(X) WATCH_VAL(X, unsigned int, "%u")

/** Helper macro to "watch" a long */
#define WATCH_LONG(X) WATCH_VAL(X, long, "%ld")

/** Helper macro to "watch" an unsigned long */
#define WATCH_ULONG(X) WATCH_VAL(X, unsigned long, "%lu")

/** Helper macro to "watch" a size_t. We're not using the `%z`,
    because it requires a C99 compiler/runtime. OTOH, we're not aware
    of a platform on which `size_t` is "larger" than `unsigned long`.
 */
#define WATCH_SIZE_T(X) do { unsigned long M_watched_val_ = (X); WATCH_ULONG(M_watched_val_); } while (0)

/** Helper macro to "watch" a short */
#define WATCH_SHORT(X) WATCH_VAL(X, short, "%hd")

/** Helper macro to "watch" an unsigned short */
#define WATCH_USHORT(X) WATCH_VAL(X, unsigned short, "%hu")

/** Helper macro to "watch" a char */
#define WATCH_CHAR(X) WATCH_VAL(X, char, "%c")

/** Helper macro to "watch" an unsigned char */
#define WATCH_UCHAR(X) WATCH_VAL(X, unsigned char, "%uc")

/** Helper macro to "watch" an enum like an integer.*/
#define WATCH_ENUM(X) do { int x_ = (X); PUBNUB_LOG(PUBNUB_LOG_LEVEL_DEBUG, __FILE__ "(%d) in %s: `" #X "` = %d\n", __LINE__, __FUNCTION__, x_); } while (0)
    
/** Helper macro to "watch" a string (char pointer) */
#define WATCH_STR(X) do { char const*s_ = (X); PUBNUB_LOG(PUBNUB_LOG_LEVEL_DEBUG, __FILE__ "(%d) in %s: `" #X "` = '%s'\n", __LINE__, __FUNCTION__, s_); } while (0)

/** Helper macro to "watch" an array of bytes */
#define WATCH_BYTEARR(X) do { unsigned char const*ba_ = (X); PUBNUB_LOG(PUBNUB_LOG_LEVEL_DEBUG, __FILE__ "(%d) in %s: `" #X "` = '%s'\n", __LINE__, __FUNCTION__, s_); } while (0)



#endif /*!defined INC_PUBNUB_LOG*/
