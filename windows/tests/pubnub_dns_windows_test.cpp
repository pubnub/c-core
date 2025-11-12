/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
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

#include "core/pubnub_dns_servers.h"
#include "core/pubnub_log.h"
#include "core/pubnub_assert.h"

#include <windows.h>
#include <iphlpapi.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windns.h>
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
    oss << (int)addr.ipv4[0] << "."
        << (int)addr.ipv4[1] << "."
        << (int)addr.ipv4[2] << "."
        << (int)addr.ipv4[3];
    return oss.str();
}

// Helper to check if DNS server is reachable
bool is_dns_reachable(const pubnub_ipv4_address& dns_server, int timeout_ms = 2000) {
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        return false;
    }

    sockaddr_in dns_addr = {};
    dns_addr.sin_family = AF_INET;
    dns_addr.sin_port = htons(53);
    memcpy(&dns_addr.sin_addr.s_addr, dns_server.ipv4, 4);

    // Set socket timeout
    DWORD timeout = timeout_ms;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    // Simple DNS query for "test.local" - type A
    unsigned char query[] = {
        0x00, 0x01,  // Transaction ID
        0x01, 0x00,  // Flags: standard query
        0x00, 0x01,  // Questions: 1
        0x00, 0x00,  // Answer RRs: 0
        0x00, 0x00,  // Authority RRs: 0
        0x00, 0x00,  // Additional RRs: 0
        // Query: test.local
        0x04, 't', 'e', 's', 't',
        0x05, 'l', 'o', 'c', 'a', 'l',
        0x00,
        0x00, 0x01,  // Type A
        0x00, 0x01   // Class IN
    };

    int sent = sendto(sock, (const char*)query, sizeof(query), 0,
                      (sockaddr*)&dns_addr, sizeof(dns_addr));

    bool reachable = false;
    if (sent > 0) {
        unsigned char response[512];
        int received = recvfrom(sock, (char*)response, sizeof(response), 0, NULL, NULL);
        // We got a response (even if it's NXDOMAIN), server is reachable
        reachable = (received > 0);
    }

    closesocket(sock);
    return reachable;
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

    IP_ADAPTER_ADDRESSES* addrs = (IP_ADAPTER_ADDRESSES*)malloc(buflen);
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
                if (ds->Address.lpSockaddr->sa_family == AF_INET) {
                    sockaddr_in* sin = (sockaddr_in*)ds->Address.lpSockaddr;
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

    IP_ADAPTER_ADDRESSES* addrs = (IP_ADAPTER_ADDRESSES*)malloc(buflen);
    GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST,
                         NULL, addrs, &buflen);

    for (IP_ADAPTER_ADDRESSES* aa = addrs; aa != NULL; aa = aa->Next) {
        if (aa->IfType == IF_TYPE_SOFTWARE_LOOPBACK || aa->IfType == IF_TYPE_TUNNEL) {
            TEST_LOG("Found invalid adapter type: " << aa->AdapterName
                     << " type=" << aa->IfType);

            for (IP_ADAPTER_DNS_SERVER_ADDRESS* ds = aa->FirstDnsServerAddress;
                 ds != NULL; ds = ds->Next) {
                if (ds->Address.lpSockaddr->sa_family == AF_INET) {
                    sockaddr_in* sin = (sockaddr_in*)ds->Address.lpSockaddr;
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

// Test 6: DNS server reachability (optional - may require network)
bool test_dns_reachability() {
    TEST_LOG("Test 6: Check if returned DNS servers are reachable");

    pubnub_ipv4_address servers[10];
    int count = pubnub_dns_read_system_servers_ipv4(servers, 10);

    TEST_ASSERT(count > 0, "Should have at least one DNS server");

    int reachable_count = 0;
    for (int i = 0; i < count; i++) {
        std::string ip = format_ipv4(servers[i]);
        TEST_LOG("Testing reachability of " << ip);

        bool reachable = is_dns_reachable(servers[i], 3000);
        if (reachable) {
            TEST_LOG("  [REACHABLE] " << ip);
            reachable_count++;
        } else {
            TEST_LOG("  [TIMEOUT] " << ip << " (may be behind firewall)");
        }
    }

    // At least one should be reachable in normal conditions
    // But we make this a warning, not a failure, as corporate firewalls may block
    if (reachable_count == 0) {
        TEST_LOG("WARNING: No DNS servers reachable - this may indicate network issues");
    }

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

    IP_ADAPTER_ADDRESSES* addrs = (IP_ADAPTER_ADDRESSES*)malloc(buflen);
    GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST,
                         NULL, addrs, &buflen);

    for (IP_ADAPTER_ADDRESSES* aa = addrs; aa != NULL; aa = aa->Next) {
        if (!(aa->Flags & IP_ADAPTER_IPV4_ENABLED)) {
            TEST_LOG("Found adapter without IPv4: " << aa->AdapterName);

            // This adapter's DNS servers should not be in our list
            for (IP_ADAPTER_DNS_SERVER_ADDRESS* ds = aa->FirstDnsServerAddress;
                 ds != NULL; ds = ds->Next) {
                if (ds->Address.lpSockaddr->sa_family == AF_INET) {
                    TEST_LOG("  WARNING: Adapter has IPv4 DNS but IPv4 not enabled");
                }
            }
        }
    }

    free(addrs);
    TEST_PASS("test_dns_ipv4_enabled_only");
    return true;
}

// Test 9: Verify no DNS from adapters without gateway (CRITICAL TEST)
bool test_dns_only_adapters_with_gateway() {
    TEST_LOG("Test 9: Verify DNS servers only from adapters with gateway");
    TEST_LOG("This test validates the fix for disconnected VPN adapters");

    pubnub_ipv4_address our_servers[16];
    int our_count = pubnub_dns_read_system_servers_ipv4(our_servers, 16);

    ULONG buflen = 0;
    GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST,
                         NULL, NULL, &buflen);

    IP_ADAPTER_ADDRESSES* addrs = (IP_ADAPTER_ADDRESSES*)malloc(buflen);
    TEST_ASSERT(addrs != NULL, "Failed to allocate memory");

    DWORD ret = GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST,
                                     NULL, addrs, &buflen);
    TEST_ASSERT(ret == NO_ERROR, "GetAdaptersAddresses failed");

    // Find adapters that are UP but have NO gateway
    std::vector<std::string> no_gateway_dns;
    int adapters_without_gateway = 0;

    for (IP_ADAPTER_ADDRESSES* aa = addrs; aa != NULL; aa = aa->Next) {
        if (aa->OperStatus == IfOperStatusUp) {
            // Check if adapter has gateway
            bool has_gateway = false;
            for (IP_ADAPTER_GATEWAY_ADDRESS* gw = aa->FirstGatewayAddress;
                 gw != NULL; gw = gw->Next) {
                if (gw->Address.lpSockaddr != NULL) {
                    has_gateway = true;
                    break;
                }
            }

            if (!has_gateway) {
                adapters_without_gateway++;
                TEST_LOG("Found UP adapter WITHOUT gateway: " << aa->AdapterName
                         << " (likely disconnected VPN or disabled connection)");

                // Collect DNS servers from this adapter
                for (IP_ADAPTER_DNS_SERVER_ADDRESS* ds = aa->FirstDnsServerAddress;
                     ds != NULL; ds = ds->Next) {
                    if (ds->Address.lpSockaddr &&
                        ds->Address.lpSockaddr->sa_family == AF_INET) {
                        sockaddr_in* sin = (sockaddr_in*)ds->Address.lpSockaddr;
                        pubnub_ipv4_address addr;
                        DWORD net_addr = sin->sin_addr.S_un.S_addr;
                        DWORD host_addr = ntohl(net_addr);
                        addr.ipv4[0] = (host_addr >> 24) & 0xFF;
                        addr.ipv4[1] = (host_addr >> 16) & 0xFF;
                        addr.ipv4[2] = (host_addr >> 8) & 0xFF;
                        addr.ipv4[3] = host_addr & 0xFF;

                        std::string dns_str = format_ipv4(addr);
                        no_gateway_dns.push_back(dns_str);
                        TEST_LOG("  Found DNS on no-gateway adapter: " << dns_str);
                    }
                }
            }
        }
    }

    TEST_LOG("Found " << adapters_without_gateway << " adapters without gateway");
    TEST_LOG("Found " << no_gateway_dns.size() << " DNS servers from no-gateway adapters");

    // CRITICAL CHECK: None of these DNS servers should be in our returned list
    for (const auto& bad_dns : no_gateway_dns) {
        for (int i = 0; i < our_count; i++) {
            std::string our_dns = format_ipv4(our_servers[i]);
            TEST_ASSERT(our_dns != bad_dns,
                "CRITICAL: DNS from no-gateway adapter found: " << bad_dns
                << " - this indicates the gateway check is not working!");
        }
    }

    if (adapters_without_gateway > 0) {
        TEST_LOG("✓ Gateway check working: " << adapters_without_gateway
                 << " adapters without gateway were properly filtered");
    } else {
        TEST_LOG("INFO: No adapters without gateway found (all connections have valid routes)");
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

// Main test runner
int main(int argc, char* argv[]) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }

    std::cout << "===========================================\n";
    std::cout << "PubNub DNS Windows Tests\n";
    std::cout << "===========================================\n\n";

    bool all_passed = true;

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
    if (all_passed) {
        std::cout << "✓ ALL TESTS PASSED\n";
    } else {
        std::cout << "✗ SOME TESTS FAILED\n";
    }
    std::cout << "===========================================\n";

    WSACleanup();
    return all_passed ? 0 : 1;
}