/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"

#include "pubnub_get_native_socket.h"
#include "core/pbpal_ntf_callback_poller.h"
#include "core/pubnub_assert.h"

#include "cgreen/cgreen.h"
#include "cgreen/mocks.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <poll.h>

#define attest assert_that
#define equals is_equal_to
#define differs is_not_equal_to

/* Assert handler */
static bool    m_expect_Assert;
static jmp_buf m_Assert_exp_jmpbuf;

void assert_handler(char const* s, const char* file, long i)
{
    printf("%s:%ld: Pubnub assert failed '%s'\n", file, i, s);
    if (m_expect_Assert) {
        m_expect_Assert = false;
        longjmp(m_Assert_exp_jmpbuf, 1);
    }
}

/* Track requeue calls */
static pubnub_t* s_last_requeued;
static int       s_requeue_count;

int pbntf_requeue_for_processing(pubnub_t* pb)
{
    s_last_requeued = pb;
    ++s_requeue_count;
    return 0;
}

pbpal_native_socket_t pubnub_get_native_socket(pubnub_t* pb)
{
    if (NULL == pb) { return -1; }
    return pb->pal.socket;
}

void pb_sleep_ms(unsigned long ms) { (void)ms; }


Describe(pbpal_poller_poll);

BeforeEach(pbpal_poller_poll) {
    s_last_requeued = NULL;
    s_requeue_count = 0;
    m_expect_Assert = false;
}

AfterEach(pbpal_poller_poll) {}


Ensure(pbpal_poller_poll, init_and_deinit)
{
    struct pbpal_poll_data* data = pbpal_ntf_callback_poller_init();
    attest(data, differs(NULL));

    pbpal_ntf_callback_poller_deinit(&data);
    attest(data == NULL);
}


Ensure(pbpal_poller_poll, save_and_remove_socket)
{
    struct pbpal_poll_data* data = pbpal_ntf_callback_poller_init();
    pubnub_t ctx = { 0 };
    int sv[2];

    attest(socketpair(AF_UNIX, SOCK_STREAM, 0, sv), equals(0));
    ctx.pal.socket = sv[0];

    pbpal_ntf_callback_save_socket(data, &ctx);

    pbpal_ntf_callback_remove_socket(data, &ctx);

    close(sv[0]);
    close(sv[1]);
    pbpal_ntf_callback_poller_deinit(&data);
}


Ensure(pbpal_poller_poll, watch_in_and_out_events)
{
    struct pbpal_poll_data* data = pbpal_ntf_callback_poller_init();
    pubnub_t ctx = { 0 };
    int sv[2];

    attest(socketpair(AF_UNIX, SOCK_STREAM, 0, sv), equals(0));
    ctx.pal.socket = sv[0];

    pbpal_ntf_callback_save_socket(data, &ctx);

    attest(pbpal_ntf_watch_in_events(data, &ctx), equals(0));
    attest(pbpal_ntf_watch_out_events(data, &ctx), equals(0));

    pbpal_ntf_callback_remove_socket(data, &ctx);
    close(sv[0]);
    close(sv[1]);
    pbpal_ntf_callback_poller_deinit(&data);
}


Ensure(pbpal_poller_poll, poll_away_detects_writable_socket)
{
    struct pbpal_poll_data* data = pbpal_ntf_callback_poller_init();
    pubnub_t ctx = { 0 };
    int sv[2];

    attest(socketpair(AF_UNIX, SOCK_STREAM, 0, sv), equals(0));
    ctx.pal.socket = sv[0];

    pbpal_ntf_callback_save_socket(data, &ctx);
    /* sv[0] should be immediately writable */
    int rslt = pbpal_ntf_poll_away(data, 100);
    attest(rslt > 0);
    attest(s_requeue_count, equals(1));
    attest(s_last_requeued, equals(&ctx));

    pbpal_ntf_callback_remove_socket(data, &ctx);
    close(sv[0]);
    close(sv[1]);
    pbpal_ntf_callback_poller_deinit(&data);
}


Ensure(pbpal_poller_poll, poll_away_detects_readable_socket)
{
    struct pbpal_poll_data* data = pbpal_ntf_callback_poller_init();
    pubnub_t ctx = { 0 };
    int sv[2];

    attest(socketpair(AF_UNIX, SOCK_STREAM, 0, sv), equals(0));
    ctx.pal.socket = sv[0];

    pbpal_ntf_callback_save_socket(data, &ctx);
    pbpal_ntf_watch_in_events(data, &ctx);

    /* Write to sv[1] so sv[0] becomes readable */
    write(sv[1], "x", 1);

    int rslt = pbpal_ntf_poll_away(data, 100);
    attest(rslt > 0);
    attest(s_requeue_count, equals(1));

    pbpal_ntf_callback_remove_socket(data, &ctx);
    close(sv[0]);
    close(sv[1]);
    pbpal_ntf_callback_poller_deinit(&data);
}


Ensure(pbpal_poller_poll, poll_away_timeout_when_not_ready)
{
    struct pbpal_poll_data* data = pbpal_ntf_callback_poller_init();
    pubnub_t ctx = { 0 };
    int sv[2];

    attest(socketpair(AF_UNIX, SOCK_STREAM, 0, sv), equals(0));
    ctx.pal.socket = sv[0];

    pbpal_ntf_callback_save_socket(data, &ctx);
    /* Watch for read but don't write anything — should timeout */
    pbpal_ntf_watch_in_events(data, &ctx);

    int rslt = pbpal_ntf_poll_away(data, 1);
    attest(rslt, equals(0));
    attest(s_requeue_count, equals(0));

    pbpal_ntf_callback_remove_socket(data, &ctx);
    close(sv[0]);
    close(sv[1]);
    pbpal_ntf_callback_poller_deinit(&data);
}


Ensure(pbpal_poller_poll, multiple_sockets)
{
    struct pbpal_poll_data* data = pbpal_ntf_callback_poller_init();
    pubnub_t ctx1 = { 0 }, ctx2 = { 0 }, ctx3 = { 0 };
    int sv1[2], sv2[2], sv3[2];

    attest(socketpair(AF_UNIX, SOCK_STREAM, 0, sv1), equals(0));
    attest(socketpair(AF_UNIX, SOCK_STREAM, 0, sv2), equals(0));
    attest(socketpair(AF_UNIX, SOCK_STREAM, 0, sv3), equals(0));

    ctx1.pal.socket = sv1[0];
    ctx2.pal.socket = sv2[0];
    ctx3.pal.socket = sv3[0];

    pbpal_ntf_callback_save_socket(data, &ctx1);
    pbpal_ntf_callback_save_socket(data, &ctx2);
    pbpal_ntf_callback_save_socket(data, &ctx3);

    /* Remove middle one */
    pbpal_ntf_callback_remove_socket(data, &ctx2);

    /* Poll — ctx1 and ctx3 should be writable */
    s_requeue_count = 0;
    int rslt = pbpal_ntf_poll_away(data, 100);
    attest(rslt > 0);
    attest(s_requeue_count, equals(2));

    pbpal_ntf_callback_remove_socket(data, &ctx1);
    pbpal_ntf_callback_remove_socket(data, &ctx3);
    close(sv1[0]); close(sv1[1]);
    close(sv2[0]); close(sv2[1]);
    close(sv3[0]); close(sv3[1]);
    pbpal_ntf_callback_poller_deinit(&data);
}


Ensure(pbpal_poller_poll, works_with_high_fd_numbers)
{
    struct pbpal_poll_data* data = pbpal_ntf_callback_poller_init();
    pubnub_t ctx = { 0 };
    int sv[2];

    attest(socketpair(AF_UNIX, SOCK_STREAM, 0, sv), equals(0));

    /* Move sv[0] to a high fd number (>= FD_SETSIZE which is typically 1024).
       This is the exact scenario that triggers the bug with select()/FD_SET. */
    int high_fd = dup2(sv[0], 1500);
    /* dup2 must succeed — CI runners should have ulimit >= 1500.
       If this fails the test environment is misconfigured. */
    attest(high_fd, equals(1500));
    close(sv[0]);
    ctx.pal.socket = high_fd;

    pbpal_ntf_callback_save_socket(data, &ctx);

    /* The high-fd socket should be immediately writable —
       this would crash with the old select()/FD_SET approach. */
    int rslt = pbpal_ntf_poll_away(data, 100);
    attest(rslt > 0);
    attest(s_requeue_count, equals(1));
    attest(s_last_requeued, equals(&ctx));

    pbpal_ntf_callback_remove_socket(data, &ctx);
    close(high_fd);
    close(sv[1]);
    pbpal_ntf_callback_poller_deinit(&data);
}


Ensure(pbpal_poller_poll, update_socket)
{
    struct pbpal_poll_data* data = pbpal_ntf_callback_poller_init();
    pubnub_t ctx = { 0 };
    int sv1[2], sv2[2];

    attest(socketpair(AF_UNIX, SOCK_STREAM, 0, sv1), equals(0));
    attest(socketpair(AF_UNIX, SOCK_STREAM, 0, sv2), equals(0));

    ctx.pal.socket = sv1[0];
    pbpal_ntf_callback_save_socket(data, &ctx);

    /* Simulate socket change */
    ctx.pal.socket = sv2[0];
    pbpal_ntf_callback_update_socket(data, &ctx);

    /* Poll should work with the new socket */
    int rslt = pbpal_ntf_poll_away(data, 100);
    attest(rslt > 0);
    attest(s_requeue_count, equals(1));

    pbpal_ntf_callback_remove_socket(data, &ctx);
    close(sv1[0]); close(sv1[1]);
    close(sv2[0]); close(sv2[1]);
    pbpal_ntf_callback_poller_deinit(&data);
}


Ensure(pbpal_poller_poll, high_fd_poll_connect_pattern)
{
    /* This test directly mirrors the pattern used in pbpal_check_connect():
       a non-blocking TCP socket with fd >= FD_SETSIZE polled for POLLOUT.
       With the old select()/FD_SET approach this would corrupt the stack. */
    int skt = socket(AF_INET, SOCK_STREAM, 0);
    attest(skt >= 0);

    int high_fd = dup2(skt, 1500);
    attest(high_fd, equals(1500));
    close(skt);

    /* Set non-blocking (same as SDK does before connect) */
    int flags = fcntl(high_fd, F_GETFL, 0);
    fcntl(high_fd, F_SETFL, flags | O_NONBLOCK);

    /* Initiate non-blocking connect to localhost:1 (will get ECONNREFUSED
       or EINPROGRESS — either is fine, we just need to poll) */
    struct sockaddr_in addr = { 0 };
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    connect(high_fd, (struct sockaddr*)&addr, sizeof(addr));

    /* This is the exact pattern from pbpal_check_connect — poll for POLLOUT */
    struct pollfd pfd;
    pfd.fd      = high_fd;
    pfd.events  = POLLOUT;
    pfd.revents = 0;
    int rslt = poll(&pfd, 1, 300);

    /* We expect either timeout (0) or ready (>0, connection refused is still
       an event). The critical assertion is that we reach this point without
       crashing — with FD_SET on fd=1500 the stack would be corrupted. */
    attest(rslt >= 0);
    if (rslt > 0) {
        attest(pfd.revents & (POLLOUT | POLLERR | POLLHUP));
    }

    close(high_fd);
}
