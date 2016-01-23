/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_sync.h"

#include "pubnub_fntest.h"

#include "pubnub_blocking_io.h"

#define SECONDS 1000
#define CHANNEL_REGISTRY_PROPAGATION_DELAY 800


TEST_DEF(complex_send_and_receive_over_several_channels_simultaneously) {

    static pubnub_t *pbp;
    static pubnub_t *pbp_2;
    pbp = pubnub_alloc();
    TEST_DEFER(pubnub_free, pbp);
    pubnub_init(pbp, g_pubkey, g_keysub);
    pubnub_origin_set(pbp, g_origin);
    pbp_2 = pubnub_alloc();
    TEST_DEFER(pubnub_free, pbp_2);
    pubnub_init(pbp_2, g_pubkey, g_keysub);
    pubnub_origin_set(pbp_2, g_origin);

    expect_pnr(pubnub_subscribe(pbp, "two,three", NULL), PNR_STARTED);
    expect_pnr(pubnub_subscribe(pbp_2, "ch", NULL), PNR_STARTED);
    await_timed_2(5*SECONDS, PNR_OK, pbp, PNR_OK, pbp_2);

    pubnub_set_non_blocking_io(pbp_2);
    expect_pnr(pubnub_publish(pbp_2, "two", "\"Test M3\""), PNR_STARTED);
    await_timed(5*SECONDS,PNR_OK, pbp_2);
    expect_pnr(pubnub_publish(pbp, "ch", "\"Test M3\""), PNR_STARTED);
    expect_pnr(pubnub_publish(pbp_2, "three", "\"Test M3 - 1\""), PNR_STARTED);
    await_timed_2(5*SECONDS, PNR_OK, pbp, PNR_OK, pbp_2);

    expect_pnr(pubnub_subscribe(pbp, "two,three", NULL), PNR_STARTED);
    expect_pnr(pubnub_subscribe(pbp_2, "ch", NULL), PNR_STARTED);
    await_timed_2(5*SECONDS, PNR_OK, pbp, PNR_OK, pbp_2);

    expect(pnfntst_got_messages(pbp, "\"Test M3\"", "\"Test M3 - 1\"", NULL));
    expect(pnfntst_got_messages(pbp_2, "\"Test M3\"", NULL));

    TEST_POP_DEFERRED;
    TEST_POP_DEFERRED;
    } TEST_ENDDEF


TEST_DEF(complex_send_and_receive_over_channel_plus_group_simultaneously) {

    static pubnub_t *pbp;
    static pubnub_t *pbp_2;
    pbp = pubnub_alloc();
    TEST_DEFER(pubnub_free, pbp);
    pubnub_init(pbp, g_pubkey, g_keysub);
    pubnub_origin_set(pbp, g_origin);
    pbp_2 = pubnub_alloc();
    TEST_DEFER(pubnub_free, pbp_2);
    pubnub_init(pbp_2, g_pubkey, g_keysub);
    pubnub_origin_set(pbp_2, g_origin);

    expect_pnr(pubnub_remove_channel_group(pbp, "gr"), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp);
    expect_pnr(pubnub_add_channel_to_group(pbp, "two,three", "gr"), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp);

    TEST_SLEEP_FOR(CHANNEL_REGISTRY_PROPAGATION_DELAY);
    
    expect_pnr(pubnub_subscribe(pbp, NULL, "gr"), PNR_STARTED);
    expect_pnr(pubnub_subscribe(pbp_2, "ch", NULL), PNR_STARTED);
    await_timed_2(5*SECONDS, PNR_OK, pbp, PNR_OK, pbp_2);

    expect_pnr(pubnub_publish(pbp, "two", "\"Test M3\""), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp);
    expect_pnr(pubnub_publish(pbp_2, "ch", "\"Test M3\""), PNR_STARTED);
    expect_pnr(pubnub_publish(pbp, "three", "\"Test M3 - 1\""), PNR_STARTED);
    await_timed_2(5*SECONDS, PNR_OK, pbp, PNR_OK, pbp_2);

    expect_pnr(pubnub_subscribe(pbp, NULL, "gr"), PNR_STARTED);
    expect_pnr(pubnub_subscribe(pbp_2, "ch", NULL), PNR_STARTED);
    await_timed_2(5*SECONDS, PNR_OK, pbp, PNR_OK, pbp_2);
    expect(pnfntst_got_messages(pbp, "\"Test M3\"", "\"Test M3 - 1\"", NULL));
    expect(pnfntst_got_messages(pbp_2, "\"Test M3\"", NULL));

    expect_pnr(pubnub_remove_channel_from_group(pbp, "two,three", "gr"), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp);

    TEST_POP_DEFERRED;
    TEST_POP_DEFERRED;
    } TEST_ENDDEF


TEST_DEF(connect_disconnect_and_connect_again) {

    static pubnub_t *pbp;
    pbp = pubnub_alloc();
    TEST_DEFER(pubnub_free, pbp);
    pubnub_init(pbp, g_pubkey, g_keysub);
    pubnub_origin_set(pbp, g_origin);
    
    expect_pnr(pubnub_subscribe(pbp, "ch", NULL), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp);

    expect_pnr(pubnub_publish(pbp, "ch", "\"Test M4\""), PNR_STARTED);
    pubnub_cancel(pbp);
    await_timed(5*SECONDS, PNR_CANCELLED, pbp);

    expect_pnr(pubnub_publish(pbp, "ch", "\"Test M4 - 2\""), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp);
    expect_pnr(pubnub_subscribe(pbp, "ch", NULL), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp);
    expect(pnfntst_got_messages(pbp, "\"Test M4 - 2\"", NULL));

    pubnub_set_non_blocking_io(pbp);
    expect_pnr(pubnub_subscribe(pbp, "ch", NULL), PNR_STARTED);
    pubnub_cancel(pbp);
    await_timed(5*SECONDS, PNR_CANCELLED, pbp);
    expect_pnr(pubnub_publish(pbp, "ch", "\"Test M4 - 3\""), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp);
    expect_pnr(pubnub_subscribe(pbp, "ch", NULL), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp);
    expect(pnfntst_got_messages(pbp, "\"Test M4 - 3\"", NULL));

    TEST_POP_DEFERRED;
    } TEST_ENDDEF


TEST_DEF(connect_disconnect_and_connect_again_group) {

    static pubnub_t *pbp;
    pbp = pubnub_alloc();
    TEST_DEFER(pubnub_free, pbp);
    pubnub_init(pbp, g_pubkey, g_keysub);
    pubnub_origin_set(pbp, g_origin);
    
    expect_pnr(pubnub_remove_channel_group(pbp, "gr"), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp);
    expect_pnr(pubnub_add_channel_to_group(pbp, "ch", "gr"), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp);
    
    TEST_SLEEP_FOR(CHANNEL_REGISTRY_PROPAGATION_DELAY);
    
    expect_pnr(pubnub_subscribe(pbp, NULL, "gr"), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp);
    
    expect_pnr(pubnub_publish(pbp, "ch", "\"Test M4\""), PNR_STARTED);
    pubnub_cancel(pbp);
    await_timed(5*SECONDS, PNR_CANCELLED, pbp);

    expect_pnr(pubnub_publish(pbp, "ch", "\"Test M4 - 2\""), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp);
    expect_pnr(pubnub_subscribe(pbp, NULL, "gr"), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp);
    expect(pnfntst_got_messages(pbp, "\"Test M4 - 2\"", NULL));

    pubnub_set_non_blocking_io(pbp);
    expect_pnr(pubnub_subscribe(pbp, NULL, "gr"), PNR_STARTED);
    pubnub_cancel(pbp);
    await_timed(5*SECONDS, PNR_CANCELLED, pbp);
    expect_pnr(pubnub_publish(pbp, "ch", "\"Test M4 - 3\""), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp);
    expect_pnr(pubnub_subscribe(pbp, NULL, "gr"), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp);
    expect(pnfntst_got_messages(pbp, "\"Test M4 - 3\"", NULL));

    expect_pnr(pubnub_remove_channel_from_group(pbp, "ch", "gr"), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp);

    TEST_POP_DEFERRED;
    } TEST_ENDDEF


TEST_DEF(connect_disconnect_and_connect_again_combo) {

    static pubnub_t *pbp;
    static pubnub_t *pbp_2;
    pbp = pubnub_alloc();
    TEST_DEFER(pubnub_free, pbp);
    pubnub_init(pbp, g_pubkey, g_keysub);
    pubnub_origin_set(pbp, g_origin);
    pbp_2 = pubnub_alloc();
    TEST_DEFER(pubnub_free, pbp_2);
    pubnub_init(pbp_2, g_pubkey, g_keysub);
    pubnub_origin_set(pbp_2, g_origin);
    
    expect_pnr(pubnub_remove_channel_group(pbp, "gr"), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp);
    expect_pnr(pubnub_add_channel_to_group(pbp, "ch", "gr"), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp);

    TEST_SLEEP_FOR(CHANNEL_REGISTRY_PROPAGATION_DELAY);

    expect_pnr(pubnub_subscribe(pbp, NULL, "gr"), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp);

    expect_pnr(pubnub_publish(pbp, "ch", "\"Test M4\""), PNR_STARTED);
    pubnub_cancel(pbp);
    expect_pnr(pubnub_publish(pbp_2, "two", "\"Test M5\""), PNR_STARTED);
    pubnub_cancel(pbp_2);
    await_timed_2(5*SECONDS, PNR_CANCELLED, pbp, PNR_CANCELLED, pbp_2);

    expect_pnr(pubnub_publish(pbp, "ch", "\"Test M4 - 2\""), PNR_STARTED);
    expect_pnr(pubnub_publish(pbp_2, "two", "\"Test M5 - 2\""), PNR_STARTED);
    await_timed_2(5*SECONDS, PNR_OK, pbp, PNR_OK, pbp_2);
    expect_pnr(pubnub_subscribe(pbp, "two", "gr"), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp);
    expect(pnfntst_got_messages(pbp, "\"Test M4 - 2\"", "\"Test M5 - 2\"", NULL));

    pubnub_set_non_blocking_io(pbp);
    expect_pnr(pubnub_subscribe(pbp, "two", "gr"), PNR_STARTED);
    pubnub_cancel(pbp);
    await_timed(5*SECONDS, PNR_CANCELLED, pbp);
    expect_pnr(pubnub_publish(pbp, "ch", "\"Test M4 - 3\""), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp);
    expect_pnr(pubnub_publish(pbp, "two", "\"Test M5 - 3\""), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp);
    expect_pnr(pubnub_subscribe(pbp, "two", "gr"), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp);
    expect(pnfntst_got_messages(pbp, "\"Test M4 - 3\"", "\"Test M5 - 3\"", NULL));

    expect_pnr(pubnub_remove_channel_from_group(pbp, "ch", "gr"), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp);

    TEST_POP_DEFERRED;
    TEST_POP_DEFERRED;
    } TEST_ENDDEF

	
TEST_DEF(wrong_api_usage) {

    static pubnub_t *pbp;
    pbp = pubnub_alloc();
    TEST_DEFER(pubnub_free, pbp);
    pubnub_init(pbp, g_pubkey, g_keysub);
    pubnub_origin_set(pbp, g_origin);
    
    expect_pnr(pubnub_subscribe(pbp, "ch", NULL), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp);

    expect_pnr(pubnub_publish(pbp, "ch", "\"Test \""), PNR_STARTED);
    expect_pnr(pubnub_publish(pbp, "ch", "\"Test - 2\""), PNR_IN_PROGRESS);
    expect_pnr(pubnub_subscribe(pbp, "ch", NULL), PNR_IN_PROGRESS);
    await_timed(5*SECONDS, PNR_OK, pbp);

    pubnub_set_non_blocking_io(pbp);
    expect_pnr(pubnub_subscribe(pbp, "ch", NULL), PNR_STARTED);
    expect_pnr(pubnub_subscribe(pbp, "ch", NULL), PNR_IN_PROGRESS);
    expect_pnr(pubnub_publish(pbp, "ch", "\"Test - 2\""), PNR_IN_PROGRESS);

    pubnub_cancel(pbp);
    await_timed(5*SECONDS, PNR_CANCELLED, pbp);

    expect_pnr(pubnub_subscribe(pbp, NULL, NULL), PNR_INVALID_CHANNEL);

    TEST_POP_DEFERRED;
    } TEST_ENDDEF

TEST_DEF(handling_errors_from_pubnub) {

    static pubnub_t *pbp;
    pbp = pubnub_alloc();
    TEST_DEFER(pubnub_free, pbp);
    pubnub_init(pbp, g_pubkey, g_keysub);
    pubnub_origin_set(pbp, g_origin);

    expect_pnr(pubnub_publish(pbp, "ch", "\"Test "), PNR_STARTED);
    await_timed(5*SECONDS, PNR_PUBLISH_FAILED, pbp);
    expect(400 == pubnub_last_http_code(pbp));
    expect(pubnub_parse_publish_result(pubnub_last_publish_result(pbp)) == PNPUB_INVALID_JSON);
    
    expect_pnr(pubnub_publish(pbp, ",", "\"Test \""), PNR_STARTED);
    await_timed(5*SECONDS, PNR_PUBLISH_FAILED, pbp);
    expect(400 == pubnub_last_http_code(pbp));
    expect(pubnub_parse_publish_result(pubnub_last_publish_result(pbp)) == PNPUB_INVALID_CHAR_IN_CHAN_NAME);

    TEST_POP_DEFERRED;
    } TEST_ENDDEF

