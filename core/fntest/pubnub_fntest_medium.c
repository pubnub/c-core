/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_sync.h"

#include "pubnub_fntest.h"
#include "core/pubnub_blocking_io.h"
#include "core/pubnub_log.h"

#include <stdlib.h>

#define SECONDS 1000
#define CHANNEL_REGISTRY_PROPAGATION_DELAY 1000


TEST_DEF(complex_send_and_receive_over_several_channels_simultaneously)
{
    static pubnub_t* pbp;
    static pubnub_t* pbp_2;
    enum pubnub_res  rslt;
    pbp = pnfntst_create_ctx();
    TEST_DEFER(pnfntst_free, pbp);
    pbp_2 = pnfntst_create_ctx();
    TEST_DEFER(pnfntst_free, pbp_2);
    pubnub_set_non_blocking_io(pbp_2);

    expect_pnr(pubnub_subscribe(pbp, "two,three", NULL), PNR_STARTED);
    expect_pnr(pubnub_subscribe(pbp_2, "this_test_name_", NULL), PNR_STARTED);
    await_timed_2(10 * SECONDS, PNR_OK, pbp, PNR_OK, pbp_2);

    expect_pnr(pubnub_publish(pbp_2, "two", "\"Test M3\""), PNR_STARTED);
    await_timed(10 * SECONDS, PNR_OK, pbp_2);
    rslt = pubnub_publish(pbp, this_test_name_, "\"Test M3\"");
    expect_pnr(pubnub_publish(pbp_2, "three", "\"Test M3 - 1\""), PNR_STARTED);
    expect_pnr_maybe_started_2(
        rslt, PNR_STARTED, 10 * SECONDS, pbp, PNR_OK, pbp_2, PNR_OK);

    rslt = pubnub_subscribe(pbp, "two,three", NULL);
    expect_pnr(pubnub_subscribe(pbp_2, this_test_name_, NULL), PNR_STARTED);
    expect_pnr_maybe_started_2(
        rslt, PNR_STARTED, 10 * SECONDS, pbp, PNR_OK, pbp_2, PNR_OK);

    expect(pnfntst_got_messages(pbp, "\"Test M3\"", "\"Test M3 - 1\"", NULL));
    expect(pnfntst_got_messages(pbp_2, "\"Test M3\"", NULL));

    TEST_POP_DEFERRED;
    TEST_POP_DEFERRED;
}
TEST_ENDDEF


TEST_DEF_NEED_CHGROUP(complex_send_and_receive_over_channel_plus_group_simultaneously)
{
    static pubnub_t* pbp;
    static pubnub_t* pbp_2;
    enum pubnub_res  rslt;
    enum pubnub_res  rslt_2;
    char* const      chgrp = pnfntst_make_name(this_test_name_);
    int              tries = 0;
    TEST_DEFER(free, chgrp);
    pbp = pnfntst_create_ctx();
    TEST_DEFER(pnfntst_free, pbp);
    pbp_2 = pnfntst_create_ctx();
    TEST_DEFER(pnfntst_free, pbp_2);

    expect_PNR_OK(pbp, pubnub_remove_channel_group(pbp, chgrp), 10 * SECONDS);
    expect_PNR_OK(
        pbp, pubnub_add_channel_to_group(pbp, "two,three", chgrp), 10 * SECONDS);

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
    expect_PNR_OK(pbp_2, pubnub_subscribe(pbp_2, "ch", NULL), 10 * SECONDS);
    expect_PNR_OK(pbp, pubnub_publish(pbp, "two", "\"Test M4 - two\""), 10 * SECONDS);
    rslt_2 = pubnub_publish(pbp_2, "ch", "\"Test M4\"");
    rslt   = pubnub_publish(pbp, "three", "\"Test M4 - three\"");
    expect_pnr_maybe_started_2(rslt, rslt_2, 10 * SECONDS, pbp, PNR_OK, pbp_2, PNR_OK);

    rslt   = pubnub_subscribe(pbp, NULL, chgrp);
    rslt_2 = pubnub_subscribe(pbp_2, "ch", NULL);
    expect_pnr_maybe_started_2(rslt, rslt_2, 10 * SECONDS, pbp, PNR_OK, pbp_2, PNR_OK);

    expect(pnfntst_got_messages(
        pbp, "\"Test M4 - two\"", "\"Test M4 - three\"", NULL));
    expect(pnfntst_got_messages(pbp_2, "\"Test M4\"", NULL));

    expect_PNR_OK(pbp, pubnub_remove_channel_group(pbp, chgrp), 10 * SECONDS);

    TEST_POP_DEFERRED;
    TEST_POP_DEFERRED;
    TEST_POP_DEFERRED;
}
TEST_ENDDEF


TEST_DEF(connect_disconnect_and_connect_again)
{
    static pubnub_t* pbp;
    enum pubnub_res  rslt;
    pbp = pnfntst_create_ctx();
    TEST_DEFER(pnfntst_free, pbp);
    pubnub_set_non_blocking_io(pbp);

    expect_pnr(pubnub_subscribe(pbp, this_test_name_, NULL), PNR_STARTED);
    await_timed(10 * SECONDS, PNR_OK, pbp);

    rslt = pubnub_publish(pbp, this_test_name_, "\"Test M5\"");
    expect_pnr_maybe_started(rslt, pbp, 10 * SECONDS, PNR_OK) pubnub_cancel(pbp);
    await_timed(10 * SECONDS, PNR_CANCELLED, pbp);

    rslt = pubnub_publish(pbp, this_test_name_, "\"Test M5 - 2\"");
    expect_pnr_maybe_started(rslt, pbp, 10 * SECONDS, PNR_OK);
    rslt = pubnub_subscribe(pbp, this_test_name_, NULL);
    expect_pnr_maybe_started(rslt, pbp, 10 * SECONDS, PNR_OK);

    expect(pnfntst_got_messages(pbp, "\"Test M5 - 2\"", NULL));

    expect_pnr(pubnub_subscribe(pbp, this_test_name_, NULL), PNR_STARTED);
    pubnub_cancel(pbp);
    await_timed(10 * SECONDS, PNR_CANCELLED, pbp);
    expect_pnr(pubnub_publish(pbp, this_test_name_, "\"Test M5 - 3\""), PNR_STARTED);
    await_timed(10 * SECONDS, PNR_OK, pbp);

    expect_pnr(pubnub_subscribe(pbp, this_test_name_, NULL), PNR_STARTED);
    await_timed(10 * SECONDS, PNR_OK, pbp);
    expect(pnfntst_got_messages(pbp, "\"Test M5 - 3\"", NULL));

    TEST_POP_DEFERRED;
}
TEST_ENDDEF


TEST_DEF_NEED_CHGROUP(connect_disconnect_and_connect_again_group)
{

    static pubnub_t* pbp;
    enum pubnub_res  rslt;
    char* const      chgrp = pnfntst_make_name(this_test_name_);
    int              tries = 0;
    pbp = pnfntst_create_ctx();
    TEST_DEFER(pnfntst_free, pbp);
    pubnub_set_non_blocking_io(pbp);

    expect_pnr(pubnub_remove_channel_group(pbp, chgrp), PNR_STARTED);
    await_timed(10 * SECONDS, PNR_OK, pbp);
    expect_pnr(pubnub_add_channel_to_group(pbp, "ch", chgrp), PNR_STARTED);
    await_timed(10 * SECONDS, PNR_OK, pbp);

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
//    expect_pnr_maybe_started(rslt, pbp, 10 * SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "ch", "\"Test M6\"");
    expect_pnr_maybe_started(rslt, pbp, 10 * SECONDS, PNR_OK);
    pubnub_cancel(pbp);
    await_timed(10 * SECONDS, PNR_CANCELLED, pbp);

    rslt = pubnub_publish(pbp, "ch", "\"Test M6 - 2\"");
    expect_pnr_maybe_started(rslt, pbp, 10 * SECONDS, PNR_OK);
    rslt = pubnub_subscribe(pbp, NULL, chgrp);
    expect_pnr_maybe_started(rslt, pbp, 10 * SECONDS, PNR_OK);

    expect(pnfntst_got_messages(pbp, "\"Test M6 - 2\"", NULL));
    
    expect_pnr(pubnub_subscribe(pbp, NULL, chgrp), PNR_STARTED);
    pubnub_cancel(pbp);
    await_timed(10 * SECONDS, PNR_CANCELLED, pbp);

    expect_pnr(pubnub_publish(pbp, "ch", "\"Test M6 - 3\""), PNR_STARTED);
    await_timed(10 * SECONDS, PNR_OK, pbp);
    expect_pnr(pubnub_subscribe(pbp, NULL, chgrp), PNR_STARTED);
    await_timed(10 * SECONDS, PNR_OK, pbp);
    expect(pnfntst_got_messages(pbp, "\"Test M6 - 3\"", NULL));

    expect_pnr(pubnub_remove_channel_from_group(pbp, "ch", chgrp),
               PNR_STARTED);
    await_timed(10 * SECONDS, PNR_OK, pbp);

    TEST_POP_DEFERRED;
}
TEST_ENDDEF


TEST_DEF_NEED_CHGROUP(connect_disconnect_and_connect_again_combo)
{

    static pubnub_t* pbp;
    static pubnub_t* pbp_2;
    enum pubnub_res  rslt;
    char* const      chgrp = pnfntst_make_name(this_test_name_);
    int              tries = 0;
    pbp = pnfntst_create_ctx();
    TEST_DEFER(pnfntst_free, pbp);
    pbp_2 = pnfntst_create_ctx();
    TEST_DEFER(pnfntst_free, pbp_2);
    pubnub_set_non_blocking_io(pbp);
    pubnub_set_non_blocking_io(pbp_2);

    expect_pnr(pubnub_remove_channel_group(pbp, chgrp), PNR_STARTED);
    await_timed(10 * SECONDS, PNR_OK, pbp);
    rslt = pubnub_add_channel_to_group(pbp, "ch", chgrp);
    expect_pnr_maybe_started(rslt, pbp, 10 * SECONDS, PNR_OK);

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
//    rslt = pubnub_subscribe(pbp, NULL, chgrp);
//    expect_pnr_maybe_started(rslt, pbp, 10 * SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "ch", "\"Test M8\"");
    if ((rslt != PNR_OK) && (rslt != PNR_STARTED)) {
        expect_pnr(rslt, PNR_STARTED);
    }
    pubnub_cancel(pbp);
    rslt = pubnub_publish(pbp_2, "two", "\"Test M9\"");
    if ((rslt != PNR_OK) && (rslt != PNR_STARTED)) {
        expect_pnr(rslt, PNR_STARTED);
    }
    pubnub_cancel(pbp_2);
    await_timed_2(10 * SECONDS, PNR_CANCELLED, pbp, PNR_CANCELLED, pbp_2);

    expect_pnr(pubnub_publish(pbp, "ch", "\"Test M8 - 2\""), PNR_STARTED);
    expect_pnr(pubnub_publish(pbp_2, "two", "\"Test M9 - 2\""), PNR_STARTED);
    await_timed_2(10 * SECONDS, PNR_OK, pbp, PNR_OK, pbp_2);
    rslt = pubnub_subscribe(pbp, "two", chgrp);
    expect_pnr_maybe_started(rslt, pbp, 10 * SECONDS, PNR_OK);
    expect(pnfntst_got_messages(pbp, "\"Test M8 - 2\"", "\"Test M9 - 2\"", NULL));

    expect_pnr(pubnub_subscribe(pbp, "two", chgrp), PNR_STARTED);
    pubnub_cancel(pbp);
    await_timed(10 * SECONDS, PNR_CANCELLED, pbp);
    expect_pnr(pubnub_publish(pbp, "ch", "\"Test M8 - 3\""), PNR_STARTED);
    await_timed(10 * SECONDS, PNR_OK, pbp);
    expect_pnr(pubnub_publish(pbp, "two", "\"Test M9 - 3\""), PNR_STARTED);
    await_timed(10 * SECONDS, PNR_OK, pbp);
    expect_pnr(pubnub_subscribe(pbp, "two", chgrp), PNR_STARTED);
    await_timed(10 * SECONDS, PNR_OK, pbp);
    expect(pnfntst_got_messages(pbp, "\"Test M8 - 3\"", "\"Test M9 - 3\"", NULL));

    expect_pnr(pubnub_remove_channel_from_group(pbp, "ch", chgrp),
               PNR_STARTED);
    await_timed(10 * SECONDS, PNR_OK, pbp);

    TEST_POP_DEFERRED;
    TEST_POP_DEFERRED;
}
TEST_ENDDEF


TEST_DEF(wrong_api_usage)
{
    static pubnub_t* pbp;
    pbp = pnfntst_create_ctx();
    TEST_DEFER(pnfntst_free, pbp);
    pubnub_set_non_blocking_io(pbp);

    expect_pnr(pubnub_subscribe(pbp, this_test_name_, NULL), PNR_STARTED);
    expect_pnr(pubnub_publish(pbp, this_test_name_, "\"Test - 2\""),
               PNR_IN_PROGRESS);
    expect_pnr(pubnub_subscribe(pbp, this_test_name_, NULL), PNR_IN_PROGRESS);
    await_timed(10 * SECONDS, PNR_OK, pbp);

    expect_pnr(pubnub_subscribe(pbp, this_test_name_, NULL), PNR_STARTED);
    expect_pnr(pubnub_subscribe(pbp, this_test_name_, NULL), PNR_IN_PROGRESS);
    expect_pnr(pubnub_publish(pbp, this_test_name_, "\"Test - 2\""),
               PNR_IN_PROGRESS);

    pubnub_cancel(pbp);
    await_timed(10 * SECONDS, PNR_CANCELLED, pbp);

#if !(PUBNUB_USE_AUTO_HEARTBEAT)
    expect_pnr(pubnub_subscribe(pbp, NULL, NULL), PNR_INVALID_CHANNEL);
#endif

    TEST_POP_DEFERRED;
}
TEST_ENDDEF


TEST_DEF(handling_errors_from_pubnub)
{
    static pubnub_t* pbp;
    enum pubnub_res  rslt;
    pbp = pnfntst_create_ctx();
    TEST_DEFER(pnfntst_free, pbp);

    expect_pnr(pubnub_publish(pbp, this_test_name_, "\"Test "), PNR_STARTED);
    await_timed(10 * SECONDS, PNR_PUBLISH_FAILED, pbp);
    expect(400 == pubnub_last_http_code(pbp));
    expect(pubnub_parse_publish_result(pubnub_last_publish_result(pbp))
           == PNPUB_INVALID_JSON);

    rslt = pubnub_publish(pbp, ",", "\"Test \"");
    expect_pnr_maybe_started(rslt, pbp, 10 * SECONDS, PNR_PUBLISH_FAILED);

    expect(400 == pubnub_last_http_code(pbp));
    expect(pubnub_parse_publish_result(pubnub_last_publish_result(pbp))
           == PNPUB_INVALID_CHAR_IN_CHAN_NAME);

    TEST_POP_DEFERRED;
}
TEST_ENDDEF
