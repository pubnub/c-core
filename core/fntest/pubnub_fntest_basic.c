/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_sync.h"

#include "pubnub_fntest.h"
#include "core/pubnub_blocking_io.h"
#include "core/pubnub_log.h"

#include <stdlib.h>


#define SECONDS 1000
#define CHANNEL_REGISTRY_PROPAGATION_DELAY 1000


TEST_DEF(simple_connect_and_send_over_single_channel)
{
    static pubnub_t* pbp;
    char* const      chan = pnfntst_make_name(this_test_name_);
    TEST_DEFER(free, chan);
    pbp = pnfntst_create_ctx();
    TEST_DEFER(pnfntst_free, pbp);

    expect_PNR_OK(pbp, pubnub_subscribe(pbp, chan, NULL), 10 * SECONDS);
    expect_PNR_OK(pbp, pubnub_publish(pbp, chan, "\"Test 1\""), 10 * SECONDS);
    expect_PNR_OK(pbp, pubnub_publish(pbp, chan, "\"Test 1-2\""), 10 * SECONDS);
    expect(pnfntst_subscribe_and_check(
        pbp, chan, NULL, 10 * SECONDS, "\"Test 1\"", NULL, "\"Test 1-2\"", NULL, NULL));

    TEST_POP_DEFERRED;
    TEST_POP_DEFERRED;
}
TEST_ENDDEF


TEST_DEF(connect_and_send_over_several_channels_simultaneously)
{
    static pubnub_t* pbp;
    char* const      chan = pnfntst_make_name(this_test_name_);
    TEST_DEFER(free, chan);
    pbp = pnfntst_create_ctx();
    TEST_DEFER(pnfntst_free, pbp);

    expect_PNR_OK(pbp, pubnub_subscribe(pbp, chan, NULL), 10 * SECONDS);
    expect_PNR_OK(pbp, pubnub_publish(pbp, chan, "\"Test M1\""), 10 * SECONDS);
    expect_PNR_OK(pbp, pubnub_publish(pbp, "two", "\"Test M1 - 2\""), 10 * SECONDS);
    expect_PNR_OK(pbp, pubnub_subscribe(pbp, chan, NULL), 10 * SECONDS);
    expect(pnfntst_got_messages(pbp, "\"Test M1\"", NULL));

    expect_PNR_OK(pbp, pubnub_subscribe(pbp, "two", NULL), 10 * SECONDS);
    expect(pnfntst_got_messages(pbp, "\"Test M1 - 2\"", NULL));

    TEST_POP_DEFERRED;
    TEST_POP_DEFERRED;
}
TEST_ENDDEF


TEST_DEF_NEED_CHGROUP(simple_connect_and_send_over_single_channel_in_group)
{
    static pubnub_t* pbp;
    char* const      chgrp = pnfntst_make_name(this_test_name_);
    char* const      chan  = pnfntst_make_name(this_test_name_);
    enum pubnub_res  rslt;
    int              tries = 0;
    TEST_DEFER(free, chgrp);
    TEST_DEFER(free, chan);
    pbp = pnfntst_create_ctx();
    TEST_DEFER(pnfntst_free, pbp);

    expect_PNR_OK(pbp, pubnub_remove_channel_group(pbp, chgrp), 10 * SECONDS);
    expect_PNR_OK(pbp, pubnub_add_channel_to_group(pbp, chan, chgrp), 10 * SECONDS);

    TEST_SLEEP_FOR(CHANNEL_REGISTRY_PROPAGATION_DELAY);
//
    do {
        rslt = pubnub_list_channel_group(pbp, chgrp);
        if (PNR_STARTED == rslt) {
            rslt = pubnub_await(pbp);
        }
        expect(PNR_OK == rslt);
        rslt = pubnub_subscribe(pbp, NULL, chgrp);
        if (PNR_STARTED == rslt) {
            rslt = pubnub_await(pbp);
        }
    } while ((++tries < 100) && (PNR_FORMAT_ERROR == rslt));
    PUBNUB_LOG_TRACE("---->pubnub_subscribe(pb=%p, chgrp) tries %d times.\n", pbp, tries);
    expect(PNR_OK == rslt);
//    expect_PNR_OK(pbp, pubnub_subscribe(pbp, NULL, chgrp), 10 * SECONDS);
    expect_PNR_OK(pbp, pubnub_publish(pbp, chan, "\"Test 2\""), 10 * SECONDS);
    expect_PNR_OK(pbp, pubnub_publish(pbp, chan, "\"Test 2 - 2\""), 10 * SECONDS);
    expect_PNR_OK(pbp, pubnub_subscribe(pbp, NULL, chgrp), 10 * SECONDS);
    expect(pnfntst_got_messages(pbp, "\"Test 2\"", "\"Test 2 - 2\"", NULL));

    expect_PNR_OK(pbp, pubnub_remove_channel_group(pbp, chgrp), 10 * SECONDS);

    TEST_POP_DEFERRED;
    TEST_POP_DEFERRED;
    TEST_POP_DEFERRED;
}
TEST_ENDDEF


TEST_DEF_NEED_CHGROUP(connect_and_send_over_several_channels_in_group_simultaneously)
{
    static pubnub_t* pbp;
    char* const      chgrp = pnfntst_make_name(this_test_name_);
    enum pubnub_res  rslt;
    int              tries = 0;
    TEST_DEFER(free, chgrp);
    pbp = pnfntst_create_ctx();
    TEST_DEFER(pnfntst_free, pbp);

    expect_PNR_OK(pbp, pubnub_remove_channel_group(pbp, chgrp), 10 * SECONDS);
    expect_PNR_OK(
        pbp, pubnub_add_channel_to_group(pbp, "ch,two", chgrp), 10 * SECONDS);

    TEST_SLEEP_FOR(CHANNEL_REGISTRY_PROPAGATION_DELAY);
//
    do {
        rslt = pubnub_list_channel_group(pbp, chgrp);
        if (PNR_STARTED == rslt) {
            rslt = pubnub_await(pbp);
        }
        expect(PNR_OK == rslt);
        rslt = pubnub_subscribe(pbp, NULL, chgrp);
        if (PNR_STARTED == rslt) {
            rslt = pubnub_await(pbp);
        }
    } while ((++tries < 100) && (PNR_FORMAT_ERROR == rslt));
    PUBNUB_LOG_TRACE("---->pubnub_subscribe(pb=%p, chgrp) tries %d times.\n", pbp, tries);
    expect(PNR_OK == rslt);
//    expect_PNR_OK(pbp, pubnub_subscribe(pbp, NULL, chgrp), 10 * SECONDS);
    expect_PNR_OK(pbp, pubnub_publish(pbp, "ch", "\"Test M2\""), 10 * SECONDS);
    expect_PNR_OK(pbp, pubnub_publish(pbp, "two", "\"Test M2-2\""), 10 * SECONDS);
    expect(pnfntst_subscribe_and_check(
        pbp, NULL, chgrp, 10 * SECONDS, "\"Test M2\"", "ch", "\"Test M2-2\"", "two", NULL));

    expect_PNR_OK(pbp, pubnub_remove_channel_group(pbp, chgrp), 10 * SECONDS);

    TEST_POP_DEFERRED;
    TEST_POP_DEFERRED;
}
TEST_ENDDEF


TEST_DEF_NEED_CHGROUP(connect_and_send_over_channel_in_group_and_single_channel_simultaneously)
{
    static pubnub_t* pbp;
    char* const      chgrp = pnfntst_make_name(this_test_name_);
    TEST_DEFER(free, chgrp);
    pbp = pnfntst_create_ctx();
    TEST_DEFER(pnfntst_free, pbp);

    expect_PNR_OK(pbp, pubnub_remove_channel_group(pbp, chgrp), 10 * SECONDS);
    expect_PNR_OK(pbp, pubnub_add_channel_to_group(pbp, "ch", chgrp), 10 * SECONDS);

    TEST_SLEEP_FOR(CHANNEL_REGISTRY_PROPAGATION_DELAY);

    expect_PNR_OK(pbp, pubnub_subscribe(pbp, "two", chgrp), 10 * SECONDS);
    expect_PNR_OK(pbp, pubnub_publish(pbp, "ch", "\"Test M3 - 1\""), 10 * SECONDS);
    expect_PNR_OK(pbp, pubnub_publish(pbp, "two", "\"Test M3 - 2\""), 10 * SECONDS);
    expect(pnfntst_subscribe_and_check(
        pbp, "two", chgrp, 10 * SECONDS, "\"Test M3 - 1\"", "ch", "\"Test M3 - 2\"", "two", NULL));

    expect_PNR_OK(pbp, pubnub_remove_channel_group(pbp, chgrp), 10 * SECONDS);

    TEST_POP_DEFERRED;
    TEST_POP_DEFERRED;
}
TEST_ENDDEF


TEST_DEF_NEED_CHGROUP(connect_and_send_over_channel_in_group_and_multi_channel_simultaneously)
{
    static pubnub_t* pbp;
    char* const      chgrp = pnfntst_make_name(this_test_name_);
    TEST_DEFER(free, chgrp);
    pbp = pnfntst_create_ctx();
    TEST_DEFER(pnfntst_free, pbp);

    expect_PNR_OK(pbp, pubnub_remove_channel_group(pbp, chgrp), 10 * SECONDS);
    expect_PNR_OK(pbp, pubnub_add_channel_to_group(pbp, "three", chgrp), 10 * SECONDS);

    TEST_SLEEP_FOR(CHANNEL_REGISTRY_PROPAGATION_DELAY);

    expect_PNR_OK(pbp, pubnub_subscribe(pbp, "ch,two", chgrp), 10 * SECONDS);
    expect_PNR_OK(pbp, pubnub_publish(pbp, "ch", "\"Test M4 - ch\""), 10 * SECONDS);
    expect_PNR_OK(pbp, pubnub_publish(pbp, "two", "\"Test M4 - two\""), 10 * SECONDS);
    expect_PNR_OK(pbp, pubnub_publish(pbp, "three", "\"Test M4-3\""), 10 * SECONDS);
    expect(pnfntst_subscribe_and_check(pbp,
                                       "ch,two",
                                       chgrp,
                                       10 * SECONDS,
                                       "\"Test M4 - ch\"",
                                       "ch",
                                       "\"Test M4 - two\"",
                                       "two",
                                       "\"Test M4-3\"",
                                       "three",
                                       NULL));

    expect_PNR_OK(pbp, pubnub_remove_channel_group(pbp, chgrp), 10 * SECONDS);

    TEST_POP_DEFERRED;
    TEST_POP_DEFERRED;
}
TEST_ENDDEF


TEST_DEF(simple_connect_and_receiver_over_single_channel)
{
    static pubnub_t* pbp;
    static pubnub_t* pbp_2;
    enum pubnub_res  rslt;
    char* const      chan = pnfntst_make_name(this_test_name_);
    TEST_DEFER(free, chan);
    pbp = pnfntst_create_ctx();
    TEST_DEFER(pnfntst_free, pbp);
    pbp_2 = pnfntst_create_ctx();
    TEST_DEFER(pnfntst_free, pbp_2);
    pubnub_set_non_blocking_io(pbp_2);

    expect_PNR_OK(pbp_2, pubnub_subscribe(pbp_2, chan, NULL), 10 * SECONDS);
    rslt = pubnub_subscribe(pbp_2, chan, NULL);
    expect_pnr(rslt, PNR_STARTED);
    rslt = pubnub_publish(pbp, chan, "\"Test 3 - 1\"");
    expect_pnr(rslt, PNR_STARTED);
    await_timed_2(10 * SECONDS, PNR_OK, pbp, PNR_OK, pbp_2);
    expect(pnfntst_got_messages(pbp_2, "\"Test 3 - 1\"", NULL));

    expect_PNR_OK(pbp, pubnub_publish(pbp, chan, "\"Test 3 - 2\""), 10 * SECONDS);
    expect_PNR_OK(pbp_2, pubnub_subscribe(pbp_2, chan, NULL), 10 * SECONDS);
    expect(pnfntst_got_messages(pbp_2, "\"Test 3 - 2\"", NULL));

    TEST_POP_DEFERRED;
    TEST_POP_DEFERRED;
    TEST_POP_DEFERRED;
}
TEST_ENDDEF


TEST_DEF(connect_and_receive_over_several_channels_simultaneously)
{
    static pubnub_t* pbp;
    static pubnub_t* pbp_2;
    char             chanlist[100];
    pbp = pnfntst_create_ctx();
    TEST_DEFER(pnfntst_free, pbp);
    pbp_2 = pnfntst_create_ctx();
    TEST_DEFER(pnfntst_free, pbp_2);

    snprintf(chanlist, sizeof chanlist, "%s,two", this_test_name_);
    expect_PNR_OK(pbp_2, pubnub_subscribe(pbp_2, chanlist, NULL), 10 * SECONDS);
    expect_PNR_OK(pbp, pubnub_publish(pbp, this_test_name_, "\"M5\""), 10 * SECONDS);
    expect_PNR_OK(pbp, pubnub_publish(pbp, "two", "\"M5-2\""), 10 * SECONDS);
    expect(pnfntst_subscribe_and_check(
        pbp_2, chanlist, NULL, 10 * SECONDS, "\"M5\"", this_test_name_, "\"M5-2\"", "two", NULL));

    TEST_POP_DEFERRED;
    TEST_POP_DEFERRED;
}
TEST_ENDDEF


TEST_DEF_NEED_CHGROUP(simple_connect_and_receiver_over_single_channel_in_group)
{
    static pubnub_t* pbp;
    static pubnub_t* pbp_2;
    enum pubnub_res  rslt;
    char* const      chgrp = pnfntst_make_name(this_test_name_);
    char* const      chan  = chgrp;
    int              tries = 0;
    TEST_DEFER(free, chgrp);
    pbp = pnfntst_create_ctx();
    TEST_DEFER(pnfntst_free, pbp);
    pbp_2 = pnfntst_create_ctx();
    TEST_DEFER(pnfntst_free, pbp_2);
    pubnub_set_non_blocking_io(pbp_2);

    expect_PNR_OK(pbp_2, pubnub_remove_channel_group(pbp_2, chgrp), 10 * SECONDS);
    expect_PNR_OK(
        pbp_2, pubnub_add_channel_to_group(pbp_2, chan, chgrp), 10 * SECONDS);

    TEST_SLEEP_FOR(CHANNEL_REGISTRY_PROPAGATION_DELAY);
//
    do {
        rslt = pubnub_list_channel_group(pbp_2, chgrp);
        if (PNR_STARTED == rslt) {
            rslt = pubnub_await(pbp_2);
        }
        expect(PNR_OK == rslt);
        rslt = pubnub_subscribe(pbp_2, NULL, chgrp);
        if (PNR_STARTED == rslt) {
            rslt = pubnub_await(pbp_2);
        }
    } while ((++tries < 100) && (PNR_FORMAT_ERROR == rslt));
    PUBNUB_LOG_TRACE("---->pubnub_subscribe(pb=%p, chgrp) tries %d times.\n", pbp_2, tries);
    expect(PNR_OK == rslt);
//    expect_PNR_OK(pbp_2, pubnub_subscribe(pbp_2, NULL, chgrp), 10 * SECONDS);
    expect_pnr(pubnub_subscribe(pbp_2, NULL, chgrp), PNR_STARTED);
    expect_pnr(pubnub_publish(pbp, chan, "\"Test 4\""), PNR_STARTED);
    await_timed_2(10 * SECONDS, PNR_OK, pbp, PNR_OK, pbp_2);
    expect(pnfntst_got_messages(pbp_2, "\"Test 4\"", NULL));

    expect_PNR_OK(pbp, pubnub_publish(pbp, chan, "\"Test 4 - 4\""), 10 * SECONDS);
    expect_PNR_OK(pbp_2, pubnub_subscribe(pbp_2, NULL, chgrp), 10 * SECONDS);
    expect(pnfntst_got_messages(pbp_2, "\"Test 4 - 4\"", NULL));

    expect_PNR_OK(pbp_2, pubnub_remove_channel_group(pbp_2, chgrp), 10 * SECONDS);

    TEST_POP_DEFERRED;
    TEST_POP_DEFERRED;
    TEST_POP_DEFERRED;
}
TEST_ENDDEF


TEST_DEF_NEED_CHGROUP(connect_and_receive_over_several_channels_in_group_simultaneously)
{

    static pubnub_t* pbp;
    static pubnub_t* pbp_2;
    enum pubnub_res  rslt;
    char* const      chgrp = pnfntst_make_name(this_test_name_);
    int              tries = 0;
    TEST_DEFER(free, chgrp);
    pbp = pnfntst_create_ctx();
    TEST_DEFER(pnfntst_free, pbp);
    pbp_2 = pnfntst_create_ctx();
    TEST_DEFER(pnfntst_free, pbp_2);
    pubnub_set_non_blocking_io(pbp_2);

    expect_PNR_OK(pbp_2, pubnub_remove_channel_group(pbp_2, chgrp), 10 * SECONDS);
    expect_PNR_OK(
        pbp_2, pubnub_add_channel_to_group(pbp_2, "ch,two", chgrp), 10 * SECONDS);

    TEST_SLEEP_FOR(CHANNEL_REGISTRY_PROPAGATION_DELAY);
//
    do {
        rslt = pubnub_list_channel_group(pbp_2, chgrp);
        if (PNR_STARTED == rslt) {
            rslt = pubnub_await(pbp_2);
        }
        expect(PNR_OK == rslt);
        rslt = pubnub_subscribe(pbp_2, NULL, chgrp);
        if (PNR_STARTED == rslt) {
            rslt = pubnub_await(pbp_2);
        }
    } while ((++tries < 100) && (PNR_FORMAT_ERROR == rslt));
    PUBNUB_LOG_TRACE("---->pubnub_subscribe(pb=%p, chgrp) tries %d times.\n", pbp_2, tries);
    expect(PNR_OK == rslt);
//    expect_PNR_OK(pbp_2, pubnub_subscribe(pbp_2, NULL, chgrp), 10 * SECONDS);
    expect_PNR_OK(pbp, pubnub_publish(pbp, "ch", "\"Test M6\""), 10 * SECONDS);
    expect_PNR_OK(pbp, pubnub_publish(pbp, "two", "\"Test M6-2\""), 10 * SECONDS);
    expect(pnfntst_subscribe_and_check(
        pbp_2, NULL, chgrp, 10 * SECONDS, "\"Test M6\"", "ch", "\"Test M6-2\"", "two", NULL));

    expect_PNR_OK(pbp_2, pubnub_remove_channel_group(pbp_2, chgrp), 10 * SECONDS);

    TEST_POP_DEFERRED;
    TEST_POP_DEFERRED;
    TEST_POP_DEFERRED;
}
TEST_ENDDEF


TEST_DEF_NEED_CHGROUP(connect_and_receive_over_channel_in_group_and_single_channel_simultaneously)
{
    static pubnub_t* pbp;
    static pubnub_t* pbp_2;
    char* const      chgrp = pnfntst_make_name(this_test_name_);
    TEST_DEFER(free, chgrp);
    pbp = pnfntst_create_ctx();
    TEST_DEFER(pnfntst_free, pbp);
    pbp_2 = pnfntst_create_ctx();
    TEST_DEFER(pnfntst_free, pbp_2);
    pubnub_set_non_blocking_io(pbp_2);

    expect_PNR_OK(pbp, pubnub_remove_channel_group(pbp, chgrp), 10 * SECONDS);
    expect_PNR_OK(pbp, pubnub_add_channel_to_group(pbp, "ch", chgrp), 10 * SECONDS);

    TEST_SLEEP_FOR(CHANNEL_REGISTRY_PROPAGATION_DELAY);

    expect_PNR_OK(pbp_2, pubnub_subscribe(pbp_2, "two", chgrp), 10 * SECONDS);
    expect_PNR_OK(pbp, pubnub_publish(pbp, "ch", "\"Test M7\""), 10 * SECONDS);
    expect_PNR_OK(pbp, pubnub_publish(pbp, "two", "\"Test M7-2\""), 10 * SECONDS);
    expect(pnfntst_subscribe_and_check(
        pbp_2, "two", chgrp, 10 * SECONDS, "\"Test M7\"", "ch", "\"Test M7-2\"", "two", NULL));

    expect_PNR_OK(pbp, pubnub_remove_channel_group(pbp, chgrp), 10 * SECONDS);

    TEST_POP_DEFERRED;
    TEST_POP_DEFERRED;
    TEST_POP_DEFERRED;
}
TEST_ENDDEF


TEST_DEF_NEED_CHGROUP(connect_and_receive_over_channel_in_group_and_multi_channel_simultaneously)
{
    static pubnub_t* pbp;
    static pubnub_t* pbp_2;
    char* const      chgrp = pnfntst_make_name(this_test_name_);
    TEST_DEFER(free, chgrp);
    pbp = pnfntst_create_ctx();
    TEST_DEFER(pnfntst_free, pbp);
    pbp_2 = pnfntst_create_ctx();
    TEST_DEFER(pnfntst_free, pbp_2);
    pubnub_set_non_blocking_io(pbp_2);

    expect_PNR_OK(pbp_2, pubnub_remove_channel_group(pbp_2, chgrp), 10 * SECONDS);
    expect_PNR_OK(
        pbp_2, pubnub_add_channel_to_group(pbp_2, "three", chgrp), 10 * SECONDS);

    TEST_SLEEP_FOR(CHANNEL_REGISTRY_PROPAGATION_DELAY);

    expect_PNR_OK(pbp_2, pubnub_subscribe(pbp_2, "ch,two", chgrp), 10 * SECONDS);
    expect_PNR_OK(pbp, pubnub_publish(pbp, "ch", "\"Test M8\""), 10 * SECONDS);
    expect_PNR_OK(pbp, pubnub_publish(pbp, "two", "\"Test M8-2\""), 10 * SECONDS);
    expect_PNR_OK(pbp, pubnub_publish(pbp, "three", "\"Test M8-3\""), 10 * SECONDS);
    expect(pnfntst_subscribe_and_check(pbp_2,
                                       "ch,two",
                                       chgrp,
                                       10 * SECONDS,
                                       "\"Test M8\"",
                                       "ch",
                                       "\"Test M8-2\"",
                                       "two",
                                       "\"Test M8-3\"",
                                       "three",
                                       NULL));

    expect_PNR_OK(pbp_2, pubnub_remove_channel_group(pbp_2, chgrp), 10 * SECONDS);

    TEST_POP_DEFERRED;
    TEST_POP_DEFERRED;
    TEST_POP_DEFERRED;
}
TEST_ENDDEF


TEST_DEF(broken_connection_test)
{
    static pubnub_t*  pbp;
    char const* const chan = this_test_name_;
    pbp                    = pnfntst_create_ctx();
    TEST_DEFER(pnfntst_free, pbp);

    expect_PNR_OK(pbp, pubnub_subscribe(pbp, chan, NULL), 12 * SECONDS);
    expect_PNR_OK(pbp, pubnub_publish(pbp, chan, "\"Test 3\""), 12 * SECONDS);
    expect_PNR_OK(pbp, pubnub_subscribe(pbp, chan, NULL), 12 * SECONDS);
    expect(pnfntst_got_messages(pbp, "\"Test 6\"", NULL));

    expect_PNR_OK(pbp, pubnub_publish(pbp, chan, "\"Test 6 - 2\""), 12 * SECONDS);

    printf("Please disconnect from Internet. Press Enter when done.");
    await_console();

    expect_pnr(pubnub_subscribe(pbp, chan, NULL), PNR_STARTED);
    await_timed(12 * SECONDS, PNR_ADDR_RESOLUTION_FAILED, pbp);

    printf("Please reconnect to Internet. Press Enter when done.");
    await_console();

    expect_PNR_OK(pbp, pubnub_subscribe(pbp, chan, NULL), 12 * SECONDS);
    expect(pnfntst_got_messages(pbp, "\"Test 6 - 2\"", NULL));

    printf("Please disconnect from Internet. Press Enter when done.");
    await_console();

    expect_pnr(pubnub_publish(pbp, chan, "\"Test 6 - 3\""), PNR_STARTED);
    await_timed(12 * SECONDS, PNR_ADDR_RESOLUTION_FAILED, pbp);

    printf("Please reconnect to Internet. Press Enter when done.");
    await_console();

    expect_PNR_OK(pbp, pubnub_publish(pbp, chan, "\"Test 6-4\""), 12 * SECONDS);
    expect_PNR_OK(pbp, pubnub_subscribe(pbp, chan, NULL), 12 * SECONDS);
    expect(pnfntst_got_messages(pbp, "\"Test 6-4\"", NULL));

    TEST_POP_DEFERRED;
}
TEST_ENDDEF


TEST_DEF(broken_connection_test_multi)
{
    static pubnub_t* pbp;
    pbp = pnfntst_create_ctx();
    TEST_DEFER(pnfntst_free, pbp);

    expect_PNR_OK(pbp, pubnub_subscribe(pbp, "ch,two", NULL), 12 * SECONDS);
    expect_PNR_OK(pbp, pubnub_publish(pbp, "ch", "\"Test 7\""), 12 * SECONDS);
    expect_PNR_OK(pbp, pubnub_publish(pbp, "two", "\"Test 8\""), 12 * SECONDS);
    expect_PNR_OK(pbp, pubnub_subscribe(pbp, "ch,two", NULL), 12 * SECONDS);
    expect(pnfntst_got_messages(pbp, "\"Test 7\"", "\"Test 8\"", NULL));

    expect_PNR_OK(pbp, pubnub_publish(pbp, "ch", "\"Test 7 - 2\""), 12 * SECONDS);
    expect_PNR_OK(pbp, pubnub_publish(pbp, "two", "\"Test 8 - 2\""), 12 * SECONDS);

    printf("Test %s: Please disconnect from Internet. Press Enter when done.",
           this_test_name_);
    await_console();

    expect_pnr(pubnub_subscribe(pbp, "ch,two", NULL), PNR_STARTED);
    await_timed(12 * SECONDS, PNR_ADDR_RESOLUTION_FAILED, pbp);

    printf("Test %s: Please reconnect to Internet. Press Enter when done.",
           this_test_name_);
    await_console();

    expect_PNR_OK(pbp, pubnub_subscribe(pbp, "ch,two", NULL), 12 * SECONDS);
    expect(pnfntst_got_messages(pbp, "\"Test 7 - 2\"", "\"Test 8 - 2\"", NULL));

    expect_PNR_OK(pbp, pubnub_publish(pbp, "ch", "\"Test 7 - 4\""), 12 * SECONDS);
    expect_PNR_OK(pbp, pubnub_publish(pbp, "two", "\"Test 8 - 4\""), 12 * SECONDS);
    expect_PNR_OK(pbp, pubnub_subscribe(pbp, "ch,two", NULL), 12 * SECONDS);
    expect(pnfntst_got_messages(pbp, "\"Test 7 - 4\"", "\"Test 8 - 4\"", NULL));

    TEST_POP_DEFERRED;
}
TEST_ENDDEF


TEST_DEF_NEED_CHGROUP(broken_connection_test_group)
{
    static pubnub_t* pbp;
    enum pubnub_res  rslt;
    pbp = pnfntst_create_ctx();
    TEST_DEFER(pnfntst_free, pbp);

    expect_pnr(pubnub_remove_channel_group(pbp, this_test_name_), PNR_STARTED);
    await_timed(10 * SECONDS, PNR_OK, pbp);
    rslt = pubnub_add_channel_to_group(pbp, "ch", this_test_name_);
    expect_pnr_maybe_started(rslt, pbp, 10 * SECONDS, PNR_OK);

    TEST_SLEEP_FOR(CHANNEL_REGISTRY_PROPAGATION_DELAY);

    rslt = pubnub_subscribe(pbp, NULL, this_test_name_);
    expect_pnr_maybe_started(rslt, pbp, 12 * SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "ch", "\"Test 9\"");
    expect_pnr_maybe_started(rslt, pbp, 12 * SECONDS, PNR_OK);

    rslt = pubnub_subscribe(pbp, NULL, this_test_name_);
    expect_pnr_maybe_started(rslt, pbp, 12 * SECONDS, PNR_OK);

    expect(pnfntst_got_messages(pbp, "\"Test 9\"", NULL));

    rslt = pubnub_publish(pbp, "ch", "\"Test 9 - 2\"");
    expect_pnr_maybe_started(rslt, pbp, 12 * SECONDS, PNR_OK);

    printf("Please disconnect from Internet. Press Enter when done.");
    await_console();
    expect_pnr(pubnub_subscribe(pbp, NULL, this_test_name_), PNR_STARTED);
    await_timed(12 * SECONDS, PNR_ADDR_RESOLUTION_FAILED, pbp);
    printf("Please reconnect to Internet. Press Enter when done.");
    await_console();
    expect_pnr(pubnub_subscribe(pbp, NULL, this_test_name_), PNR_STARTED);
    await_timed(12 * SECONDS, PNR_OK, pbp);
    expect(pnfntst_got_messages(pbp, "\"Test 9 - 2\"", NULL));

    printf("Please disconnect from Internet. Press Enter when done.");
    await_console();
    expect_pnr(pubnub_publish(pbp, "ch", "\"Test 9 - 3\""), PNR_STARTED);
    await_timed(12 * SECONDS, PNR_ADDR_RESOLUTION_FAILED, pbp);

    printf("Please reconnect to Internet. Press Enter when done.");
    await_console();
    expect_pnr(pubnub_publish(pbp, "ch", "\"Test 9 - 4\""), PNR_STARTED);
    await_timed(12 * SECONDS, PNR_OK, pbp);
    rslt = pubnub_subscribe(pbp, NULL, this_test_name_);
    expect_pnr_maybe_started(rslt, pbp, 12 * SECONDS, PNR_OK);

    expect(pnfntst_got_messages(pbp, "\"Test 9 - 4\"", NULL));

    rslt = pubnub_remove_channel_from_group(pbp, "ch", this_test_name_);
    expect_pnr_maybe_started(rslt, pbp, 10 * SECONDS, PNR_OK);

    TEST_POP_DEFERRED;
}
TEST_ENDDEF


TEST_DEF_NEED_CHGROUP(broken_connection_test_multi_in_group)
{
    static pubnub_t* pbp;
    enum pubnub_res  rslt;
    pbp = pnfntst_create_ctx();
    TEST_DEFER(pnfntst_free, pbp);

    expect_pnr(pubnub_remove_channel_group(pbp, this_test_name_), PNR_STARTED);
    await_timed(10 * SECONDS, PNR_OK, pbp);
    rslt = pubnub_add_channel_to_group(pbp, "ch,two", this_test_name_);
    expect_pnr_maybe_started(rslt, pbp, 10 * SECONDS, PNR_OK);

    TEST_SLEEP_FOR(CHANNEL_REGISTRY_PROPAGATION_DELAY);

    rslt = pubnub_subscribe(pbp, "ch,two", NULL);
    expect_pnr_maybe_started(rslt, pbp, 12 * SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "ch", "\"Test A\"");
    expect_pnr_maybe_started(rslt, pbp, 12 * SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "two", "\"Test B\"");
    expect_pnr_maybe_started(rslt, pbp, 12 * SECONDS, PNR_OK);

    rslt = pubnub_subscribe(pbp, "ch,two", NULL);
    expect_pnr_maybe_started(rslt, pbp, 12 * SECONDS, PNR_OK);

    expect(pnfntst_got_messages(pbp, "\"Test A\"", "\"Test B\"", NULL));

    rslt = pubnub_publish(pbp, "ch", "\"Test A - 2\"");
    expect_pnr_maybe_started(rslt, pbp, 12 * SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "two", "\"Test B - 2\"");
    expect_pnr_maybe_started(rslt, pbp, 12 * SECONDS, PNR_OK);

    printf("Please disconnect from Internet. Press Enter when done.");
    await_console();
    expect_pnr(pubnub_subscribe(pbp, "ch,two", NULL), PNR_STARTED);
    await_timed(12 * SECONDS, PNR_ADDR_RESOLUTION_FAILED, pbp);
    printf("Please reconnect to Internet. Press Enter when done.");
    await_console();
    expect_pnr(pubnub_subscribe(pbp, "ch,two", NULL), PNR_STARTED);
    await_timed(12 * SECONDS, PNR_OK, pbp);
    expect(pnfntst_got_messages(pbp, "\"Test A - 2\"", "\"Test B - 2\"", NULL));

    rslt = pubnub_publish(pbp, "ch", "\"Test A - 4\"");
    expect_pnr_maybe_started(rslt, pbp, 12 * SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "two", "\"Test B - 4\"");
    expect_pnr_maybe_started(rslt, pbp, 12 * SECONDS, PNR_OK);

    rslt = pubnub_subscribe(pbp, "ch,two", NULL);
    expect_pnr_maybe_started(rslt, pbp, 12 * SECONDS, PNR_OK);

    expect(pnfntst_got_messages(pbp, "\"Test A - 4\"", "\"Test B - 4\"", NULL));

    rslt = pubnub_remove_channel_from_group(pbp, "ch,two", this_test_name_);
    expect_pnr_maybe_started(rslt, pbp, 10 * SECONDS, PNR_OK);

    TEST_POP_DEFERRED;
}
TEST_ENDDEF


TEST_DEF_NEED_CHGROUP(broken_connection_test_group_in_group_out)
{
    static pubnub_t* pbp;
    enum pubnub_res  rslt;
    pbp = pnfntst_create_ctx();
    TEST_DEFER(pnfntst_free, pbp);

    expect_pnr(pubnub_remove_channel_group(pbp, this_test_name_), PNR_STARTED);
    await_timed(10 * SECONDS, PNR_OK, pbp);
    rslt = pubnub_add_channel_to_group(pbp, "ch", this_test_name_);
    expect_pnr_maybe_started(rslt, pbp, 10 * SECONDS, PNR_OK);

    TEST_SLEEP_FOR(CHANNEL_REGISTRY_PROPAGATION_DELAY);

    rslt = pubnub_subscribe(pbp, "two", this_test_name_);
    expect_pnr_maybe_started(rslt, pbp, 12 * SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "ch", "\"Test C\"");
    expect_pnr_maybe_started(rslt, pbp, 12 * SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "two", "\"Test D\"");
    expect_pnr_maybe_started(rslt, pbp, 12 * SECONDS, PNR_OK);

    rslt = pubnub_subscribe(pbp, "two", this_test_name_);
    expect_pnr_maybe_started(rslt, pbp, 12 * SECONDS, PNR_OK);

    expect(pnfntst_got_messages(pbp, "\"Test C\"", "\"Test D\"", NULL));

    rslt = pubnub_publish(pbp, "ch", "\"Test C - 2\"");
    expect_pnr_maybe_started(rslt, pbp, 12 * SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "two", "\"Test D - 2\"");
    expect_pnr_maybe_started(rslt, pbp, 12 * SECONDS, PNR_OK);

    printf("Please disconnect from Internet. Press Enter when done.");
    await_console();
    expect_pnr(pubnub_subscribe(pbp, "two", this_test_name_), PNR_STARTED);
    await_timed(12 * SECONDS, PNR_ADDR_RESOLUTION_FAILED, pbp);

    printf("Please reconnect to Internet. Press Enter when done.");
    await_console();
    expect_pnr(pubnub_subscribe(pbp, "two", this_test_name_), PNR_STARTED);
    await_timed(12 * SECONDS, PNR_OK, pbp);
    expect(pnfntst_got_messages(pbp, "\"Test C - 2\"", "\"Test D - 2\"", NULL));

    rslt = pubnub_publish(pbp, "ch", "\"Test C - 4\"");
    expect_pnr_maybe_started(rslt, pbp, 12 * SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "two", "\"Test D - 4\"");
    expect_pnr_maybe_started(rslt, pbp, 12 * SECONDS, PNR_OK);

    rslt = pubnub_subscribe(pbp, "two", this_test_name_);
    expect_pnr_maybe_started(rslt, pbp, 12 * SECONDS, PNR_OK);

    expect(pnfntst_got_messages(pbp, "\"Test C - 4\"", "\"Test D - 4\"", NULL));

    rslt = pubnub_remove_channel_from_group(pbp, "ch", this_test_name_);
    expect_pnr_maybe_started(rslt, pbp, 12 * SECONDS, PNR_OK);

    TEST_POP_DEFERRED;
}
TEST_ENDDEF


TEST_DEF_NEED_CHGROUP(broken_connection_test_group_multichannel_out)
{
    static pubnub_t* pbp;
    enum pubnub_res  rslt;
    pbp = pnfntst_create_ctx();
    TEST_DEFER(pnfntst_free, pbp);

    expect_pnr(pubnub_remove_channel_group(pbp, this_test_name_), PNR_STARTED);
    await_timed(10 * SECONDS, PNR_OK, pbp);
    rslt = pubnub_add_channel_to_group(pbp, "three", this_test_name_);
    expect_pnr_maybe_started(rslt, pbp, 10 * SECONDS, PNR_OK);

    TEST_SLEEP_FOR(CHANNEL_REGISTRY_PROPAGATION_DELAY);

    rslt = pubnub_subscribe(pbp, "ch,two", this_test_name_);
    expect_pnr_maybe_started(rslt, pbp, 12 * SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "ch", "\"Test E\"");
    expect_pnr_maybe_started(rslt, pbp, 12 * SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "two", "\"Test F\"");
    expect_pnr_maybe_started(rslt, pbp, 12 * SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "three", "\"Test G\"");
    expect_pnr_maybe_started(rslt, pbp, 12 * SECONDS, PNR_OK);

    rslt = pubnub_subscribe(pbp, "ch,two", this_test_name_);
    expect_pnr_maybe_started(rslt, pbp, 12 * SECONDS, PNR_OK);

    expect(pnfntst_got_messages(pbp, "\"Test E\"", "\"Test F\"", "\"Test G\"", NULL));

    rslt = pubnub_publish(pbp, "ch", "\"Test E - 2\"");
    expect_pnr_maybe_started(rslt, pbp, 12 * SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "two", "\"Test F - 2\"");
    expect_pnr_maybe_started(rslt, pbp, 12 * SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "three", "\"Test G - 2\"");
    expect_pnr_maybe_started(rslt, pbp, 12 * SECONDS, PNR_OK);

    printf("Please disconnect from Internet. Press Enter when done.");
    await_console();
    expect_pnr(pubnub_subscribe(pbp, "ch,two", this_test_name_), PNR_STARTED);
    await_timed(12 * SECONDS, PNR_ADDR_RESOLUTION_FAILED, pbp);
    printf("Please reconnect to Internet. Press Enter when done.");
    await_console();
    expect_pnr(pubnub_subscribe(pbp, "ch,two", this_test_name_), PNR_STARTED);
    await_timed(12 * SECONDS, PNR_OK, pbp);
    expect(pnfntst_got_messages(
        pbp, "\"Test E - 2\"", "\"Test F - 2\"", "\"Test G - 2\"", NULL));

    rslt = pubnub_publish(pbp, "ch", "\"Test E - 4\"");
    expect_pnr_maybe_started(rslt, pbp, 12 * SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "two", "\"Test F - 4\"");
    expect_pnr_maybe_started(rslt, pbp, 12 * SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "three", "\"Test G - 4\"");
    expect_pnr_maybe_started(rslt, pbp, 12 * SECONDS, PNR_OK);

    rslt = pubnub_subscribe(pbp, "ch,two", this_test_name_);
    expect_pnr_maybe_started(rslt, pbp, 12 * SECONDS, PNR_OK);

    expect(pnfntst_got_messages(
        pbp, "\"Test E - 4\"", "\"Test F - 4\"", "\"Test G - 4\"", NULL));

    rslt = pubnub_remove_channel_from_group(pbp, "three", this_test_name_);
    expect_pnr_maybe_started(rslt, pbp, 10 * SECONDS, PNR_OK);

    TEST_POP_DEFERRED;
}
TEST_ENDDEF
