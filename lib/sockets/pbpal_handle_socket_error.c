/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"

#include "core/pbpal.h"
#include "core/pubnub_assert.h"
#include "core/pubnub_log.h"

#include <string.h>


void pbpal_report_error_from_environment(pubnub_t* pb, char const* file, int line)
{
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
    PUBNUB_LOG_WARNING(
        "%s:%d: pbpal_report_error_from_environment(pb=%p): errno=%d('%s')",
        file,
        line,
        pb,
        errno,
        err_str);
    if (pb != NULL) {
#if PUBNUB_BLOCKING_IO_SETTABLE
        PUBNUB_LOG_DEBUG(" use_blocking_io=%d\n", (int)pb->options.use_blocking_io);
#else
        PUBNUB_LOG_DEBUG("\n");
#endif // PUBNUB_BLOCKING_IO_SETTABLE
    }
    else {
        PUBNUB_LOG_DEBUG("\n");
    }
#if defined(_WIN32)
    PUBNUB_LOG_DEBUG("%s:%d: pbpal_report_error_from_environment(pb=%p): errno=%d('%s') "
                     "GetLastError()=%lu "
                     "WSAGetLastError()=%d\n",
                     file,
                     line,
                     pb,
                     errno,
                     err_str,
                     GetLastError(),
                     WSAGetLastError());
#endif
}


enum pubnub_res pbpal_handle_socket_error(int socket_result,
                                          pubnub_t* pb,
                                          char const* file,
                                          int line)
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
            return PNR_IN_PROGRESS;
        }
        else {
            pb->sock_state = STATE_NONE;
            pbpal_report_error_from_environment(pb, file, line);
            return socket_timed_out() ? PNR_CONNECTION_TIMEOUT : PNR_IO_ERROR;
        }
    }
    else if (0 == socket_result) {
        pb->sock_state = STATE_NONE;
        return PNR_TIMEOUT;
    }
    pb->sock_state = STATE_NONE;
    return PNR_INTERNAL_ERROR;
}
