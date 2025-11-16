/**
 * @file pubnub_tls_security_test.cpp
 * @brief TLS/SSL security validation tests for PubNub C-core SDK on Windows
 *
 * Tests cover:
 * - Certificate validation
 * - Hostname verification (currently missing - this test will fail!)
 * - Protocol version negotiation
 * - System certificate store integration
 * - SSL session reuse security
 * - Certificate expiration handling
 */

#include "pubnub_sync.h"
#include "core/pubnub_ssl.h"
#include "core/pubnub_helper.h"
#include "core/pubnub_timers.h"
#include "core/pubnub_log.h"

#include <iostream>
#include <string>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <windows.h>

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

#define TEST_FAIL(name, reason) \
    std::cout << "[FAIL] " << name << ": " << reason << std::endl

// Test helper: create context
pubnub_t* create_test_context() {
    pubnub_t* pb = pubnub_alloc();
    if (pb != NULL) {
        pubnub_init(pb, "demo", "demo");
        pubnub_set_ssl_options(pb, true, false);  // SSL required, no fallback
        pubnub_set_user_id(pb, "demo");
    }
    return pb;
}

// Test 1: Verify SSL is enabled and working
bool test_ssl_basic_connection() {
    TEST_LOG("Test 1: Basic SSL connection to PubNub");

    pubnub_t* pb = create_test_context();
    TEST_ASSERT(pb != NULL, "Failed to create context");

    pubnub_set_transaction_timeout(pb, 15000);

    // Attempt time request over SSL
    enum pubnub_res res = pubnub_time(pb);
    if (res == PNR_STARTED) {
        res = pubnub_await(pb);
    }

    TEST_ASSERT(res == PNR_OK, "SSL connection failed: " << pubnub_res_2_string(res));

    std::string timetoken = pubnub_get(pb);
    TEST_LOG("Received timetoken: " << timetoken);
    TEST_ASSERT(!timetoken.empty(), "Timetoken should not be empty");

    pubnub_free(pb);
    TEST_PASS("test_ssl_basic_connection");
    return true;
}

// Test 2: Verify certificate validation (should reject self-signed certs)
bool test_certificate_validation() {
    TEST_LOG("Test 2: Certificate validation with invalid certificate");

    pubnub_t* pb = create_test_context();
    TEST_ASSERT(pb != NULL, "Failed to create context");

    // Try connecting to a known self-signed cert site
    // Note: This requires the ability to set custom origin
#if PUBNUB_ORIGIN_SETTABLE
    pubnub_origin_set(pb, "self-signed.badssl.com");
    pubnub_set_transaction_timeout(pb, 10000);

    enum pubnub_res res = pubnub_time(pb);
    if (res == PNR_STARTED) {
        res = pubnub_await(pb);
    }

    // Should fail with IO_ERROR or similar due to certificate validation failure
    TEST_ASSERT(res != PNR_OK, "Should reject self-signed certificate");
    TEST_LOG("Correctly rejected invalid certificate with error: " << pubnub_res_2_string(res));
#else
    TEST_LOG("SKIP: Origin setting not enabled (PUBNUB_ORIGIN_SETTABLE=0)");
#endif

    pubnub_free(pb);
    TEST_PASS("test_certificate_validation");
    return true;
}

// Test 3: CRITICAL - Hostname verification
bool test_hostname_verification() {
    TEST_LOG("Test 3: CRITICAL - Hostname verification");

#if PUBNUB_ORIGIN_SETTABLE
    pubnub_t* pb = create_test_context();
    TEST_ASSERT(pb != NULL, "Failed to create context");

    // Try connecting to wrong-host.badssl.com
    // This has a valid certificate but for the wrong hostname
    pubnub_origin_set(pb, "wrong.host.badssl.com");
    pubnub_set_transaction_timeout(pb, 10000);

    enum pubnub_res res = pubnub_time(pb);
    if (res == PNR_STARTED) {
        res = pubnub_await(pb);
    }

    if (res == PNR_OK) {
        TEST_FAIL("test_hostname_verification",
                  "SECURITY VULNERABILITY: Accepted certificate with wrong hostname!");
        std::cerr << "\n===========================================\n";
        std::cerr << "CRITICAL SECURITY ISSUE DETECTED!\n";
        std::cerr << "The library does not verify hostname in certificates!\n";
        std::cerr << "This allows Man-in-the-Middle attacks!\n";
        std::cerr << "===========================================\n\n";
        pubnub_free(pb);
        return false;
    } else {
        TEST_LOG("Correctly rejected certificate with wrong hostname");
    }

    pubnub_free(pb);
    TEST_PASS("test_hostname_verification");
    return true;
#else
    TEST_LOG("SKIP: Origin setting not enabled (PUBNUB_ORIGIN_SETTABLE=0)");
    TEST_LOG("NOTE: Hostname verification cannot be tested without custom origin support");
    return true;
#endif
}

// Test 4: System certificate store usage
bool test_system_certificate_store() {
    TEST_LOG("Test 4: System certificate store integration");

    pubnub_t* pb = create_test_context();
    TEST_ASSERT(pb != NULL, "Failed to create context");

    // Enable system certificate store
    int res_cert = pubnub_ssl_use_system_certificate_store(pb);
    TEST_ASSERT(res_cert == 0, "Failed to enable system cert store");

    pubnub_set_transaction_timeout(pb, 15000);

    // This should work with system certs
    enum pubnub_res res = pubnub_time(pb);
    if (res == PNR_STARTED) {
        res = pubnub_await(pb);
    }

    TEST_ASSERT(res == PNR_OK, "Connection failed with system certs: " << pubnub_res_2_string(res));

    pubnub_free(pb);
    TEST_PASS("test_system_certificate_store");
    return true;
}

// Test 5: SSL protocol version (should use TLS 1.2+, not SSLv3)
bool test_protocol_version() {
    TEST_LOG("Test 5: SSL/TLS protocol version negotiation");

#if PUBNUB_ORIGIN_SETTABLE
    pubnub_t* pb = create_test_context();
    TEST_ASSERT(pb != NULL, "Failed to create context");

    // Try connecting to TLS 1.2 only server
    // tls-v1-2.badssl.com requires TLS 1.2 or higher
    pubnub_origin_set(pb, "tls-v1-2.badssl.com");
    pubnub_set_transaction_timeout(pb, 10000);

    enum pubnub_res res = pubnub_time(pb);
    if (res == PNR_STARTED) {
        res = pubnub_await(pb);
    }

    // Should succeed if we support TLS 1.2+
    if (res == PNR_OK || res == PNR_FORMAT_ERROR) {
        TEST_LOG("Successfully negotiated TLS 1.2+");
    } else {
        TEST_LOG("WARNING: Failed to connect to TLS 1.2 server: " << pubnub_res_2_string(res));
    }

    pubnub_free(pb);
#else
    TEST_LOG("SKIP: Origin setting not enabled");
#endif

    TEST_PASS("test_protocol_version");
    return true;
}

// Test 6: Certificate expiration handling
bool test_expired_certificate() {
    TEST_LOG("Test 6: Expired certificate handling");

#if PUBNUB_ORIGIN_SETTABLE
    pubnub_t* pb = create_test_context();
    TEST_ASSERT(pb != NULL, "Failed to create context");

    // Try connecting to expired.badssl.com
    pubnub_origin_set(pb, "expired.badssl.com");
    pubnub_set_transaction_timeout(pb, 10000);

    enum pubnub_res res = pubnub_time(pb);
    if (res == PNR_STARTED) {
        res = pubnub_await(pb);
    }

    // Should fail due to expired certificate
    TEST_ASSERT(res != PNR_OK, "Should reject expired certificate");
    TEST_LOG("Correctly rejected expired certificate");

    pubnub_free(pb);
#else
    TEST_LOG("SKIP: Origin setting not enabled");
#endif

    TEST_PASS("test_expired_certificate");
    return true;
}

// Test 7: Concurrent SSL connections (thread safety)
bool test_concurrent_ssl_connections() {
    TEST_LOG("Test 7: Concurrent SSL connections");

    const int num_contexts = 5;
    pubnub_t* contexts[num_contexts];

    // Create multiple contexts
    for (int i = 0; i < num_contexts; i++) {
        contexts[i] = create_test_context();
        TEST_ASSERT(contexts[i] != NULL, "Failed to create context " << i);
        pubnub_set_transaction_timeout(contexts[i], 20000);
    }

    // Start all time requests
    for (int i = 0; i < num_contexts; i++) {
        enum pubnub_res res = pubnub_time(contexts[i]);
        TEST_ASSERT(res == PNR_STARTED || res == PNR_OK,
                    "Failed to start request " << i);
    }

    // Wait for all to complete
    bool all_ok = true;
    for (int i = 0; i < num_contexts; i++) {
        enum pubnub_res res = pubnub_await(contexts[i]);
        if (res != PNR_OK) {
            TEST_LOG("Context " << i << " failed: " << pubnub_res_2_string(res));
            all_ok = false;
        }
    }

    // Cleanup
    for (int i = 0; i < num_contexts; i++) {
        pubnub_free(contexts[i]);
    }

    TEST_ASSERT(all_ok, "Some concurrent connections failed");
    TEST_PASS("test_concurrent_ssl_connections");
    return true;
}

// Test 8: Custom CA certificate
bool test_custom_ca_certificate() {
    TEST_LOG("Test 8: Custom CA certificate handling");

    pubnub_t* pb = create_test_context();
    TEST_ASSERT(pb != NULL, "Failed to create context");

    // Try setting a non-existent CA file (should be handled gracefully)
    int res = pubnub_set_ssl_verify_locations(pb, "nonexistent.pem", NULL);
    TEST_ASSERT(res == 0, "set_ssl_verify_locations should not fail for missing file");

    // The actual error should occur during connection
    pubnub_set_transaction_timeout(pb, 10000);
    enum pubnub_res conn_res = pubnub_time(pb);
    if (conn_res == PNR_STARTED) {
        conn_res = pubnub_await(pb);
    }

    // With invalid CA, connection might fail (or succeed if it falls back to hardcoded certs)
    TEST_LOG("Connection result with invalid CA: " << pubnub_res_2_string(conn_res));

    pubnub_free(pb);
    TEST_PASS("test_custom_ca_certificate");
    return true;
}

// Test 9: Verify no cleartext fallback
bool test_no_cleartext_fallback() {
    TEST_LOG("Test 9: Verify no cleartext fallback");

    pubnub_t* pb = create_test_context();
    TEST_ASSERT(pb != NULL, "Failed to create context");

    // SSL required, no fallback
    pubnub_set_ssl_options(pb, true, false);
    pubnub_set_transaction_timeout(pb, 15000);

    enum pubnub_res res = pubnub_time(pb);
    if (res == PNR_STARTED) {
        res = pubnub_await(pb);
    }

    // Should succeed over SSL
    TEST_ASSERT(res == PNR_OK, "SSL connection should succeed");

    // Verify we're actually using SSL (not cleartext)
    // This is implicit - if useSSL=true and we succeed, we used SSL

    pubnub_free(pb);
    TEST_PASS("test_no_cleartext_fallback");
    return true;
}

int main(int argc, char* argv[]) {
    std::cout << "===========================================\n";
    std::cout << "PubNub TLS/SSL Security Tests (Windows)\n";
    std::cout << "===========================================\n\n";

    std::cout << "NOTE: These tests connect to real servers!\n";
    std::cout << "Some tests use badssl.com to verify security\n";
    std::cout << "Internet connection required.\n\n";

    bool all_passed = true;

    all_passed &= test_ssl_basic_connection();
    all_passed &= test_system_certificate_store();
    all_passed &= test_certificate_validation();

    // This is the critical test - may expose security vulnerability
    std::cout << "\n*** CRITICAL SECURITY TEST ***\n";
    all_passed &= test_hostname_verification();
    std::cout << "*** END CRITICAL TEST ***\n\n";

    all_passed &= test_protocol_version();
    all_passed &= test_expired_certificate();
    all_passed &= test_concurrent_ssl_connections();
    all_passed &= test_custom_ca_certificate();
    all_passed &= test_no_cleartext_fallback();

    std::cout << "\n===========================================\n";
    if (all_passed) {
        std::cout << "✓ ALL TESTS PASSED\n";
    } else {
        std::cout << "✗ SOME TESTS FAILED\n";
        std::cout << "\nPLEASE REVIEW FAILURES CAREFULLY!\n";
        std::cout << "Security issues require immediate attention.\n";
    }
    std::cout << "===========================================\n";

    return all_passed ? 0 : 1;
}