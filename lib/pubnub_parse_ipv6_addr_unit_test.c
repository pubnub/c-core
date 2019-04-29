/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"

#include "lib/pubnub_parse_ipv6_addr.h"
#include "core/pubnub_assert.h"
#include "core/pubnub_log.h"

#include "cgreen/cgreen.h"
#include "cgreen/mocks.h"

#include <assert.h>
#include <string.h>
#include <setjmp.h>

/* A less chatty cgreen :) */

#define attest assert_that
#define equals is_equal_to
#define streqs is_equal_to_string
#define differs is_not_equal_to
#define strdifs is_not_equal_to_string
#define ptreqs(val) is_equal_to_contents_of(&(val), sizeof(val))
#define ptrdifs(val) is_not_equal_to_contents_of(&(val), sizeof(val))
#define sets(par, val) will_set_contents_of_parameter(par, &(val), sizeof(val))
#define sets_ex will_set_contents_of_parameter
#define returns will_return

/* Assert "catching" */
static bool        m_expect_Assert;
static jmp_buf     m_Assert_exp_jmpbuf;
static char const* m_expect_Assert_file;


void assert_handler(char const* s, const char* file, long i)
{
    printf("%s:%ld: Pubnub assert failed '%s'\n", file, i, s);
}

void test_assert_handler(char const* s, const char* file, long i)
{
    assert_handler(s, file, i);

    attest(m_expect_Assert);
    attest(m_expect_Assert_file, streqs(file));
    if (m_expect_Assert) {
        m_expect_Assert = false;
        longjmp(m_Assert_exp_jmpbuf, 1);
    }
}

#define expect_assert_in(expr, file)                               \
    {                                                              \
        m_expect_Assert      = true;                               \
        m_expect_Assert_file = file;                               \
        int val              = setjmp(m_Assert_exp_jmpbuf);        \
        if (0 == val)                                              \
            expr;                                                  \
        attest(!m_expect_Assert);                                  \
    }

Describe(pubnub_parse_ipv6_addr);

BeforeEach(pubnub_parse_ipv6_addr)
{
    pubnub_assert_set_handler((pubnub_assert_handler_t)assert_handler);
}

AfterEach(pubnub_parse_ipv6_addr)
{
    PUBNUB_LOG_TRACE("========================================================\n");
}

Ensure(pubnub_parse_ipv6_addr, parses_local_host)
{
    /* Resolved Ipv6 address */
    uint8_t key[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1};
    struct pubnub_ipv6_address resolved_addr_ipv6;
    attest(pubnub_parse_ipv6_addr("::1", &resolved_addr_ipv6), equals(0));
    attest(memcmp(resolved_addr_ipv6.ipv6, key, sizeof resolved_addr_ipv6.ipv6), equals(0));
}

Ensure(pubnub_parse_ipv6_addr, parses_some_Ipv6_addresses)
{
    /* Resolved Ipv6 addresses */
    uint8_t key1[16] = {0x26,0x20,0x01,0x19,0,0x01,0,0x35,0,0,0,0,0xaf,0,0,0x35};
    uint8_t key2[16] = {0x26,0x20,0x01,0x19,0,0x01,0,0x35,0xaf,0,0,0x35,0,0,0,0x0e};
    uint8_t key3[16] = {0x26,0x20,0x01,0x19,0,0x01,0,0x35,0xaf,0,0,0x35};
    uint8_t key4[16] = {0,0x0a,0,};
    uint8_t key5[16] = {0,0,0,0xf,0};
    struct pubnub_ipv6_address resolved_addr_ipv6;
    attest(pubnub_parse_ipv6_addr("2620:119:1:35::Af00:35", &resolved_addr_ipv6), equals(0));
    attest(memcmp(resolved_addr_ipv6.ipv6, key1, sizeof resolved_addr_ipv6.ipv6), equals(0));
    attest(pubnub_parse_ipv6_addr("2620:119:1:35:Af00:35:0:e", &resolved_addr_ipv6), equals(0));
    attest(memcmp(resolved_addr_ipv6.ipv6, key2, sizeof resolved_addr_ipv6.ipv6), equals(0));
    attest(pubnub_parse_ipv6_addr("2620:119:1:35:Af00:35:0:", &resolved_addr_ipv6), equals(0));
    attest(memcmp(resolved_addr_ipv6.ipv6, key3, sizeof resolved_addr_ipv6.ipv6), equals(0));
    attest(pubnub_parse_ipv6_addr("a::", &resolved_addr_ipv6), equals(0));
    attest(memcmp(resolved_addr_ipv6.ipv6, key4, sizeof resolved_addr_ipv6.ipv6), equals(0));
    attest(pubnub_parse_ipv6_addr(":f::", &resolved_addr_ipv6), equals(0));
    attest(memcmp(resolved_addr_ipv6.ipv6, key5, sizeof resolved_addr_ipv6.ipv6), equals(0));
}

Ensure(pubnub_parse_ipv6_addr, parses_incomplete_Ipv6_address)
{
    struct pubnub_ipv6_address resolved_addr_ipv6;
    attest(pubnub_parse_ipv6_addr("2C2f:119:35:35", &resolved_addr_ipv6), equals(-1));
}

Ensure(pubnub_parse_ipv6_addr, parses_something_thats_not_Ipv6_address)
{
    struct pubnub_ipv6_address resolved_addr_ipv6;
    attest(pubnub_parse_ipv6_addr("::E:", &resolved_addr_ipv6), equals(-1));
    attest(pubnub_parse_ipv6_addr("A1.B2.c3.d4", &resolved_addr_ipv6), equals(-1));
    attest(pubnub_parse_ipv6_addr("1234", &resolved_addr_ipv6), equals(-1));
    attest(pubnub_parse_ipv6_addr("12345:", &resolved_addr_ipv6), equals(-1));
    attest(pubnub_parse_ipv6_addr("1234:g5:", &resolved_addr_ipv6), equals(-1));
    attest(pubnub_parse_ipv6_addr("F234:::37", &resolved_addr_ipv6), equals(-1));
    attest(pubnub_parse_ipv6_addr("F234:.37", &resolved_addr_ipv6), equals(-1));
    attest(pubnub_parse_ipv6_addr("1B3C:01:0:0:1234:FDCA:37:9032:15", &resolved_addr_ipv6),
           equals(-1));
    attest(pubnub_parse_ipv6_addr("2620:119:1:35:", &resolved_addr_ipv6), equals(-1));
}

/* Verify ASSERT gets fired */

Ensure(pubnub_parse_ipv6_addr, fires_asserts_on_illegal_parameters)
{
    struct pubnub_ipv6_address resolved_addr_ipv6;
    pubnub_assert_set_handler((pubnub_assert_handler_t)test_assert_handler);
    expect_assert_in(pubnub_parse_ipv6_addr(NULL, &resolved_addr_ipv6),
                     "pubnub_parse_ipv6_addr.c");
    expect_assert_in(pubnub_parse_ipv6_addr("::1", NULL),
                     "pubnub_parse_ipv6_addr.c");
}
