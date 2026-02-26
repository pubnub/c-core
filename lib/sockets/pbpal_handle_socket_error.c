/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"

#include "core/pbpal.h"
#include "core/pubnub_assert.h"
#if PUBNUB_USE_LOGGER
#include "core/pubnub_logger.h"
#endif // PUBNUB_USE_LOGGER

#include <string.h>


void pbpal_report_error_from_environment(
    pubnub_t*   pb,
    char const* file,
    int         line)
{
#if PUBNUB_LOG_ENABLED(WARNING)
    char const* err_str;
#if HAVE_STRERROR_R
    char errstr_r[1024];
    strerror_r(errno, errstr_r, sizeof errstr_r / sizeof errstr_r[0]);
    err_str = errstr_r;
#elif HAVE_STRERROR_S
    char errstr_s[1024];
    strerror_s(errstr_s, sizeof errstr_s / sizeof errstr_s[0], errno);
    err_str = errstr_s;
#else
    err_str = strerror(errno);
#endif
    if (pubnub_logger_should_log(pb, PUBNUB_LOG_LEVEL_WARNING)) {
        pubnub_log_value_t data = pubnub_log_value_map_init();
        PUBNUB_LOG_MAP_SET_NUMBER(&data, errno, code)
        PUBNUB_LOG_MAP_SET_STRING(&data, err_str, details)
        pubnub_log_object(
            pb,
            PUBNUB_LOG_LEVEL_WARNING,
            PUBNUB_LOG_LOCATION,
            &data,
            "Error from environment:");
    }
#endif
    if (pb != NULL) {
#if PUBNUB_BLOCKING_IO_SETTABLE
        PUBNUB_LOG_DEBUG(
            pb,
            pb->options.use_blocking_io ? "Using blocking IO"
                                        : "Using non-blocking IO");
#endif // PUBNUB_BLOCKING_IO_SETTABLE
    }
#if defined(_WIN32)
#if PUBNUB_LOG_ENABLED(WARNING)
    if (pubnub_logger_should_log(pb, PUBNUB_LOG_LEVEL_WARNING)) {
        pubnub_log_value_t data = pubnub_log_value_map_init();
        PUBNUB_LOG_MAP_SET_NUMBER(&data, errno, code)
        PUBNUB_LOG_MAP_SET_STRING(&data, err_str, details)
        PUBNUB_LOG_MAP_SET_NUMBER(&data, GetLastError(), get_last_error)
        PUBNUB_LOG_MAP_SET_NUMBER(&data, WSAGetLastError(), wsa_get_last_error)
        pubnub_log_object(
            pb,
            PUBNUB_LOG_LEVEL_WARNING,
            PUBNUB_LOG_LOCATION,
            &data,
            "Error from environment:");
    }
#endif
#endif
}


enum pubnub_res pbpal_handle_socket_error(
    int         socket_result,
    pubnub_t*   pb,
    char const* file,
    int         line)
{
    PUBNUB_ASSERT_INT_OPT(socket_result, <=, 0);
    if (socket_result < 0) {
        if (socket_would_block()) {
#if PUBNUB_BLOCKING_IO_SETTABLE
            if (pb->options.use_blocking_io) {
                pb->sock_state = STATE_NONE;
                return PNR_TIMEOUT;
            }
#endif
            PUBNUB_LOG_TRACE(pb, "Socket would block.");
            return PNR_IN_PROGRESS;
        }
        else {
            // Whether socket already in use for data sending / receiving or
            // not.
            const bool handles_data = STATE_READ == pb->sock_state ||
                                      STATE_NEWDATA_EXHAUSTED ==
                                          pb->sock_state ||
                                      STATE_READ_LINE == pb->sock_state ||
                                      STATE_SENDING_DATA == pb->sock_state;
            const bool timed_out = socket_timed_out();
            pb->sock_state       = STATE_NONE;
            pbpal_report_error_from_environment(pb, file, line);

            // Report data sending / receive timeout, which happened in the
            // middle of the operation.
            if (timed_out && handles_data) return PNR_TIMEOUT;

            return timed_out ? PNR_CONNECTION_TIMEOUT : PNR_IO_ERROR;
        }
    }
    else if (0 == socket_result) {
        pb->sock_state = STATE_NONE;
        return PNR_TIMEOUT;
    }
    pb->sock_state = STATE_NONE;
    return PNR_INTERNAL_ERROR;
}
