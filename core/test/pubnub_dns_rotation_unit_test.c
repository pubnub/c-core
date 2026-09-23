/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
/* Unit tests for DNS server selection + rotation exhaustion.

   These exercise the REAL get_dns_ip() (static) and pbpal_dns_rotate_server()
   by #including the implementation translation unit directly, so no test-only
   seam is added to production code. The system DNS readers are stubbed so the
   cascade is deterministic and no real network/socket I/O happens. */

#include "cgreen/cgreen.h"

/* This test compiles the REAL socket transport (below), so it needs the
   platform internal header with the socket layer -- not the mock-suite stub
   core/test/pubnub_internal.h that would otherwise shadow it. */
#if defined(_WIN32)
#include "windows/pubnub_internal.h"
#else
#include "posix/pubnub_internal.h"
#endif
#include "core/pubnub_dns_servers.h"

#include <string.h>

/* No system-configured DNS servers -> cascade falls to the compiled-in
   default (8.8.8.8 / Google IPv6), deterministically. */
int pubnub_dns_read_system_servers_ipv4(pubnub_t*                   pb,
                                        struct pubnub_ipv4_address* o_ipv4,
                                        size_t                      n)
{
    (void)pb;
    (void)o_ipv4;
    (void)n;
    return -1;
}
#if PUBNUB_USE_IPV6
int pubnub_dns_read_system_servers_ipv6(pubnub_t*                   pb,
                                        struct pubnub_ipv6_address* o_ipv6,
                                        size_t                      n)
{
    (void)pb;
    (void)o_ipv6;
    (void)n;
    return -1;
}
#endif

/* Pull in the real selection + rotation logic under test. */
#include "lib/sockets/pbpal_resolv_and_connect_sockets.c"

/* Absolute safety net for the "never terminates" case: if rotation does not
   converge within this many iterations the test FAILS (rather than hanging). */
#define ITER_CAP 1000

/* Mimic the netcore timeout loop against a permanently unresponsive DNS
   server: select a server, "time out", rotate. Returns the iteration at which
   the transaction gave up (rotate == 1), or -1 if it never terminated. */
static int run_dead_dns(pubnub_t* pb, sa_family_t family, int max_iter)
{
    int i;
    for (i = 0; i < max_iter; ++i) {
        sockaddr_inX_t addr;
        memset(&addr, 0, sizeof addr);
        get_dns_ip(pb, family, &pb->dns_check, (struct sockaddr*)&addr);
        if (pbpal_dns_rotate_server(pb) == 1) { return i + 1; }
    }
    return -1;
}

/* One loop step that also reports the selected IPv4 address (network order). */
static uint32_t select_ipv4(pubnub_t* pb, sa_family_t family)
{
    sockaddr_inX_t addr;
    memset(&addr, 0, sizeof addr);
    get_dns_ip(pb, family, &pb->dns_check, (struct sockaddr*)&addr);
    if (((struct sockaddr*)&addr)->sa_family != AF_INET) { return 0; }
    return ((struct sockaddr_in*)&addr)->sin_addr.s_addr;
}

/* Reset the per-transaction DNS state as initialize_fields_in_state_IDLE(). */
static void idle_reset(pubnub_t* pb)
{
    pb->dns_check.dns_server_check    = 0;
    pb->dns_check.last_server_reached = 0;
    pb->flags.rotations_count         = 0;
    pb->flags.sent_queries            = 0;
}

static pubnub_t m_pb;

Describe(dns_rotation);
BeforeEach(dns_rotation)
{
    memset(&m_pb, 0, sizeof m_pb);
    /* Clear any DNS servers left set by a previous test. */
    pubnub_dns_set_primary_server_ipv4_str(&m_pb, "0.0.0.0");
    pubnub_dns_set_secondary_server_ipv4_str(&m_pb, "0.0.0.0");
#if PUBNUB_USE_IPV6
    pubnub_dns_set_primary_server_ipv6_str(&m_pb, "::");
    pubnub_dns_set_secondary_server_ipv6_str(&m_pb, "::");
#endif
    idle_reset(&m_pb);
}
AfterEach(dns_rotation) {}

/* The core regression: a dead DNS server must not loop forever. */
Ensure(dns_rotation, af_inet_no_user_servers_terminates)
{
    int r = run_dead_dns(&m_pb, AF_INET, ITER_CAP);
    assert_that(r, is_greater_than(0));
    /* one server slot (the default) per rotation */
    assert_that(r, is_equal_to(PUBNUB_MAX_DNS_ROTATION));
}

Ensure(dns_rotation, af_inet_primary_only_terminates)
{
    pubnub_dns_set_primary_server_ipv4_str(&m_pb, "100.64.0.53");
    idle_reset(&m_pb);
    int r = run_dead_dns(&m_pb, AF_INET, ITER_CAP);
    assert_that(r, is_greater_than(0));
    /* primary + default = 2 slots per rotation */
    assert_that(r, is_equal_to(PUBNUB_MAX_DNS_ROTATION * 2));
}

Ensure(dns_rotation, af_inet_primary_and_secondary_terminates)
{
    pubnub_dns_set_primary_server_ipv4_str(&m_pb, "100.64.0.53");
    pubnub_dns_set_secondary_server_ipv4_str(&m_pb, "100.64.0.54");
    idle_reset(&m_pb);
    int r = run_dead_dns(&m_pb, AF_INET, ITER_CAP);
    assert_that(r, is_greater_than(0));
    /* primary + secondary + default = 3 slots per rotation */
    assert_that(r, is_equal_to(PUBNUB_MAX_DNS_ROTATION * 3));
}

#if PUBNUB_USE_IPV6
/* Corner case: IPv6-enabled build, IPv6 unavailable at runtime (family
   resolves to AF_INET), only IPv4 user servers configured, and no system DNS.
   Must still terminate. */
Ensure(dns_rotation, af_inet6_family_ipv4_users_no_system_terminates)
{
    pubnub_dns_set_primary_server_ipv4_str(&m_pb, "100.64.0.53");
    pubnub_dns_set_secondary_server_ipv4_str(&m_pb, "100.64.0.54");
    idle_reset(&m_pb);
    int r = run_dead_dns(&m_pb, AF_INET6, ITER_CAP);
    assert_that(r, is_greater_than(0));
}

Ensure(dns_rotation, af_inet6_no_user_servers_terminates)
{
    int r = run_dead_dns(&m_pb, AF_INET6, ITER_CAP);
    assert_that(r, is_greater_than(0));
}
#endif

/* The DNS send-error path (check_dns_server_error) must converge too: a
   non-terminal server requests a retry, the last-resort server does not. */
Ensure(dns_rotation, error_path_sets_retry_only_for_non_terminal)
{
    pubnub_dns_set_primary_server_ipv4_str(&m_pb, "100.64.0.53");
    idle_reset(&m_pb);

    sockaddr_inX_t addr;
    /* First selection: user primary (non-terminal). */
    memset(&addr, 0, sizeof addr);
    get_dns_ip(&m_pb, AF_INET, &m_pb.dns_check, (struct sockaddr*)&addr);
    assert_that(m_pb.dns_check.last_server_reached, is_equal_to(0));
    m_pb.flags.retry_after_close = false;
    check_dns_server_error(&m_pb.dns_check, &m_pb.flags);
    assert_that(m_pb.flags.retry_after_close, is_equal_to(true));

    /* Advance to the last-resort server (primary now marked failed). */
    memset(&addr, 0, sizeof addr);
    get_dns_ip(&m_pb, AF_INET, &m_pb.dns_check, (struct sockaddr*)&addr);
    assert_that(m_pb.dns_check.last_server_reached, is_equal_to(1));
    m_pb.flags.retry_after_close = false;
    check_dns_server_error(&m_pb.dns_check, &m_pb.flags);
    assert_that(m_pb.flags.retry_after_close, is_equal_to(false));
}

/* After a full rotation resets the failed-server bitmap, the user primary must
   be selected again (guards against the "stuck on OS DNS forever" symptom). */
Ensure(dns_rotation, primary_reselected_after_rotation)
{
    pubnub_dns_set_primary_server_ipv4_str(&m_pb, "100.64.0.53");
    idle_reset(&m_pb);

    struct pubnub_ipv4_address prim;
    inet_pton(AF_INET, "100.64.0.53", &prim);
    uint32_t primary = *(uint32_t*)prim.ipv4;

    /* Sweep 1: primary selected. */
    assert_that(select_ipv4(&m_pb, AF_INET), is_equal_to(primary));
    assert_that(pbpal_dns_rotate_server(&m_pb), is_equal_to(0));
    /* Sweep 2: primary is now marked failed -> default (last resort). */
    assert_that(select_ipv4(&m_pb, AF_INET), is_not_equal_to(primary));
    assert_that(pbpal_dns_rotate_server(&m_pb), is_equal_to(0));
    /* Rotation reset the bitmap -> primary selected again. */
    assert_that(select_ipv4(&m_pb, AF_INET), is_equal_to(primary));
}

/* Per-transaction budget must not depend on the previous transaction's
   outcome: two consecutive dead-server transactions give up identically. */
Ensure(dns_rotation, rotation_budget_symmetric_across_transactions)
{
    pubnub_dns_set_primary_server_ipv4_str(&m_pb, "100.64.0.53");
    idle_reset(&m_pb);
    int r1 = run_dead_dns(&m_pb, AF_INET, ITER_CAP);
    idle_reset(&m_pb);
    int r2 = run_dead_dns(&m_pb, AF_INET, ITER_CAP);
    assert_that(r1, is_greater_than(0));
    assert_that(r2, is_equal_to(r1));
}

int main(int argc, char* argv[])
{
    TestSuite* suite = create_test_suite();
    add_test_with_context(suite, dns_rotation, af_inet_no_user_servers_terminates);
    add_test_with_context(suite, dns_rotation, af_inet_primary_only_terminates);
    add_test_with_context(suite, dns_rotation, af_inet_primary_and_secondary_terminates);
#if PUBNUB_USE_IPV6
    add_test_with_context(suite, dns_rotation, af_inet6_family_ipv4_users_no_system_terminates);
    add_test_with_context(suite, dns_rotation, af_inet6_no_user_servers_terminates);
#endif
    add_test_with_context(suite, dns_rotation, error_path_sets_retry_only_for_non_terminal);
    add_test_with_context(suite, dns_rotation, primary_reselected_after_rotation);
    add_test_with_context(suite, dns_rotation, rotation_budget_symmetric_across_transactions);
    if (argc > 1) { return run_single_test(suite, argv[1], create_text_reporter()); }
    return run_test_suite(suite, create_text_reporter());
}
