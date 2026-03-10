/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
/**
 * @file dns_discovery_test.c
 * @brief Windows DNS server discovery test harness.
 *
 * Tests the SDK's `pubnub_dns_read_system_servers_ipv4` (and IPv6)
 * implementation against various network scenarios.
 *
 * Build (MSVC):
 *   cl /nologo /W3 /Fe:dns_discovery_test.exe ^
 *     /DPUBNUB_CALLBACK_API /DPUBNUB_USE_IPV6=1 /DPUBNUB_USE_SSL=0 ^
 *     /DPUBNUB_SET_DNS_SERVERS=1 /DPUBNUB_USE_LOGGER=1 ^
 *     /DPUBNUB_USE_DEFAULT_LOGGER=0 /DPUBNUB_LOG_MIN_LEVEL=NONE ^
 *     /DPUBNUB_DYNAMIC_REPLY_BUFFER=1 /DPUBNUB_RECEIVE_GZIP_RESPONSE=0 ^
 *     /DPUBNUB_ADVANCED_KEEP_ALIVE=0 ^
 *     /DPUBNUB_USE_SUBSCRIBE_EVENT_ENGINE=0 /DPUBNUB_USE_SUBSCRIBE_V2=0 ^
 *     /DPUBNUB_CRYPTO_API=0 /DPUBNUB_USE_GZIP_COMPRESSION=0 ^
 *     /DPUBNUB_USE_GRANT_TOKEN_API=0 /DPUBNUB_USE_REVOKE_TOKEN_API=0 ^
 *     /DPUBNUB_USE_ACTIONS_API=0 /DPUBNUB_USE_OBJECTS_API=0 ^
 *     /DPUBNUB_USE_AUTO_HEARTBEAT=0 /DPUBNUB_USE_RETRY_CONFIGURATION=0 ^
 *     /D_WINSOCKAPI_ ^
 *     /I. /Icore /Icore/c99 /Iwindows /Ilib /Ilib/base64 ^
 *     tests\dns_discovery\dns_discovery_test.c ^
 *     windows\pubnub_dns_system_servers.c ^
 *     core\pubnub_assert_std.c ^
 *     ws2_32.lib iphlpapi.lib
 *
 * Run:
 *   dns_discovery_test.exe <scenario>
 *
 * Scenarios: baseline, loopback, disabled, metric, concurrent, ipv6,
 *            stability, buffer_edge, broadcast, no_dns, flapping,
 *            multi_dns, boundary, dedup, all
 * Exit code: number of failures (0 = all pass)
 */

#if defined(_WIN32)

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <process.h>

#include "core/pubnub_dns_servers.h"
#include "core/pubnub_assert.h"


/* ------------------------------------------------------------------ */
/*                         Test infrastructure                         */
/* ------------------------------------------------------------------ */

static int g_passes  = 0;
static int g_fails   = 0;
static int g_skipped = 0;

#define TEST_PASS(name, ...)                                                   \
    do {                                                                       \
        printf("  [PASS] %s", name);                                           \
        printf(" " __VA_ARGS__);                                               \
        printf("\n");                                                          \
        g_passes++;                                                            \
    } while (0)

#define TEST_FAIL(name, ...)                                                   \
    do {                                                                       \
        printf("  [FAIL] %s: ", name);                                         \
        printf(__VA_ARGS__);                                                   \
        printf("\n");                                                          \
        g_fails++;                                                             \
    } while (0)

#define TEST_SKIP(name, ...)                                                   \
    do {                                                                       \
        printf("  [SKIP] %s: ", name);                                         \
        printf(__VA_ARGS__);                                                   \
        printf("\n");                                                          \
        g_skipped++;                                                           \
    } while (0)


/* ------------------------------------------------------------------ */
/*                            Helpers                                   */
/* ------------------------------------------------------------------ */

/** Format an IPv4 address from 4 raw bytes into a string. */
static void fmt_ipv4(const uint8_t addr[4], char* buf, size_t buf_size)
{
    snprintf(buf, buf_size, "%u.%u.%u.%u", addr[0], addr[1], addr[2], addr[3]);
}

#if PUBNUB_USE_IPV6
/** Format an IPv6 address from 16 raw bytes into a string. */
static void fmt_ipv6(const uint8_t addr[16], char* buf, size_t buf_size)
{
    inet_ntop(AF_INET6, addr, buf, (socklen_t)buf_size);
}
#endif

/** Check if an adapter named `name` exists. */
static bool adapter_exists(const char* name)
{
    IP_ADAPTER_ADDRESSES* addrs  = NULL;
    ULONG                 buflen = 15000;
    DWORD                 ret;

    addrs = (IP_ADAPTER_ADDRESSES*)malloc(buflen);
    if (!addrs) return false;

    ret = GetAdaptersAddresses(
        AF_UNSPEC,
        GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST,
        NULL,
        addrs,
        &buflen);

    if (ret == ERROR_BUFFER_OVERFLOW) {
        free(addrs);
        addrs = (IP_ADAPTER_ADDRESSES*)malloc(buflen);
        if (!addrs) return false;
        ret = GetAdaptersAddresses(
            AF_UNSPEC,
            GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST,
            NULL,
            addrs,
            &buflen);
    }

    if (ret != NO_ERROR) {
        free(addrs);
        return false;
    }

    bool found = false;
    for (IP_ADAPTER_ADDRESSES* aa = addrs; aa; aa = aa->Next) {
        /* Compare both friendly name (wide) and adapter name (narrow). */
        if (aa->AdapterName && strcmp(aa->AdapterName, name) == 0) {
            found = true;
            break;
        }
        if (aa->FriendlyName) {
            char narrow[256];
            WideCharToMultiByte(
                CP_UTF8,
                0,
                aa->FriendlyName,
                -1,
                narrow,
                sizeof(narrow),
                NULL,
                NULL);
            if (strcmp(narrow, name) == 0) {
                found = true;
                break;
            }
        }
    }

    free(addrs);
    return found;
}


/* ------------------------------------------------------------------ */
/*                     Phase 1: Baseline tests                         */
/* ------------------------------------------------------------------ */

static void test_basic_discovery(void)
{
    const char*                name = "basic_discovery";
    struct pubnub_ipv4_address addrs[8];
    int count = pubnub_dns_read_system_servers_ipv4(NULL, addrs, 8);

    if (count <= 0) {
        TEST_FAIL(name, "expected >0 DNS servers, got %d", count);
        return;
    }
    TEST_PASS(name, "(found %d server(s))", count);
}

static void test_no_loopback_returned(void)
{
    const char*                name = "no_loopback_returned";
    struct pubnub_ipv4_address addrs[8];
    int count = pubnub_dns_read_system_servers_ipv4(NULL, addrs, 8);

    if (count <= 0) {
        TEST_SKIP(name, "no DNS servers discovered");
        return;
    }

    for (int i = 0; i < count; i++) {
        if (addrs[i].ipv4[0] == 127) {
            char buf[20];
            fmt_ipv4(addrs[i].ipv4, buf, sizeof(buf));
            TEST_FAIL(name, "loopback address returned: %s", buf);
            return;
        }
    }
    TEST_PASS(name, "");
}

static void test_no_apipa_returned(void)
{
    const char*                name = "no_apipa_returned";
    struct pubnub_ipv4_address addrs[8];
    int count = pubnub_dns_read_system_servers_ipv4(NULL, addrs, 8);

    if (count <= 0) {
        TEST_SKIP(name, "no DNS servers discovered");
        return;
    }

    for (int i = 0; i < count; i++) {
        if (addrs[i].ipv4[0] == 169 && addrs[i].ipv4[1] == 254) {
            char buf[20];
            fmt_ipv4(addrs[i].ipv4, buf, sizeof(buf));
            TEST_FAIL(name, "APIPA address returned: %s", buf);
            return;
        }
    }
    TEST_PASS(name, "");
}

static void test_no_zero_address(void)
{
    const char*                name = "no_zero_address";
    struct pubnub_ipv4_address addrs[8];
    int count = pubnub_dns_read_system_servers_ipv4(NULL, addrs, 8);

    if (count <= 0) {
        TEST_SKIP(name, "no DNS servers discovered");
        return;
    }

    for (int i = 0; i < count; i++) {
        if (addrs[i].ipv4[0] == 0) {
            char buf[20];
            fmt_ipv4(addrs[i].ipv4, buf, sizeof(buf));
            TEST_FAIL(name, "zero-prefix address returned at index %d: %s",
                      i, buf);
            return;
        }
    }
    TEST_PASS(name, "");
}

static void test_no_multicast_reserved(void)
{
    const char*                name = "no_multicast_reserved";
    struct pubnub_ipv4_address addrs[8];
    int count = pubnub_dns_read_system_servers_ipv4(NULL, addrs, 8);

    if (count <= 0) {
        TEST_SKIP(name, "no DNS servers discovered");
        return;
    }

    for (int i = 0; i < count; i++) {
        if (addrs[i].ipv4[0] >= 224) {
            char buf[20];
            fmt_ipv4(addrs[i].ipv4, buf, sizeof(buf));
            TEST_FAIL(
                name, "multicast/reserved/broadcast address returned: %s", buf);
            return;
        }
    }
    TEST_PASS(name, "");
}

static void test_no_duplicates(void)
{
    const char*                name = "no_duplicates";
    struct pubnub_ipv4_address addrs[8];
    int count = pubnub_dns_read_system_servers_ipv4(NULL, addrs, 8);

    if (count <= 1) {
        TEST_SKIP(name, "need >= 2 servers to check duplicates");
        return;
    }

    for (int i = 0; i < count; i++) {
        for (int j = i + 1; j < count; j++) {
            if (memcmp(addrs[i].ipv4, addrs[j].ipv4, 4) == 0) {
                char buf[20];
                fmt_ipv4(addrs[i].ipv4, buf, sizeof(buf));
                TEST_FAIL(name, "duplicate at [%d] and [%d]: %s", i, j, buf);
                return;
            }
        }
    }
    TEST_PASS(name, "");
}

static void test_software_loopback_filtered(void)
{
    const char* name = "software_loopback_filtered";

    /* Enumerate all adapters and collect DNS from IF_TYPE_SOFTWARE_LOOPBACK
       interfaces. Then verify none of those DNS addresses appear in our
       discovery results. This tests the IfType filter against the real
       Windows loopback pseudo-interface(s). */
    IP_ADAPTER_ADDRESSES* all_addrs = NULL;
    ULONG                 buflen = 15000;
    DWORD                 ret;
    ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST;

    all_addrs = (IP_ADAPTER_ADDRESSES*)malloc(buflen);
    if (!all_addrs) {
        TEST_SKIP(name, "malloc failed");
        return;
    }

    ret = GetAdaptersAddresses(AF_INET, flags, NULL, all_addrs, &buflen);
    if (ret == ERROR_BUFFER_OVERFLOW) {
        free(all_addrs);
        all_addrs = (IP_ADAPTER_ADDRESSES*)malloc(buflen);
        if (!all_addrs) { TEST_SKIP(name, "malloc failed"); return; }
        ret = GetAdaptersAddresses(AF_INET, flags, NULL, all_addrs, &buflen);
    }
    if (ret != NO_ERROR) {
        free(all_addrs);
        TEST_SKIP(name, "GetAdaptersAddresses failed: %lu", (unsigned long)ret);
        return;
    }

    /* Collect DNS addresses from software loopback interfaces. */
    uint8_t loopback_dns[8][4];
    int     loopback_dns_count = 0;
    bool    found_loopback_iface = false;

    for (IP_ADAPTER_ADDRESSES* aa = all_addrs; aa; aa = aa->Next) {
        if (aa->IfType != 24 /* IF_TYPE_SOFTWARE_LOOPBACK */) continue;
        found_loopback_iface = true;

        {
            char desc[256];
            WideCharToMultiByte(
                CP_UTF8, 0, aa->FriendlyName, -1, desc, sizeof(desc),
                NULL, NULL);
            printf("    Found loopback interface: %s (idx=%lu)\n",
                   desc, (unsigned long)aa->IfIndex);
        }

        for (IP_ADAPTER_DNS_SERVER_ADDRESS* ds = aa->FirstDnsServerAddress;
             ds && loopback_dns_count < 8;
             ds = ds->Next) {
            if (!ds->Address.lpSockaddr ||
                ds->Address.lpSockaddr->sa_family != AF_INET) continue;
            struct sockaddr_in* sin =
                (struct sockaddr_in*)ds->Address.lpSockaddr;
            memcpy(loopback_dns[loopback_dns_count],
                   &sin->sin_addr.s_addr, 4);
            char buf[20];
            fmt_ipv4(loopback_dns[loopback_dns_count], buf, sizeof(buf));
            printf("    Loopback DNS: %s\n", buf);
            loopback_dns_count++;
        }
    }
    free(all_addrs);

    if (!found_loopback_iface) {
        TEST_SKIP(name, "no IF_TYPE_SOFTWARE_LOOPBACK interface on system");
        return;
    }

    printf("    Loopback interfaces found, %d DNS server(s) on them\n",
           loopback_dns_count);

    if (loopback_dns_count == 0) {
        /* Loopback interface exists but has no DNS — the filter is still
           exercised (adapter rejected before DNS enumeration). Pass. */
        TEST_PASS(name, "(loopback interface present, no DNS to leak)");
        return;
    }

    /* Verify none of the loopback DNS addresses appear in results. */
    struct pubnub_ipv4_address result[8];
    int count = pubnub_dns_read_system_servers_ipv4(NULL, result, 8);
    if (count <= 0) {
        TEST_PASS(name, "(no DNS returned, nothing leaked)");
        return;
    }

    for (int i = 0; i < count; i++) {
        for (int j = 0; j < loopback_dns_count; j++) {
            if (memcmp(result[i].ipv4, loopback_dns[j], 4) == 0) {
                char buf[20];
                fmt_ipv4(result[i].ipv4, buf, sizeof(buf));
                TEST_FAIL(name,
                          "DNS %s from loopback interface leaked into results",
                          buf);
                return;
            }
        }
    }
    TEST_PASS(name, "(loopback DNS correctly filtered)");
}

static void test_addresses_are_printable(void)
{
    const char*                name = "addresses_printable";
    struct pubnub_ipv4_address addrs[8];
    int count = pubnub_dns_read_system_servers_ipv4(NULL, addrs, 8);

    if (count <= 0) {
        TEST_SKIP(name, "no DNS servers discovered");
        return;
    }

    printf("    Discovered DNS servers:\n");
    for (int i = 0; i < count; i++) {
        char buf[20];
        fmt_ipv4(addrs[i].ipv4, buf, sizeof(buf));
        printf("      [%d] %s\n", i, buf);
    }
    TEST_PASS(name, "(%d server(s) listed)", count);
}

static void run_baseline(void)
{
    printf("\n=== Phase 1: Baseline ===\n");
    test_basic_discovery();
    test_no_loopback_returned();
    test_no_apipa_returned();
    test_no_zero_address();
    test_no_multicast_reserved();
    test_no_duplicates();
    test_software_loopback_filtered();
    test_addresses_are_printable();
}


/* ------------------------------------------------------------------ */
/*        Phase 2: Virtual adapter visibility                          */
/* ------------------------------------------------------------------ */

static void test_virtual_adapter_dns_visible(void)
{
    const char* name = "virtual_adapter_dns_visible";

    struct pubnub_ipv4_address addrs[8];
    int count = pubnub_dns_read_system_servers_ipv4(NULL, addrs, 8);

    if (count <= 0) {
        TEST_FAIL(name, "expected DNS servers, got %d", count);
        return;
    }

    /* The Hyper-V adapter has Ethernet IfType with valid unicast IP and
       DNS 10.255.255.1. It passes adapter suitability, so the DNS
       SHOULD appear — proving our code sees virtual adapters. */
    for (int i = 0; i < count; i++) {
        if (addrs[i].ipv4[0] == 10 && addrs[i].ipv4[1] == 255 &&
            addrs[i].ipv4[2] == 255 && addrs[i].ipv4[3] == 1) {
            TEST_PASS(name, "(10.255.255.1 found at [%d])", i);
            return;
        }
    }
    TEST_FAIL(name, "test adapter DNS 10.255.255.1 not found in results");
}

static void run_loopback(void)
{
    printf("\n=== Phase 2: Virtual adapter visibility ===\n");
    if (!adapter_exists("vEthernet (PNTestSwitch)")) {
        TEST_SKIP("loopback_phase", "test adapter not installed");
        return;
    }
    test_virtual_adapter_dns_visible();
    test_basic_discovery();
}


/* ------------------------------------------------------------------ */
/*                 Phase 3: Disabled adapter                           */
/* ------------------------------------------------------------------ */

static void test_disabled_adapter_filtered(void)
{
    const char* name = "disabled_adapter_filtered";

    struct pubnub_ipv4_address addrs[8];
    int count = pubnub_dns_read_system_servers_ipv4(NULL, addrs, 8);

    if (count <= 0) {
        TEST_FAIL(name, "expected real DNS servers, got %d", count);
        return;
    }

    /* Disabled adapter's DNS (10.255.255.1) should not appear */
    for (int i = 0; i < count; i++) {
        if (addrs[i].ipv4[0] == 10 && addrs[i].ipv4[1] == 255 &&
            addrs[i].ipv4[2] == 255 && addrs[i].ipv4[3] == 1) {
            TEST_FAIL(name, "disabled adapter DNS 10.255.255.1 was returned");
            return;
        }
    }
    TEST_PASS(name, "");
}

static void run_disabled(void)
{
    printf("\n=== Phase 3: Disabled adapter ===\n");
    /* Disabled adapters are invisible to GetAdaptersAddresses, so
       adapter_exists() would always return false after disabling.
       We just verify the disabled adapter's DNS (10.255.255.1)
       does not leak into results. */
    test_disabled_adapter_filtered();
    test_basic_discovery();
}


/* ------------------------------------------------------------------ */
/*              Phase 5: Metric-based ordering                         */
/* ------------------------------------------------------------------ */

static void test_metric_ordering(void)
{
    const char* name = "metric_ordering";

    /* We can only verify that results come back and the best-route
       adapter is prioritized. We check that the first address is
       not the loopback adapter's fake DNS. */
    struct pubnub_ipv4_address addrs[8];
    int count = pubnub_dns_read_system_servers_ipv4(NULL, addrs, 8);

    if (count <= 0) {
        TEST_FAIL(name, "expected DNS servers, got %d", count);
        return;
    }

    /* First result should NOT be the high-metric loopback adapter */
    if (addrs[0].ipv4[0] == 10 && addrs[0].ipv4[1] == 255 &&
        addrs[0].ipv4[2] == 255 && addrs[0].ipv4[3] == 1) {
        TEST_FAIL(name, "high-metric adapter DNS came first");
        return;
    }

    char buf[20];
    fmt_ipv4(addrs[0].ipv4, buf, sizeof(buf));
    TEST_PASS(name, "(first DNS: %s)", buf);
}

static void run_metric(void)
{
    printf("\n=== Phase 5: Metric ordering ===\n");
    if (!adapter_exists("vEthernet (PNTestSwitch)")) {
        TEST_SKIP("metric_phase", "test adapter not installed");
        return;
    }
    test_metric_ordering();
}


/* ------------------------------------------------------------------ */
/*                Phase 6: IPv6 tests                                  */
/* ------------------------------------------------------------------ */

#if PUBNUB_USE_IPV6
static void test_ipv6_basic_discovery(void)
{
    const char*                name = "ipv6_basic_discovery";
    struct pubnub_ipv6_address addrs[8];
    int count = pubnub_dns_read_system_servers_ipv6(NULL, addrs, 8);

    if (count <= 0) {
        /* IPv6 DNS might genuinely not be available */
        TEST_SKIP(name, "no IPv6 DNS servers (may be expected)");
        return;
    }
    TEST_PASS(name, "(found %d server(s))", count);
}

static void test_ipv6_no_link_local(void)
{
    const char*                name = "ipv6_no_link_local";
    struct pubnub_ipv6_address addrs[8];
    int count = pubnub_dns_read_system_servers_ipv6(NULL, addrs, 8);

    if (count <= 0) {
        TEST_SKIP(name, "no IPv6 DNS servers");
        return;
    }

    for (int i = 0; i < count; i++) {
        if (addrs[i].ipv6[0] == 0xfe && (addrs[i].ipv6[1] & 0xc0) == 0x80) {
            char buf[INET6_ADDRSTRLEN];
            fmt_ipv6(addrs[i].ipv6, buf, sizeof(buf));
            TEST_FAIL(name, "link-local address returned: %s", buf);
            return;
        }
    }
    TEST_PASS(name, "");
}

static void test_ipv6_no_loopback(void)
{
    const char*                name = "ipv6_no_loopback";
    struct pubnub_ipv6_address addrs[8];
    int count = pubnub_dns_read_system_servers_ipv6(NULL, addrs, 8);

    if (count <= 0) {
        TEST_SKIP(name, "no IPv6 DNS servers");
        return;
    }

    for (int i = 0; i < count; i++) {
        /* ::1 = all zeros except last byte == 1 */
        bool is_loopback = (addrs[i].ipv6[15] == 1);
        for (int j = 0; j < 15 && is_loopback; j++) {
            if (addrs[i].ipv6[j] != 0) is_loopback = false;
        }
        if (is_loopback) {
            TEST_FAIL(name, "loopback ::1 returned at index %d", i);
            return;
        }
    }
    TEST_PASS(name, "");
}

static void test_ipv6_no_duplicates(void)
{
    const char*                name = "ipv6_no_duplicates";
    struct pubnub_ipv6_address addrs[8];
    int count = pubnub_dns_read_system_servers_ipv6(NULL, addrs, 8);

    if (count <= 1) {
        TEST_SKIP(name, "need >= 2 servers to check duplicates");
        return;
    }

    for (int i = 0; i < count; i++) {
        for (int j = i + 1; j < count; j++) {
            if (memcmp(addrs[i].ipv6, addrs[j].ipv6, 16) == 0) {
                char buf[INET6_ADDRSTRLEN];
                fmt_ipv6(addrs[i].ipv6, buf, sizeof(buf));
                TEST_FAIL(name, "duplicate at [%d] and [%d]: %s", i, j, buf);
                return;
            }
        }
    }
    TEST_PASS(name, "");
}

static void test_ipv6_addresses_printable(void)
{
    const char*                name = "ipv6_addresses_printable";
    struct pubnub_ipv6_address addrs[8];
    int count = pubnub_dns_read_system_servers_ipv6(NULL, addrs, 8);

    if (count <= 0) {
        TEST_SKIP(name, "no IPv6 DNS servers");
        return;
    }

    printf("    Discovered IPv6 DNS servers:\n");
    for (int i = 0; i < count; i++) {
        char buf[INET6_ADDRSTRLEN];
        fmt_ipv6(addrs[i].ipv6, buf, sizeof(buf));
        printf("      [%d] %s\n", i, buf);
    }
    TEST_PASS(name, "(%d server(s) listed)", count);
}

static void test_ipv6_no_all_zeros(void)
{
    const char*                name = "ipv6_no_all_zeros";
    struct pubnub_ipv6_address addrs[8];
    int count = pubnub_dns_read_system_servers_ipv6(NULL, addrs, 8);

    if (count <= 0) {
        TEST_SKIP(name, "no IPv6 DNS servers");
        return;
    }

    for (int i = 0; i < count; i++) {
        bool all_zero = true;
        for (int j = 0; j < 16 && all_zero; j++) {
            if (addrs[i].ipv6[j] != 0) all_zero = false;
        }
        if (all_zero) {
            TEST_FAIL(name, "all-zeros address (::) returned at index %d", i);
            return;
        }
    }
    TEST_PASS(name, "");
}

static void test_ipv6_injected_found(void)
{
    const char*                name = "ipv6_injected_found";
    struct pubnub_ipv6_address addrs[8];
    int count = pubnub_dns_read_system_servers_ipv6(NULL, addrs, 8);

    if (count <= 0) {
        TEST_SKIP(name, "no IPv6 DNS servers discovered");
        return;
    }

    /* Look for fd00::53 (injected by IPv6 setup scenario).
       fd00::53 = { 0xfd, 0x00, 0,...0, 0x00, 0x53 } */
    uint8_t expected[16] = { 0xfd, 0 };
    expected[15] = 0x53;

    for (int i = 0; i < count; i++) {
        if (memcmp(addrs[i].ipv6, expected, 16) == 0) {
            TEST_PASS(name, "(fd00::53 found at [%d])", i);
            return;
        }
    }
    /* fd00::53 might not be present if IPv6 setup was not run */
    TEST_SKIP(name, "fd00::53 not found (IPv6 setup may not have run)");
}
#endif /* PUBNUB_USE_IPV6 */

static void run_ipv6(void)
{
    printf("\n=== Phase 6: IPv6 ===\n");
#if PUBNUB_USE_IPV6
    test_ipv6_basic_discovery();
    test_ipv6_no_link_local();
    test_ipv6_no_loopback();
    test_ipv6_no_all_zeros();
    test_ipv6_no_duplicates();
    test_ipv6_addresses_printable();
    test_ipv6_injected_found();
#else
    TEST_SKIP("ipv6_phase", "PUBNUB_USE_IPV6 not enabled");
#endif
}


/* ------------------------------------------------------------------ */
/*                Phase 7: Concurrency stress test                     */
/* ------------------------------------------------------------------ */

#define CONCURRENT_THREADS 8
#define CONCURRENT_ITERATIONS 100

struct thread_result {
    int  iterations_ok;
    int  iterations_fail;
    bool crashed;
};

static unsigned __stdcall concurrent_worker(void* arg)
{
    struct thread_result* result = (struct thread_result*)arg;
    result->iterations_ok        = 0;
    result->iterations_fail      = 0;
    result->crashed              = false;

    for (int i = 0; i < CONCURRENT_ITERATIONS; i++) {
        struct pubnub_ipv4_address addrs[4];
        int count = pubnub_dns_read_system_servers_ipv4(NULL, addrs, 4);
        if (count > 0) { result->iterations_ok++; }
        else {
            result->iterations_fail++;
        }
    }

    return 0;
}

static void test_concurrent_discovery(void)
{
    const char* name = "concurrent_discovery";

    HANDLE               threads[CONCURRENT_THREADS];
    struct thread_result results[CONCURRENT_THREADS];

    printf(
        "    Spawning %d threads x %d iterations...\n",
        CONCURRENT_THREADS,
        CONCURRENT_ITERATIONS);

    for (int i = 0; i < CONCURRENT_THREADS; i++) {
        threads[i] = (HANDLE)_beginthreadex(
            NULL, 0, concurrent_worker, &results[i], 0, NULL);
        if (!threads[i]) {
            TEST_FAIL(name, "failed to create thread %d", i);
            return;
        }
    }

    DWORD wait =
        WaitForMultipleObjects(CONCURRENT_THREADS, threads, TRUE, 60000);

    if (wait == WAIT_TIMEOUT) {
        TEST_FAIL(name, "threads did not finish within 60s");
        return;
    }

    int total_ok   = 0;
    int total_fail = 0;
    for (int i = 0; i < CONCURRENT_THREADS; i++) {
        total_ok += results[i].iterations_ok;
        total_fail += results[i].iterations_fail;
        CloseHandle(threads[i]);
    }

    printf(
        "    Total: %d OK, %d fail out of %d\n",
        total_ok,
        total_fail,
        CONCURRENT_THREADS * CONCURRENT_ITERATIONS);

    if (total_fail > 0) { TEST_FAIL(name, "%d iterations failed", total_fail); }
    else {
        TEST_PASS(name, "(%d iterations, no crashes)", total_ok);
    }
}

static void run_concurrent(void)
{
    printf("\n=== Phase 7: Concurrency stress test ===\n");
    test_concurrent_discovery();
}


/* ------------------------------------------------------------------ */
/*             Phase 8: Buffer edge cases                              */
/* ------------------------------------------------------------------ */

static void test_buffer_n_equals_1(void)
{
    const char*                name = "buffer_n_equals_1";
    struct pubnub_ipv4_address addrs[1];
    int count = pubnub_dns_read_system_servers_ipv4(NULL, addrs, 1);

    if (count <= 0) {
        TEST_FAIL(name, "expected 1 DNS server, got %d", count);
        return;
    }
    if (count > 1) {
        TEST_FAIL(
            name, "requested n=1 but got %d servers (buffer overrun?)", count);
        return;
    }

    char buf[20];
    fmt_ipv4(addrs[0].ipv4, buf, sizeof(buf));
    TEST_PASS(name, "(got %s)", buf);
}

static void test_buffer_over_request(void)
{
    const char* name = "buffer_over_request";
    /* Request more servers than likely exist. Should return actual count,
       not garbage-fill the rest. */
    struct pubnub_ipv4_address addrs[32];
    memset(addrs, 0xBB, sizeof(addrs));
    int count = pubnub_dns_read_system_servers_ipv4(NULL, addrs, 32);

    if (count <= 0) {
        TEST_FAIL(name, "expected >0 DNS servers, got %d", count);
        return;
    }

    /* Verify entries beyond count were not written. */
    if (count < 32) {
        uint8_t canary[4] = { 0xBB, 0xBB, 0xBB, 0xBB };
        if (memcmp(addrs[count].ipv4, canary, 4) != 0) {
            char buf[20];
            fmt_ipv4(addrs[count].ipv4, buf, sizeof(buf));
            TEST_FAIL(
                name, "entry [%d] was written beyond count: %s", count, buf);
            return;
        }
    }

    TEST_PASS(name, "(requested 32, got %d, no overrun)", count);
}

static void test_buffer_n1_matches_first_of_n8(void)
{
    const char* name = "buffer_n1_consistent";
    /* The server returned for n=1 should be the same as the first server
       returned for n=8 (both use the same priority logic). */
    struct pubnub_ipv4_address one[1];
    struct pubnub_ipv4_address eight[8];
    int c1 = pubnub_dns_read_system_servers_ipv4(NULL, one, 1);
    int c8 = pubnub_dns_read_system_servers_ipv4(NULL, eight, 8);

    if (c1 <= 0 || c8 <= 0) {
        TEST_SKIP(name, "need DNS for both calls (c1=%d, c8=%d)", c1, c8);
        return;
    }

    if (memcmp(one[0].ipv4, eight[0].ipv4, 4) != 0) {
        char b1[20], b8[20];
        fmt_ipv4(one[0].ipv4, b1, sizeof(b1));
        fmt_ipv4(eight[0].ipv4, b8, sizeof(b8));
        TEST_FAIL(name, "n=1 returned %s but n=8 first is %s", b1, b8);
        return;
    }
    TEST_PASS(name, "");
}

static void run_buffer_edge(void)
{
    printf("\n=== Phase 8: Buffer edge cases ===\n");
    test_buffer_n_equals_1();
    test_buffer_over_request();
    test_buffer_n1_matches_first_of_n8();
}


/* ------------------------------------------------------------------ */
/*             Phase 9: Result stability                               */
/* ------------------------------------------------------------------ */

#define STABILITY_ITERATIONS 50

static void test_result_stability(void)
{
    const char*                name = "result_stability";
    struct pubnub_ipv4_address ref[8];
    int ref_count = pubnub_dns_read_system_servers_ipv4(NULL, ref, 8);

    if (ref_count <= 0) {
        TEST_FAIL(name, "initial call failed (count=%d)", ref_count);
        return;
    }

    printf(
        "    Running %d iterations for stability...\n", STABILITY_ITERATIONS);

    for (int iter = 0; iter < STABILITY_ITERATIONS; iter++) {
        struct pubnub_ipv4_address cur[8];
        int cur_count = pubnub_dns_read_system_servers_ipv4(NULL, cur, 8);

        if (cur_count != ref_count) {
            TEST_FAIL(
                name,
                "iteration %d: count changed from %d to %d",
                iter,
                ref_count,
                cur_count);
            return;
        }

        for (int i = 0; i < cur_count; i++) {
            if (memcmp(ref[i].ipv4, cur[i].ipv4, 4) != 0) {
                char r[20], c[20];
                fmt_ipv4(ref[i].ipv4, r, sizeof(r));
                fmt_ipv4(cur[i].ipv4, c, sizeof(c));
                TEST_FAIL(
                    name,
                    "iteration %d: server [%d] changed from %s to %s",
                    iter,
                    i,
                    r,
                    c);
                return;
            }
        }
    }

    TEST_PASS(
        name,
        "(%d iterations, count=%d, stable)",
        STABILITY_ITERATIONS,
        ref_count);
}

static void run_stability(void)
{
    printf("\n=== Phase 9: Result stability ===\n");
    test_result_stability();
}


/* ------------------------------------------------------------------ */
/*        Phase 10: Broadcast/multicast DNS filtering                  */
/* ------------------------------------------------------------------ */

static void test_broadcast_dns_filtered(void)
{
    const char* name = "broadcast_dns_filtered";

    struct pubnub_ipv4_address addrs[8];
    int count = pubnub_dns_read_system_servers_ipv4(NULL, addrs, 8);

    if (count <= 0) {
        /* With a broadcast-only DNS adapter, real adapters should still
           provide DNS. If not, the runner has no network. */
        TEST_FAIL(name, "expected real DNS servers, got %d", count);
        return;
    }

    /* Verify 255.255.255.255 is NOT in results. */
    for (int i = 0; i < count; i++) {
        if (addrs[i].ipv4[0] == 255 && addrs[i].ipv4[1] == 255 &&
            addrs[i].ipv4[2] == 255 && addrs[i].ipv4[3] == 255) {
            TEST_FAIL(name, "broadcast address 255.255.255.255 was returned");
            return;
        }
    }
    TEST_PASS(name, "");
}

static void test_multicast_dns_filtered(void)
{
    const char* name = "multicast_dns_filtered";

    struct pubnub_ipv4_address addrs[8];
    int count = pubnub_dns_read_system_servers_ipv4(NULL, addrs, 8);

    if (count <= 0) {
        TEST_FAIL(name, "expected real DNS servers, got %d", count);
        return;
    }

    /* Verify 239.255.255.250 (or any 224-239 range) is NOT in results. */
    for (int i = 0; i < count; i++) {
        if (addrs[i].ipv4[0] >= 224 && addrs[i].ipv4[0] <= 239) {
            char buf[20];
            fmt_ipv4(addrs[i].ipv4, buf, sizeof(buf));
            TEST_FAIL(name, "multicast address returned: %s", buf);
            return;
        }
    }
    TEST_PASS(name, "");
}

static void test_reserved_dns_filtered(void)
{
    const char* name = "reserved_dns_filtered";

    struct pubnub_ipv4_address addrs[8];
    int count = pubnub_dns_read_system_servers_ipv4(NULL, addrs, 8);

    if (count <= 0) {
        TEST_FAIL(name, "expected real DNS servers, got %d", count);
        return;
    }

    /* Verify 240.0.0.1 (or any 240-254 range) is NOT in results. */
    for (int i = 0; i < count; i++) {
        if (addrs[i].ipv4[0] >= 240) {
            char buf[20];
            fmt_ipv4(addrs[i].ipv4, buf, sizeof(buf));
            TEST_FAIL(name, "reserved/broadcast address returned: %s", buf);
            return;
        }
    }
    TEST_PASS(name, "");
}

static void run_broadcast(void)
{
    printf("\n=== Phase 10: Broadcast/multicast DNS filtering ===\n");
    test_broadcast_dns_filtered();
    test_multicast_dns_filtered();
    test_reserved_dns_filtered();
    test_basic_discovery();
}


/* ------------------------------------------------------------------ */
/*        Phase 11: No-DNS adapter                                     */
/* ------------------------------------------------------------------ */

static void test_no_dns_adapter_handled(void)
{
    const char* name = "no_dns_adapter_handled";

    struct pubnub_ipv4_address addrs[8];
    int count = pubnub_dns_read_system_servers_ipv4(NULL, addrs, 8);

    /* Should still return real DNS from other adapters. */
    if (count <= 0) {
        TEST_FAIL(
            name, "expected real DNS despite no-DNS adapter, got %d", count);
        return;
    }

    /* Verify no garbage (all returned addresses should be valid). */
    for (int i = 0; i < count; i++) {
        if (addrs[i].ipv4[0] == 0 || addrs[i].ipv4[0] == 127 ||
            (addrs[i].ipv4[0] == 169 && addrs[i].ipv4[1] == 254) ||
            addrs[i].ipv4[0] >= 224) {
            char buf[20];
            fmt_ipv4(addrs[i].ipv4, buf, sizeof(buf));
            TEST_FAIL(name, "invalid address returned: %s", buf);
            return;
        }
    }
    TEST_PASS(name, "(got %d valid server(s))", count);
}

static void run_no_dns(void)
{
    printf("\n=== Phase 11: No-DNS adapter ===\n");
    if (!adapter_exists("vEthernet (PNTestSwitch)")) {
        TEST_SKIP("no_dns_phase", "test adapter not installed");
        return;
    }
    test_no_dns_adapter_handled();
    test_basic_discovery();
}


/* ------------------------------------------------------------------ */
/*        Phase 12: Adapter flapping stress test                       */
/* ------------------------------------------------------------------ */

#define FLAP_THREADS 4
#define FLAP_ITERATIONS 50
#define FLAP_TOGGLE_MS 100

struct flap_result {
    int ok;
    int fail;
};

static unsigned __stdcall flap_reader(void* arg)
{
    struct flap_result* result = (struct flap_result*)arg;
    result->ok                 = 0;
    result->fail               = 0;

    for (int i = 0; i < FLAP_ITERATIONS; i++) {
        struct pubnub_ipv4_address addrs[4];
        int count = pubnub_dns_read_system_servers_ipv4(NULL, addrs, 4);

        /* During flapping, -1 is acceptable (no suitable adapters
           momentarily). What matters is no crash / access violation. */
        if (count > 0) result->ok++;
        else result->fail++;

        Sleep(10);
    }
    return 0;
}

static unsigned __stdcall flap_toggler(void* arg)
{
    (void)arg;
    /* Toggle loopback adapter state rapidly. */
    for (int i = 0; i < FLAP_ITERATIONS / 2; i++) {
        system(
            "powershell -Command \"Disable-NetAdapter -Name 'vEthernet (PNTestSwitch)'"
            " -Confirm:$false -ErrorAction SilentlyContinue\" >nul 2>&1");
        Sleep(FLAP_TOGGLE_MS);
        system(
            "powershell -Command \"Enable-NetAdapter -Name 'vEthernet (PNTestSwitch)'"
            " -Confirm:$false -ErrorAction SilentlyContinue\" >nul 2>&1");
        Sleep(FLAP_TOGGLE_MS);
    }
    return 0;
}

static void test_flapping_no_crash(void)
{
    const char* name = "flapping_no_crash";

    HANDLE toggle_thread =
        (HANDLE)_beginthreadex(NULL, 0, flap_toggler, NULL, 0, NULL);
    if (!toggle_thread) {
        TEST_FAIL(name, "failed to create toggle thread");
        return;
    }

    HANDLE             readers[FLAP_THREADS];
    struct flap_result results[FLAP_THREADS];

    printf(
        "    Flapping loopback + %d reader threads x %d iterations...\n",
        FLAP_THREADS,
        FLAP_ITERATIONS);

    for (int i = 0; i < FLAP_THREADS; i++) {
        readers[i] =
            (HANDLE)_beginthreadex(NULL, 0, flap_reader, &results[i], 0, NULL);
        if (!readers[i]) {
            TEST_FAIL(name, "failed to create reader thread %d", i);
            WaitForSingleObject(toggle_thread, 30000);
            CloseHandle(toggle_thread);
            return;
        }
    }

    /* Wait for all threads. */
    HANDLE all[FLAP_THREADS + 1];
    all[0] = toggle_thread;
    for (int i = 0; i < FLAP_THREADS; i++)
        all[i + 1] = readers[i];

    DWORD wait = WaitForMultipleObjects(FLAP_THREADS + 1, all, TRUE, 120000);

    if (wait == WAIT_TIMEOUT) {
        TEST_FAIL(name, "threads did not finish within 120s");
        /* Clean up what we can. */
        for (int i = 0; i <= FLAP_THREADS; i++)
            CloseHandle(all[i]);
        return;
    }

    int total_ok = 0, total_fail = 0;
    for (int i = 0; i < FLAP_THREADS; i++) {
        total_ok += results[i].ok;
        total_fail += results[i].fail;
        CloseHandle(readers[i]);
    }
    CloseHandle(toggle_thread);

    printf(
        "    Flap results: %d ok, %d transient-fail (expected), "
        "no crashes\n",
        total_ok,
        total_fail);

    /* The test passes as long as we didn't crash. Transient failures
       during adapter state changes are expected and acceptable. */
    TEST_PASS(name, "(%d ok, %d transient-fail)", total_ok, total_fail);
}

static void run_flapping(void)
{
    printf("\n=== Phase 12: Adapter flapping stress test ===\n");
    if (!adapter_exists("vEthernet (PNTestSwitch)")) {
        TEST_SKIP("flapping_phase", "test adapter not installed");
        return;
    }
    test_flapping_no_crash();
}


/* ------------------------------------------------------------------ */
/*        Phase 13: Multiple DNS servers per adapter                   */
/* ------------------------------------------------------------------ */

static void test_multi_dns_both_visible(void)
{
    const char* name = "multi_dns_both_visible";
    /* The Hyper-V adapter has DNS 10.255.255.1 and 10.255.255.2.
       Both should be returned since it's an Ethernet-type adapter. */
    struct pubnub_ipv4_address addrs[8];
    int count = pubnub_dns_read_system_servers_ipv4(NULL, addrs, 8);

    if (count <= 0) {
        TEST_FAIL(name, "expected DNS servers, got %d", count);
        return;
    }

    bool found1 = false, found2 = false;
    for (int i = 0; i < count; i++) {
        if (addrs[i].ipv4[0] == 10 && addrs[i].ipv4[1] == 255 &&
            addrs[i].ipv4[2] == 255) {
            if (addrs[i].ipv4[3] == 1) found1 = true;
            if (addrs[i].ipv4[3] == 2) found2 = true;
        }
    }

    if (!found1 || !found2) {
        TEST_FAIL(name, "expected both 10.255.255.1 and .2 (got .1=%d .2=%d)",
                  found1, found2);
        return;
    }
    TEST_PASS(name, "(both 10.255.255.1 and 10.255.255.2 found)");
}

static void run_multi_dns(void)
{
    printf("\n=== Phase 13: Multiple DNS per adapter ===\n");
    if (!adapter_exists("vEthernet (PNTestSwitch)")) {
        TEST_SKIP("multi_dns_phase", "test adapter not installed");
        return;
    }
    test_multi_dns_both_visible();
    test_no_duplicates();
    test_basic_discovery();
}


/* ------------------------------------------------------------------ */
/*        Phase 14: Boundary input tests                               */
/* ------------------------------------------------------------------ */

static void test_n_zero_returns_error(void)
{
    const char*                name = "n_zero_returns_error";
    struct pubnub_ipv4_address addrs[1];

    /* Suppress assert abort for intentional invalid input */
    pubnub_assert_set_handler(pubnub_assert_handler_printf);
    int count = pubnub_dns_read_system_servers_ipv4(NULL, addrs, 0);
    pubnub_assert_set_handler(NULL);

    if (count == -1) {
        TEST_PASS(name, "(returned -1 as expected)");
    }
    else {
        TEST_FAIL(name, "expected -1 for n=0, got %d", count);
    }
}

static void test_null_output_returns_error(void)
{
    const char* name = "null_output_returns_error";

    pubnub_assert_set_handler(pubnub_assert_handler_printf);
    int count = pubnub_dns_read_system_servers_ipv4(NULL, NULL, 8);
    pubnub_assert_set_handler(NULL);

    if (count == -1) {
        TEST_PASS(name, "(returned -1 as expected)");
    }
    else {
        TEST_FAIL(name, "expected -1 for NULL output, got %d", count);
    }
}

#if PUBNUB_USE_IPV6
static void test_ipv6_n_zero_returns_error(void)
{
    const char*                name = "ipv6_n_zero_returns_error";
    struct pubnub_ipv6_address addrs[1];

    pubnub_assert_set_handler(pubnub_assert_handler_printf);
    int count = pubnub_dns_read_system_servers_ipv6(NULL, addrs, 0);
    pubnub_assert_set_handler(NULL);

    if (count == -1) {
        TEST_PASS(name, "(returned -1 as expected)");
    }
    else {
        TEST_FAIL(name, "expected -1 for n=0, got %d", count);
    }
}

static void test_ipv6_null_output_returns_error(void)
{
    const char* name = "ipv6_null_output_returns_error";

    pubnub_assert_set_handler(pubnub_assert_handler_printf);
    int count = pubnub_dns_read_system_servers_ipv6(NULL, NULL, 8);
    pubnub_assert_set_handler(NULL);

    if (count == -1) {
        TEST_PASS(name, "(returned -1 as expected)");
    }
    else {
        TEST_FAIL(name, "expected -1 for NULL output, got %d", count);
    }
}
#endif

static void run_boundary(void)
{
    printf("\n=== Phase 14: Boundary input tests ===\n");
    test_n_zero_returns_error();
    test_null_output_returns_error();
#if PUBNUB_USE_IPV6
    test_ipv6_n_zero_returns_error();
    test_ipv6_null_output_returns_error();
#endif
}


/* ------------------------------------------------------------------ */
/*        Phase 15: Cross-adapter deduplication                        */
/* ------------------------------------------------------------------ */

static void test_dedup_across_adapters(void)
{
    const char*                name = "dedup_across_adapters";
    struct pubnub_ipv4_address addrs[16];
    int count = pubnub_dns_read_system_servers_ipv4(NULL, addrs, 16);

    if (count <= 1) {
        TEST_SKIP(name, "need >= 2 servers to verify dedup");
        return;
    }

    for (int i = 0; i < count; i++) {
        for (int j = i + 1; j < count; j++) {
            if (memcmp(addrs[i].ipv4, addrs[j].ipv4, 4) == 0) {
                char buf[20];
                fmt_ipv4(addrs[i].ipv4, buf, sizeof(buf));
                TEST_FAIL(name, "duplicate at [%d] and [%d]: %s", i, j, buf);
                return;
            }
        }
    }
    TEST_PASS(name, "(no duplicates among %d servers from multiple adapters)",
              count);
}

static void run_dedup(void)
{
    printf("\n=== Phase 15: Cross-adapter deduplication ===\n");
    if (!adapter_exists("vEthernet (PNTestSwitch)")) {
        TEST_SKIP("dedup_phase", "test adapter not installed");
        return;
    }
    test_dedup_across_adapters();
    test_basic_discovery();
}


/* ------------------------------------------------------------------ */
/*                              Main                                   */
/* ------------------------------------------------------------------ */

static void print_usage(void)
{
    printf("Usage: dns_discovery_test.exe <scenario>\n");
    printf("Scenarios:\n");
    printf("  baseline   - Phase 1: basic discovery validation\n");
    printf("  loopback   - Phase 2: loopback adapter filtering\n");
    printf("  disabled   - Phase 3: disabled adapter filtering\n");
    printf("  metric     - Phase 5: metric-based ordering\n");
    printf("  ipv6       - Phase 6: IPv6 tests\n");
    printf("  concurrent - Phase 7: concurrency stress test\n");
    printf("  buffer     - Phase 8: buffer edge cases\n");
    printf("  stability  - Phase 9: result stability\n");
    printf("  broadcast  - Phase 10: broadcast/multicast filtering\n");
    printf("  no_dns     - Phase 11: no-DNS adapter handling\n");
    printf("  flapping   - Phase 12: adapter flapping stress test\n");
    printf("  multi_dns  - Phase 13: multiple DNS per adapter\n");
    printf("  boundary   - Phase 14: boundary input tests\n");
    printf("  dedup      - Phase 15: cross-adapter deduplication\n");
    printf("  all        - Run all phases\n");
}

int main(int argc, char* argv[])
{
    WSADATA wsa;

    /* Disable stdout buffering so output is visible even on crash. */
    setvbuf(stdout, NULL, _IONBF, 0);

    if (argc < 2) {
        print_usage();
        return 1;
    }

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("[FATAL] WSAStartup failed: %d\n", WSAGetLastError());
        return 1;
    }

    const char* scenario = argv[1];

    printf("DNS Discovery Test Harness\n");
    printf("Scenario: %s\n", scenario);

    if (strcmp(scenario, "baseline") == 0 || strcmp(scenario, "all") == 0) {
        run_baseline();
    }
    if (strcmp(scenario, "loopback") == 0 || strcmp(scenario, "all") == 0) {
        run_loopback();
    }
    if (strcmp(scenario, "disabled") == 0 || strcmp(scenario, "all") == 0) {
        run_disabled();
    }
    if (strcmp(scenario, "metric") == 0 || strcmp(scenario, "all") == 0) {
        run_metric();
    }
    if (strcmp(scenario, "ipv6") == 0 || strcmp(scenario, "all") == 0) {
        run_ipv6();
    }
    if (strcmp(scenario, "concurrent") == 0 || strcmp(scenario, "all") == 0) {
        run_concurrent();
    }
    if (strcmp(scenario, "buffer") == 0 || strcmp(scenario, "all") == 0) {
        run_buffer_edge();
    }
    if (strcmp(scenario, "stability") == 0 || strcmp(scenario, "all") == 0) {
        run_stability();
    }
    if (strcmp(scenario, "broadcast") == 0 || strcmp(scenario, "all") == 0) {
        run_broadcast();
    }
    if (strcmp(scenario, "no_dns") == 0 || strcmp(scenario, "all") == 0) {
        run_no_dns();
    }
    if (strcmp(scenario, "flapping") == 0 || strcmp(scenario, "all") == 0) {
        run_flapping();
    }
    if (strcmp(scenario, "multi_dns") == 0 || strcmp(scenario, "all") == 0) {
        run_multi_dns();
    }
    if (strcmp(scenario, "boundary") == 0 || strcmp(scenario, "all") == 0) {
        run_boundary();
    }
    if (strcmp(scenario, "dedup") == 0 || strcmp(scenario, "all") == 0) {
        run_dedup();
    }

    WSACleanup();

    printf("\n========================================\n");
    printf(
        "Results: %d passed, %d failed, %d skipped\n",
        g_passes,
        g_fails,
        g_skipped);
    printf("========================================\n");

    return g_fails;
}

#else /* !_WIN32 */

#include <stdio.h>
int main(void)
{
    printf("[SKIP] DNS discovery tests are Windows-only.\n");
    return 0;
}

#endif /* _WIN32 */
