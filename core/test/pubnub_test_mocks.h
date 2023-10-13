/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_TEST_MOCKS
#define      INC_PUBNUB_TEST_MOCKS

#include "core/pubnub_api_types.h"
#include <time.h>

int _set_expected_assert(char const* file);

void _test_expected_assert();

#define expect_assert_in(expr, file)\
        {\
            int val = _set_expected_assert(file);\
            if (0 == val)\
                expr;\
            _test_expected_assert();\
        }

struct uint8_block {
    size_t   size;
    uint8_t* block;
};

void wait_time_in_seconds(time_t time_in_seconds);

void free_m_msgs(char** msg_array);

void incoming(char const* str, struct uint8_block* p_data);

void test_assert_handler(char const* s, const char* file, long i);

void pubnub_setup_mocks(pubnub_t** pbp);

void pubnub_cleanup_mocks(pubnub_t* pbp);

void expect_outgoing_with_url_no_params_on_ctx(pubnub_t* pbp, char const* url);

void expect_have_dns_for_pubnub_origin_on_ctx(pubnub_t* pbp);

void expect_outgoing_with_url_on_ctx(pubnub_t* pbp, char const* url);

#endif // INC_PUBNUB_TEST_MOCKS
