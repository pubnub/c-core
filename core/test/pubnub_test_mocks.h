/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_TEST_MOCKS
#define      INC_PUBNUB_TEST_MOCKS

#include "core/pubnub_api_types.h"

void pubnub_setup_mocks(pubnub_t** pbp);

void pubnub_cleanup_mocks(pubnub_t* pbp);

#endif // INC_PUBNUB_TEST_MOCKS
