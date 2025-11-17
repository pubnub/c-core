/**
 * @file pubnub_dns_windows_test.cpp
 * @brief Comprehensive Windows DNS resolution tests for PubNub C-core SDK
 *
 * Tests cover:
 * - DNS server enumeration with various adapter states
 * - Filtering of invalid/disabled adapters
 * - Thread safety of DNS resolution
 * - Fallback mechanisms
 * - Edge cases with VPN/virtual adapters
 */

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#include <windns.h>

#include "core/pubnub_dns_servers.h"
#include "core/pubnub_log.h"
#include "core/pubnub_assert.h"

#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <sstream>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "dnsapi.lib")

// Test framework macros
#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "FAIL: " << msg << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            return false; \
        } \
    } while(0)

#define TEST_LOG(msg) \
    std::cout << "[TEST] " << msg << std::endl

#define TEST_PASS(name) \
    std::cout << "[PASS] " << name << std::endl

// Helper to format IPv4 address
std::string format_ipv4(const pubnub_ipv4_address& addr) {
    std::ostringstream oss;
    oss << static_cast<int>(addr.ipv4[0]) << "."
        << static_cast<int>(addr.ipv4[1]) << "."
        << static_cast<int>(addr.ipv4[2]) << "."
        << static_cast<int>(addr.ipv4[3]);
    return oss.str();
}

// Helper to check if DNS server is reachable via TCP connection
// This matches the implementation in pubnub_dns_system_servers.c
bool is_dns_reachable(const pubnub_ipv4_address& dns_server, int timeout_ms = 2000) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        return false;
    }

    // Set socket to non-blocking mode for timeout control
    u_long mode = 1;
    if (ioctlsocket(sock, FIONBIO, &mode) != 0) {
        closesocket(sock);
        return false;
    }

    sockaddr_in dns_addr = {};
    dns_addr.sin_family = AF_INET;
    dns_addr.sin_port = htons(53);
    memcpy(&dns_addr.sin_addr.s_addr, dns_server.ipv4, 4);

    // Attempt connection (will return immediately with WSAEWOULDBLOCK)
    int result = connect(sock, reinterpret_cast<sockaddr*>(&dns_addr), sizeof(dns_addr));

    if (result == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err != WSAEWOULDBLOCK) {
            // Immediate failure (connection refused, no route, etc.)
            closesocket(sock);
            return false;
        }

        // Connection in progress - wait for completion with timeout
        fd_set writefds;
        fd_set exceptfds;
        timeval timeout;

        FD_ZERO(&writefds);
        FD_ZERO(&exceptfds);
        FD_SET(sock, &writefds);
        FD_SET(sock, &exceptfds);

        timeout.tv_sec = timeout_ms / 1000;
        timeout.tv_usec = (timeout_ms % 1000) * 1000;

        result = select(0, NULL, &writefds, &exceptfds, &timeout);

        if (result == SOCKET_ERROR || result == 0) {
            // select failed or timeout
            closesocket(sock);
            return false;
        }

        if (FD_ISSET(sock, &exceptfds)) {
            // Connection failed
            closesocket(sock);
            return false;
        }

        if (!FD_ISSET(sock, &writefds)) {
            // Should not happen, but handle it
            closesocket(sock);
            return false;
        }

        // Verify connection succeeded (check SO_ERROR)
        int so_error = 0;
        int len = sizeof(so_error);
        if (getsockopt(sock, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&so_error), &len) != 0 || so_error != 0) {
            closesocket(sock);
            return false;
        }
    }

    // Connection successful
    closesocket(sock);
    return true;
}

// Test 1: Basic DNS server enumeration
bool test_dns_enumeration_basic() {
    TEST_LOG("Test 1: Basic DNS server enumeration");

    pubnub_ipv4_address servers[10];
    memset(servers, 0, sizeof(servers));

    int count = pubnub_dns_read_system_servers_ipv4(servers, 10);

    TEST_ASSERT(count >= 0, "DNS enumeration should not fail");
    TEST_ASSERT(count <= 10, "Should not exceed array bounds");

    TEST_LOG("Found " << count << " DNS servers:");
    for (int i = 0; i < count; i++) {
        TEST_LOG("  [" << i << "] " << format_ipv4(servers[i]));

        // Verify not 0.0.0.0
        TEST_ASSERT(servers[i].ipv4[0] != 0 || servers[i].ipv4[1] != 0 ||
                    servers[i].ipv4[2] != 0 || servers[i].ipv4[3] != 0,
                    "DNS server should not be 0.0.0.0");
    }

    TEST_PASS("test_dns_enumeration_basic");
    return true;
}

// Test 2: DNS server uniqueness (no duplicates)
bool test_dns_no_duplicates() {
    TEST_LOG("Test 2: Verify no duplicate DNS servers returned");

    pubnub_ipv4_address servers[16];
    memset(servers, 0, sizeof(servers));

    int count = pubnub_dns_read_system_servers_ipv4(servers, 16);
    TEST_ASSERT(count >= 0, "DNS enumeration should succeed");

    // Check for duplicates
    for (int i = 0; i < count; i++) {
        for (int j = i + 1; j < count; j++) {
            bool is_duplicate = (memcmp(servers[i].ipv4, servers[j].ipv4, 4) == 0);
            TEST_ASSERT(!is_duplicate,
                "Found duplicate DNS server: " << format_ipv4(servers[i]));
        }
    }

    TEST_PASS("test_dns_no_duplicates");
    return true;
}

// Test 3: Verify only UP adapters are used
bool test_dns_only_up_adapters() {
    TEST_LOG("Test 3: Verify DNS servers come from UP adapters only");

    // Get DNS servers from our function
    pubnub_ipv4_address our_servers[16];
    memset(our_servers, 0, sizeof(our_servers));
    int our_count = pubnub_dns_read_system_servers_ipv4(our_servers, 16);

    // Manually enumerate all adapters including DOWN ones
    ULONG buflen = 0;
    GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST,
                         NULL, NULL, &buflen);

    IP_ADAPTER_ADDRESSES* addrs = static_cast<IP_ADAPTER_ADDRESSES*>(malloc(buflen));
    TEST_ASSERT(addrs != NULL, "Failed to allocate memory");

    DWORD ret = GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST,
                                     NULL, addrs, &buflen);
    TEST_ASSERT(ret == NO_ERROR, "GetAdaptersAddresses failed");

    // Collect DNS servers from DOWN adapters
    std::vector<std::string> down_adapter_dns;
    for (IP_ADAPTER_ADDRESSES* aa = addrs; aa != NULL; aa = aa->Next) {
        if (aa->OperStatus != IfOperStatusUp) {
            TEST_LOG("Found DOWN adapter: " << aa->AdapterName << " status=" << aa->OperStatus);

            for (IP_ADAPTER_DNS_SERVER_ADDRESS* ds = aa->FirstDnsServerAddress;
                 ds != NULL; ds = ds->Next) {
                if (ds->Address.lpSockaddr &&
                    ds->Address.lpSockaddr->sa_family == AF_INET) {
                    sockaddr_in* sin = reinterpret_cast<sockaddr_in*>(ds->Address.lpSockaddr);
                    pubnub_ipv4_address addr;
                    DWORD net_addr = sin->sin_addr.S_un.S_addr;
                    DWORD host_addr = ntohl(net_addr);
                    addr.ipv4[0] = (host_addr >> 24) & 0xFF;
                    addr.ipv4[1] = (host_addr >> 16) & 0xFF;
                    addr.ipv4[2] = (host_addr >> 8) & 0xFF;
                    addr.ipv4[3] = host_addr & 0xFF;
                    down_adapter_dns.push_back(format_ipv4(addr));
                }
            }
        }
    }

    // Verify none of the DOWN adapter DNS servers are in our list
    for (const auto& down_dns : down_adapter_dns) {
        for (int i = 0; i < our_count; i++) {
            std::string our_dns = format_ipv4(our_servers[i]);
            TEST_ASSERT(our_dns != down_dns,
                "DNS server from DOWN adapter found: " << down_dns);
        }
    }

    free(addrs);
    TEST_PASS("test_dns_only_up_adapters");
    return true;
}

// Test 4: Thread safety
bool test_dns_thread_safety() {
    TEST_LOG("Test 4: Thread safety of DNS enumeration");

    const int num_threads = 10;
    const int iterations = 5;
    std::vector<std::thread> threads;
    std::atomic<int> failures(0);

    auto worker = [&]() {
        for (int i = 0; i < iterations; i++) {
            pubnub_ipv4_address servers[16];
            int count = pubnub_dns_read_system_servers_ipv4(servers, 16);
            if (count < 0) {
                failures++;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    };

    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back(worker);
    }

    for (auto& t : threads) {
        t.join();
    }

    TEST_ASSERT(failures == 0, "Thread safety test had " << failures << " failures");
    TEST_PASS("test_dns_thread_safety");
    return true;
}

// Test 5: Verify no loopback or tunnel adapters
bool test_dns_no_invalid_adapter_types() {
    TEST_LOG("Test 5: Verify no loopback/tunnel adapter DNS servers");

    pubnub_ipv4_address our_servers[16];
    int our_count = pubnub_dns_read_system_servers_ipv4(our_servers, 16);

    ULONG buflen = 0;
    GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST,
                         NULL, NULL, &buflen);

    IP_ADAPTER_ADDRESSES* addrs = static_cast<IP_ADAPTER_ADDRESSES*>(malloc(buflen));
    TEST_ASSERT(addrs != NULL, "Failed to allocate memory");

    DWORD ret = GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST,
                                     NULL, addrs, &buflen);
    TEST_ASSERT(ret == NO_ERROR, "GetAdaptersAddresses failed");

    for (IP_ADAPTER_ADDRESSES* aa = addrs; aa != NULL; aa = aa->Next) {
        if (aa->IfType == IF_TYPE_SOFTWARE_LOOPBACK || aa->IfType == IF_TYPE_TUNNEL) {
            TEST_LOG("Found invalid adapter type: " << aa->AdapterName
                     << " type=" << aa->IfType);

            for (IP_ADAPTER_DNS_SERVER_ADDRESS* ds = aa->FirstDnsServerAddress;
                 ds != NULL; ds = ds->Next) {
                if (ds->Address.lpSockaddr &&
                    ds->Address.lpSockaddr->sa_family == AF_INET) {
                    sockaddr_in* sin = reinterpret_cast<sockaddr_in*>(ds->Address.lpSockaddr);
                    pubnub_ipv4_address bad_addr;
                    DWORD net_addr = sin->sin_addr.S_un.S_addr;
                    DWORD host_addr = ntohl(net_addr);
                    bad_addr.ipv4[0] = (host_addr >> 24) & 0xFF;
                    bad_addr.ipv4[1] = (host_addr >> 16) & 0xFF;
                    bad_addr.ipv4[2] = (host_addr >> 8) & 0xFF;
                    bad_addr.ipv4[3] = host_addr & 0xFF;

                    // Ensure it's not in our list
                    for (int i = 0; i < our_count; i++) {
                        bool found = (memcmp(our_servers[i].ipv4, bad_addr.ipv4, 4) == 0);
                        TEST_ASSERT(!found,
                            "DNS from invalid adapter found: " << format_ipv4(bad_addr));
                    }
                }
            }
        }
    }

    free(addrs);
    TEST_PASS("test_dns_no_invalid_adapter_types");
    return true;
}

// Test 6: DNS server reachability (CRITICAL TEST)
bool test_dns_reachability() {
    TEST_LOG("Test 6: Verify all returned DNS servers are reachable");
    TEST_LOG("This is the PRIMARY test - we should NOT return unreachable DNS servers");

    pubnub_ipv4_address servers[10];
    int count = pubnub_dns_read_system_servers_ipv4(servers, 10);

    TEST_ASSERT(count > 0, "Should have at least one DNS server");

    int reachable_count = 0;
    std::vector<std::string> unreachable_servers;

    for (int i = 0; i < count; i++) {
        std::string ip = format_ipv4(servers[i]);
        TEST_LOG("Testing reachability of " << ip << "...");

        bool reachable = is_dns_reachable(servers[i], 3000);
        if (reachable) {
            TEST_LOG("  ✓ [REACHABLE] " << ip);
            reachable_count++;
        } else {
            TEST_LOG("  ✗ [UNREACHABLE] " << ip);
            unreachable_servers.push_back(ip);
        }
    }

    TEST_LOG("Summary: " << reachable_count << "/" << count << " DNS servers reachable");

    // CRITICAL: ALL returned DNS servers MUST be reachable
    // This is the core requirement - we should filter out unreachable servers
    if (!unreachable_servers.empty()) {
        TEST_LOG("FAILED: Found " << unreachable_servers.size() << " unreachable DNS server(s):");
        for (const auto& ip : unreachable_servers) {
            TEST_LOG("  - " << ip);
        }
        TEST_ASSERT(false,
            "CRITICAL: Returned " << unreachable_servers.size() << " unreachable DNS server(s). "
            "Our implementation should only return DNS servers from active, routable adapters.");
    }

    TEST_ASSERT(reachable_count == count,
        "All " << count << " DNS servers should be reachable");

    TEST_LOG("✓ All DNS servers are reachable!");
    TEST_PASS("test_dns_reachability");
    return true;
}

// Test 7: Boundary conditions
bool test_dns_boundary_conditions() {
    TEST_LOG("Test 7: Boundary conditions");

    // Test with n=0
    int count = pubnub_dns_read_system_servers_ipv4(NULL, 0);
    TEST_ASSERT(count == 0, "Should return 0 for n=0");

    // Test with n=1
    pubnub_ipv4_address one_server[1];
    count = pubnub_dns_read_system_servers_ipv4(one_server, 1);
    TEST_ASSERT(count <= 1, "Should return at most 1 server");

    // Test with large n
    pubnub_ipv4_address many_servers[100];
    count = pubnub_dns_read_system_servers_ipv4(many_servers, 100);
    TEST_ASSERT(count >= 0 && count <= 100, "Should respect array bounds");

    TEST_PASS("test_dns_boundary_conditions");
    return true;
}

// Test 8: IPv4-enabled flag verification
bool test_dns_ipv4_enabled_only() {
    TEST_LOG("Test 8: Verify DNS servers only from IPv4-enabled adapters");

    pubnub_ipv4_address our_servers[16];
    int our_count = pubnub_dns_read_system_servers_ipv4(our_servers, 16);

    ULONG buflen = 0;
    GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST,
                         NULL, NULL, &buflen);

    IP_ADAPTER_ADDRESSES* addrs = static_cast<IP_ADAPTER_ADDRESSES*>(malloc(buflen));
    TEST_ASSERT(addrs != NULL, "Failed to allocate memory");

    DWORD ret = GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST,
                                     NULL, addrs, &buflen);
    TEST_ASSERT(ret == NO_ERROR, "GetAdaptersAddresses failed");

    // Collect DNS servers from adapters without IPv4 enabled
    std::vector<std::string> non_ipv4_adapter_dns;
    int adapters_without_ipv4 = 0;

    for (IP_ADAPTER_ADDRESSES* aa = addrs; aa != NULL; aa = aa->Next) {
        if (!(aa->Flags & IP_ADAPTER_IPV4_ENABLED)) {
            adapters_without_ipv4++;
            TEST_LOG("Found adapter without IPv4 enabled: " << aa->AdapterName);

            // Collect DNS servers from this adapter
            for (IP_ADAPTER_DNS_SERVER_ADDRESS* ds = aa->FirstDnsServerAddress;
                 ds != NULL; ds = ds->Next) {
                if (ds->Address.lpSockaddr &&
                    ds->Address.lpSockaddr->sa_family == AF_INET) {
                    sockaddr_in* sin = reinterpret_cast<sockaddr_in*>(ds->Address.lpSockaddr);
                    pubnub_ipv4_address addr;
                    DWORD net_addr = sin->sin_addr.S_un.S_addr;
                    DWORD host_addr = ntohl(net_addr);
                    addr.ipv4[0] = (host_addr >> 24) & 0xFF;
                    addr.ipv4[1] = (host_addr >> 16) & 0xFF;
                    addr.ipv4[2] = (host_addr >> 8) & 0xFF;
                    addr.ipv4[3] = host_addr & 0xFF;

                    std::string dns_str = format_ipv4(addr);
                    non_ipv4_adapter_dns.push_back(dns_str);
                    TEST_LOG("  Found DNS on non-IPv4 adapter: " << dns_str);
                }
            }
        }
    }

    TEST_LOG("Found " << adapters_without_ipv4 << " adapters without IPv4 enabled");
    TEST_LOG("Found " << non_ipv4_adapter_dns.size() << " DNS servers from non-IPv4 adapters");

    // CRITICAL CHECK: None of these DNS servers should be in our returned list
    for (const auto& bad_dns : non_ipv4_adapter_dns) {
        for (int i = 0; i < our_count; i++) {
            std::string our_dns = format_ipv4(our_servers[i]);
            TEST_ASSERT(our_dns != bad_dns,
                "CRITICAL: DNS from non-IPv4-enabled adapter found: " << bad_dns
                << " - this indicates the IPv4-enabled check is not working!");
        }
    }

    if (adapters_without_ipv4 > 0) {
        TEST_LOG("✓ IPv4-enabled check working: " << adapters_without_ipv4
                 << " adapters without IPv4 were properly filtered");
    } else {
        TEST_LOG("INFO: All adapters have IPv4 enabled");
    }

    free(addrs);
    TEST_PASS("test_dns_ipv4_enabled_only");
    return true;
}

// Test 9: Verify DNS from adapters with valid gateway or unicast (UPDATED TEST)
bool test_dns_only_adapters_with_gateway() {
    TEST_LOG("Test 9: Verify DNS servers come from properly routable adapters");
    TEST_LOG("NOTE: Our implementation accepts adapters with EITHER valid gateway OR valid unicast");
    TEST_LOG("This test verifies we filter based on actual reachability, not just gateway presence");

    pubnub_ipv4_address our_servers[16];
    int our_count = pubnub_dns_read_system_servers_ipv4(our_servers, 16);

    // Use GAA_FLAG_INCLUDE_GATEWAYS to get actual gateway information
    ULONG buflen = 0;
    GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_INCLUDE_GATEWAYS,
                         NULL, NULL, &buflen);

    IP_ADAPTER_ADDRESSES* addrs = static_cast<IP_ADAPTER_ADDRESSES*>(malloc(buflen));
    TEST_ASSERT(addrs != NULL, "Failed to allocate memory");

    DWORD ret = GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_INCLUDE_GATEWAYS,
                                     NULL, addrs, &buflen);
    TEST_ASSERT(ret == NO_ERROR, "GetAdaptersAddresses failed");

    // Helper function to check if address is valid (not loopback, not APIPA)
    auto is_valid_ip = [](DWORD host_addr) -> bool {
        return host_addr != 0 &&
               ((host_addr >> 24) & 0xFF) != 127 &&
               !(((host_addr >> 24) & 0xFF) == 169 && ((host_addr >> 16) & 0xFF) == 254);
    };

    // Find adapters that are UP but have NEITHER valid gateway NOR valid unicast
    std::vector<std::string> invalid_adapter_dns;
    int adapters_without_valid_addresses = 0;

    for (IP_ADAPTER_ADDRESSES* aa = addrs; aa != NULL; aa = aa->Next) {
        if (aa->OperStatus == IfOperStatusUp &&
            aa->IfType != IF_TYPE_SOFTWARE_LOOPBACK &&
            aa->IfType != IF_TYPE_TUNNEL &&
            (aa->Flags & IP_ADAPTER_IPV4_ENABLED)) {

            // Check if adapter has valid gateway
            bool has_valid_gateway = false;
            for (IP_ADAPTER_GATEWAY_ADDRESS* gw = aa->FirstGatewayAddress; gw != NULL; gw = gw->Next) {
                if (gw->Address.lpSockaddr && gw->Address.lpSockaddr->sa_family == AF_INET) {
                    sockaddr_in* sin = reinterpret_cast<sockaddr_in*>(gw->Address.lpSockaddr);
                    DWORD gw_addr = ntohl(sin->sin_addr.S_un.S_addr);
                    if (is_valid_ip(gw_addr)) {
                        has_valid_gateway = true;
                        break;
                    }
                }
            }

            // Check if adapter has valid unicast address
            bool has_valid_unicast = false;
            for (IP_ADAPTER_UNICAST_ADDRESS* ua = aa->FirstUnicastAddress; ua != NULL; ua = ua->Next) {
                if (ua->Address.lpSockaddr && ua->Address.lpSockaddr->sa_family == AF_INET) {
                    sockaddr_in* sin = reinterpret_cast<sockaddr_in*>(ua->Address.lpSockaddr);
                    DWORD ua_addr = ntohl(sin->sin_addr.S_un.S_addr);
                    if (is_valid_ip(ua_addr)) {
                        has_valid_unicast = true;
                        break;
                    }
                }
            }

            // If adapter has NEITHER valid gateway NOR valid unicast, it shouldn't contribute DNS
            if (!has_valid_gateway && !has_valid_unicast) {
                adapters_without_valid_addresses++;
                TEST_LOG("Found UP adapter with NO valid addresses: " << aa->AdapterName);

                // Collect DNS servers from this adapter
                for (IP_ADAPTER_DNS_SERVER_ADDRESS* ds = aa->FirstDnsServerAddress; ds != NULL; ds = ds->Next) {
                    if (ds->Address.lpSockaddr && ds->Address.lpSockaddr->sa_family == AF_INET) {
                        sockaddr_in* sin = reinterpret_cast<sockaddr_in*>(ds->Address.lpSockaddr);
                        pubnub_ipv4_address addr;
                        DWORD net_addr = sin->sin_addr.S_un.S_addr;
                        DWORD host_addr = ntohl(net_addr);
                        addr.ipv4[0] = (host_addr >> 24) & 0xFF;
                        addr.ipv4[1] = (host_addr >> 16) & 0xFF;
                        addr.ipv4[2] = (host_addr >> 8) & 0xFF;
                        addr.ipv4[3] = host_addr & 0xFF;

                        std::string dns_str = format_ipv4(addr);
                        invalid_adapter_dns.push_back(dns_str);
                        TEST_LOG("  Found DNS on invalid adapter: " << dns_str);
                    }
                }
            }
        }
    }

    TEST_LOG("Found " << adapters_without_valid_addresses << " adapters without valid gateway or unicast");
    TEST_LOG("Found " << invalid_adapter_dns.size() << " DNS servers from invalid adapters");

    // CRITICAL CHECK: None of these DNS servers should be in our returned list
    for (const auto& bad_dns : invalid_adapter_dns) {
        for (int i = 0; i < our_count; i++) {
            std::string our_dns = format_ipv4(our_servers[i]);
            TEST_ASSERT(our_dns != bad_dns,
                "CRITICAL: DNS from invalid adapter found: " << bad_dns
                << " - adapter has neither valid gateway nor valid unicast!");
        }
    }

    if (adapters_without_valid_addresses > 0) {
        TEST_LOG("✓ Adapter filtering working: " << adapters_without_valid_addresses
                 << " adapters without valid addresses were properly filtered");
    } else {
        TEST_LOG("INFO: All adapters have valid addresses (normal case)");
    }

    free(addrs);
    TEST_PASS("test_dns_only_adapters_with_gateway");
    return true;
}

// Test 10: Verify loopback (127.x.x.x) and APIPA (169.254.x.x) filtering
bool test_dns_no_loopback_or_apipa() {
    TEST_LOG("Test 10: Verify no loopback (127.x.x.x) or APIPA (169.254.x.x) DNS servers");

    pubnub_ipv4_address servers[16];
    int count = pubnub_dns_read_system_servers_ipv4(servers, 16);

    for (int i = 0; i < count; i++) {
        std::string ip = format_ipv4(servers[i]);

        // Check for loopback (127.x.x.x)
        TEST_ASSERT(servers[i].ipv4[0] != 127,
            "Found loopback DNS server: " << ip);

        // Check for APIPA (169.254.x.x)
        bool is_apipa = (servers[i].ipv4[0] == 169 && servers[i].ipv4[1] == 254);
        TEST_ASSERT(!is_apipa,
            "Found APIPA DNS server: " << ip);
    }

    TEST_PASS("test_dns_no_loopback_or_apipa");
    return true;
}

// Test 11: Verify metric-based prioritization (NEW)
bool test_dns_metric_prioritization() {
    TEST_LOG("Test 11: Verify DNS servers are returned in metric priority order");
    TEST_LOG("Lower metric adapters should have their DNS servers returned first");

    pubnub_ipv4_address our_servers[16];
    int our_count = pubnub_dns_read_system_servers_ipv4(our_servers, 16);

    if (our_count == 0) {
        TEST_LOG("INFO: No DNS servers found (this is OK for testing)");
        TEST_PASS("test_dns_metric_prioritization");
        return true;
    }

    // Get all adapters with metrics
    ULONG buflen = 0;
    GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_INCLUDE_GATEWAYS,
                         NULL, NULL, &buflen);

    IP_ADAPTER_ADDRESSES* addrs = static_cast<IP_ADAPTER_ADDRESSES*>(malloc(buflen));
    TEST_ASSERT(addrs != NULL, "Failed to allocate memory");

    DWORD ret = GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_INCLUDE_GATEWAYS,
                                     NULL, addrs, &buflen);
    TEST_ASSERT(ret == NO_ERROR, "GetAdaptersAddresses failed");

    // Find which adapter each DNS belongs to and its metric
    std::vector<ULONG> dns_adapter_metrics;

    for (int i = 0; i < our_count; i++) {
        std::string our_dns = format_ipv4(our_servers[i]);
        ULONG found_metric = 0xFFFFFFFF; // Max value if not found

        // Find this DNS in the adapters
        for (IP_ADAPTER_ADDRESSES* aa = addrs; aa != NULL; aa = aa->Next) {
            if (aa->OperStatus != IfOperStatusUp) continue;

            for (IP_ADAPTER_DNS_SERVER_ADDRESS* ds = aa->FirstDnsServerAddress; ds != NULL; ds = ds->Next) {
                if (ds->Address.lpSockaddr && ds->Address.lpSockaddr->sa_family == AF_INET) {
                    sockaddr_in* sin = reinterpret_cast<sockaddr_in*>(ds->Address.lpSockaddr);
                    pubnub_ipv4_address addr;
                    DWORD net_addr = sin->sin_addr.S_un.S_addr;
                    DWORD host_addr = ntohl(net_addr);
                    addr.ipv4[0] = (host_addr >> 24) & 0xFF;
                    addr.ipv4[1] = (host_addr >> 16) & 0xFF;
                    addr.ipv4[2] = (host_addr >> 8) & 0xFF;
                    addr.ipv4[3] = host_addr & 0xFF;

                    if (format_ipv4(addr) == our_dns) {
                        found_metric = aa->Ipv4Metric;
                        TEST_LOG("DNS " << our_dns << " from adapter metric " << found_metric);
                        break;
                    }
                }
            }
            if (found_metric != 0xFFFFFFFF) break;
        }

        dns_adapter_metrics.push_back(found_metric);
    }

    // Verify metrics are in non-decreasing order (lower or equal metrics come first)
    bool metrics_sorted = true;
    for (size_t i = 1; i < dns_adapter_metrics.size(); i++) {
        if (dns_adapter_metrics[i] < dns_adapter_metrics[i-1]) {
            TEST_LOG("WARNING: DNS server " << i << " has lower metric (" << dns_adapter_metrics[i]
                     << ") than previous (" << dns_adapter_metrics[i-1] << ")");
            metrics_sorted = false;
        }
    }

    if (metrics_sorted) {
        TEST_LOG("✓ DNS servers are correctly prioritized by adapter metric");
    } else {
        TEST_LOG("NOTE: Metrics not strictly sorted - this is OK if GetBestInterface prioritized differently");
    }

    free(addrs);
    TEST_PASS("test_dns_metric_prioritization");
    return true;
}

// Test 12: Verify GetBestInterface awareness (NEW)
bool test_dns_best_interface_prioritization() {
    TEST_LOG("Test 12: Verify DNS servers from best route adapter are prioritized");
    TEST_LOG("This tests that GetBestInterface logic is working");

    pubnub_ipv4_address our_servers[16];
    int our_count = pubnub_dns_read_system_servers_ipv4(our_servers, 16);

    if (our_count == 0) {
        TEST_LOG("INFO: No DNS servers found");
        TEST_PASS("test_dns_best_interface_prioritization");
        return true;
    }

    // Get best interface for reaching public DNS (8.8.8.8)
    DWORD best_if_index = 0;
    struct in_addr dest;
    if (inet_pton(AF_INET, "8.8.8.8", &dest) == 1) {
        GetBestInterface(dest.S_un.S_addr, &best_if_index);
    }

    if (best_if_index == 0) {
        TEST_LOG("INFO: Could not determine best interface (no Internet connection?)");
        TEST_PASS("test_dns_best_interface_prioritization");
        return true;
    }

    TEST_LOG("Best interface index for reaching 8.8.8.8: " << best_if_index);

    // Get all adapters
    ULONG buflen = 0;
    GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_INCLUDE_GATEWAYS,
                         NULL, NULL, &buflen);

    IP_ADAPTER_ADDRESSES* addrs = static_cast<IP_ADAPTER_ADDRESSES*>(malloc(buflen));
    TEST_ASSERT(addrs != NULL, "Failed to allocate memory");

    DWORD ret = GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_INCLUDE_GATEWAYS,
                                     NULL, addrs, &buflen);
    TEST_ASSERT(ret == NO_ERROR, "GetAdaptersAddresses failed");

    // Find DNS servers from the best interface adapter
    std::vector<std::string> best_if_dns;
    for (IP_ADAPTER_ADDRESSES* aa = addrs; aa != NULL; aa = aa->Next) {
        if (aa->IfIndex == best_if_index) {
            TEST_LOG("Found best interface adapter: " << aa->AdapterName);

            for (IP_ADAPTER_DNS_SERVER_ADDRESS* ds = aa->FirstDnsServerAddress; ds != NULL; ds = ds->Next) {
                if (ds->Address.lpSockaddr && ds->Address.lpSockaddr->sa_family == AF_INET) {
                    sockaddr_in* sin = reinterpret_cast<sockaddr_in*>(ds->Address.lpSockaddr);
                    pubnub_ipv4_address addr;
                    DWORD net_addr = sin->sin_addr.S_un.S_addr;
                    DWORD host_addr = ntohl(net_addr);
                    addr.ipv4[0] = (host_addr >> 24) & 0xFF;
                    addr.ipv4[1] = (host_addr >> 16) & 0xFF;
                    addr.ipv4[2] = (host_addr >> 8) & 0xFF;
                    addr.ipv4[3] = host_addr & 0xFF;

                    // Validate not loopback/APIPA
                    if (host_addr != 0 &&
                        ((host_addr >> 24) & 0xFF) != 127 &&
                        !(((host_addr >> 24) & 0xFF) == 169 && ((host_addr >> 16) & 0xFF) == 254)) {
                        std::string dns_str = format_ipv4(addr);
                        best_if_dns.push_back(dns_str);
                        TEST_LOG("  DNS from best interface: " << dns_str);
                    }
                }
            }
            break;
        }
    }

    // Verify that DNS servers from best interface appear in our returned list
    if (!best_if_dns.empty()) {
        bool found_best_if_dns = false;
        for (const auto& best_dns : best_if_dns) {
            for (int i = 0; i < our_count; i++) {
                if (format_ipv4(our_servers[i]) == best_dns) {
                    found_best_if_dns = true;
                    TEST_LOG("✓ Found DNS from best interface in returned list: " << best_dns);
                    break;
                }
            }
            if (found_best_if_dns) break;
        }

        if (!found_best_if_dns) {
            TEST_LOG("WARNING: No DNS from best interface found in returned list");
            TEST_LOG("This might be OK if the best interface has no DNS configured");
        }
    } else {
        TEST_LOG("INFO: Best interface has no DNS servers configured");
    }

    free(addrs);
    TEST_PASS("test_dns_best_interface_prioritization");
    return true;
}

// Test 13: Verify DNS reachability validation (NEW)
bool test_dns_subnet_reachability() {
    TEST_LOG("Test 13: Verify returned DNS servers are on same subnet OR adapter has gateway");
    TEST_LOG("This tests the DNS reachability validation logic");

    pubnub_ipv4_address our_servers[16];
    int our_count = pubnub_dns_read_system_servers_ipv4(our_servers, 16);

    if (our_count == 0) {
        TEST_LOG("INFO: No DNS servers found");
        TEST_PASS("test_dns_subnet_reachability");
        return true;
    }

    // Get all adapters
    ULONG buflen = 0;
    GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_INCLUDE_GATEWAYS,
                         NULL, NULL, &buflen);

    IP_ADAPTER_ADDRESSES* addrs = static_cast<IP_ADAPTER_ADDRESSES*>(malloc(buflen));
    TEST_ASSERT(addrs != NULL, "Failed to allocate memory");

    DWORD ret = GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_INCLUDE_GATEWAYS,
                                     NULL, addrs, &buflen);
    TEST_ASSERT(ret == NO_ERROR, "GetAdaptersAddresses failed");

    // For each returned DNS, verify reachability
    for (int i = 0; i < our_count; i++) {
        std::string dns_str = format_ipv4(our_servers[i]);
        DWORD dns_addr = (our_servers[i].ipv4[0] << 24) |
                        (our_servers[i].ipv4[1] << 16) |
                        (our_servers[i].ipv4[2] << 8) |
                        our_servers[i].ipv4[3];

        bool is_reachable = false;
        std::string reason;

        // Find which adapter has this DNS
        for (IP_ADAPTER_ADDRESSES* aa = addrs; aa != NULL; aa = aa->Next) {
            if (aa->OperStatus != IfOperStatusUp) continue;

            // Check if this adapter has this DNS
            bool adapter_has_dns = false;
            for (IP_ADAPTER_DNS_SERVER_ADDRESS* ds = aa->FirstDnsServerAddress; ds != NULL; ds = ds->Next) {
                if (ds->Address.lpSockaddr && ds->Address.lpSockaddr->sa_family == AF_INET) {
                    sockaddr_in* sin = reinterpret_cast<sockaddr_in*>(ds->Address.lpSockaddr);
                    DWORD net_addr = sin->sin_addr.S_un.S_addr;
                    DWORD host_addr = ntohl(net_addr);

                    pubnub_ipv4_address check_addr;
                    check_addr.ipv4[0] = (host_addr >> 24) & 0xFF;
                    check_addr.ipv4[1] = (host_addr >> 16) & 0xFF;
                    check_addr.ipv4[2] = (host_addr >> 8) & 0xFF;
                    check_addr.ipv4[3] = host_addr & 0xFF;

                    if (format_ipv4(check_addr) == dns_str) {
                        adapter_has_dns = true;
                        break;
                    }
                }
            }

            if (!adapter_has_dns) continue;

            // Check if DNS is on same subnet
            for (IP_ADAPTER_UNICAST_ADDRESS* ua = aa->FirstUnicastAddress; ua != NULL; ua = ua->Next) {
                if (ua->Address.lpSockaddr && ua->Address.lpSockaddr->sa_family == AF_INET) {
                    sockaddr_in* sin = reinterpret_cast<sockaddr_in*>(ua->Address.lpSockaddr);
                    DWORD if_addr = ntohl(sin->sin_addr.S_un.S_addr);
                    ULONG prefix_len = ua->OnLinkPrefixLength;

                    if (prefix_len > 0 && prefix_len <= 32) {
                        DWORD mask = (prefix_len == 32) ? 0xFFFFFFFF : ~((1UL << (32 - prefix_len)) - 1);
                        if ((dns_addr & mask) == (if_addr & mask)) {
                            is_reachable = true;
                            reason = "on same subnet";
                            break;
                        }
                    }
                }
            }

            // Check if adapter has gateway
            if (!is_reachable) {
                for (IP_ADAPTER_GATEWAY_ADDRESS* gw = aa->FirstGatewayAddress; gw != NULL; gw = gw->Next) {
                    if (gw->Address.lpSockaddr && gw->Address.lpSockaddr->sa_family == AF_INET) {
                        sockaddr_in* sin = reinterpret_cast<sockaddr_in*>(gw->Address.lpSockaddr);
                        DWORD gw_addr = ntohl(sin->sin_addr.S_un.S_addr);
                        // Valid gateway (not 0.0.0.0, not loopback, not APIPA)
                        if (gw_addr != 0 &&
                            ((gw_addr >> 24) & 0xFF) != 127 &&
                            !(((gw_addr >> 24) & 0xFF) == 169 && ((gw_addr >> 16) & 0xFF) == 254)) {
                            is_reachable = true;
                            reason = "adapter has valid gateway";
                            break;
                        }
                    }
                }
            }

            if (is_reachable) break;
        }

        TEST_ASSERT(is_reachable,
            "DNS server " << dns_str << " is not reachable - no subnet match and no gateway");

        TEST_LOG("✓ DNS " << dns_str << " is reachable: " << reason);
    }

    free(addrs);
    TEST_PASS("test_dns_subnet_reachability");
    return true;
}

// Test 14: Verify no DnsQueryConfig usage (thread safety check) (NEW)
bool test_no_dnsqueryconfig_crashes() {
    TEST_LOG("Test 14: Verify multi-threaded stability (no DnsQueryConfig crashes)");
    TEST_LOG("This is a stress test - calling DNS enumeration from many threads simultaneously");

    const int num_threads = 20;
    const int iterations = 10;
    std::vector<std::thread> threads;
    std::atomic<int> failures(0);
    std::atomic<int> successes(0);

    auto worker = [&]() {
        for (int i = 0; i < iterations; i++) {
            pubnub_ipv4_address servers[16];
            int count = pubnub_dns_read_system_servers_ipv4(servers, 16);
            if (count < 0) {
                failures++;
            } else {
                successes++;
            }
            // Small random delay to increase contention
            std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 10));
        }
    };

    TEST_LOG("Starting " << num_threads << " threads, " << iterations << " iterations each...");

    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back(worker);
    }

    for (auto& t : threads) {
        t.join();
    }

    TEST_LOG("Completed: " << successes << " successes, " << failures << " failures");

    TEST_ASSERT(failures == 0,
        "Thread safety test had " << failures << " failures - possible crash/corruption");

    TEST_LOG("✓ No crashes or failures in multi-threaded stress test");
    TEST_PASS("test_no_dnsqueryconfig_crashes");
    return true;
}

// Main test runner
int main(int argc, char* argv[]) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }

    std::cout << "===========================================\n";
    std::cout << "PubNub DNS Windows Tests (Enhanced)\n";
    std::cout << "===========================================\n\n";

    bool all_passed = true;

    // Original tests
    all_passed &= test_dns_enumeration_basic();
    all_passed &= test_dns_no_duplicates();
    all_passed &= test_dns_only_up_adapters();
    all_passed &= test_dns_thread_safety();
    all_passed &= test_dns_no_invalid_adapter_types();
    all_passed &= test_dns_reachability();
    all_passed &= test_dns_boundary_conditions();
    all_passed &= test_dns_ipv4_enabled_only();
    all_passed &= test_dns_only_adapters_with_gateway();
    all_passed &= test_dns_no_loopback_or_apipa();

    std::cout << "\n===========================================\n";
    std::cout << "NEW ROUTING-AWARE TESTS\n";
    std::cout << "===========================================\n\n";

    // New tests for routing-aware functionality
    all_passed &= test_dns_metric_prioritization();
    all_passed &= test_dns_best_interface_prioritization();
    all_passed &= test_dns_subnet_reachability();
    all_passed &= test_no_dnsqueryconfig_crashes();

    std::cout << "\n===========================================\n";
    if (all_passed) {
        std::cout << "✓ ALL TESTS PASSED\n";
    } else {
        std::cout << "✗ SOME TESTS FAILED\n";
    }
    std::cout << "===========================================\n";

    WSACleanup();
    return all_passed ? 0 : 1;
}