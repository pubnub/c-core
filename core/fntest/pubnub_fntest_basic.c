/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_sync.h"

#include "pubnub_fntest.h"
#include "core/pubnub_blocking_io.h"

#define SECONDS 1000
#define CHANNEL_REGISTRY_PROPAGATION_DELAY 800


TEST_DEF(simple_connect_and_send_over_single_channel) {

    static pubnub_t *pbp;
    enum pubnub_res rslt;
    pbp = pubnub_alloc();
    TEST_DEFER(pnfntst_free, pbp);
    pubnub_init(pbp, g_pubkey, g_keysub);
    pubnub_origin_set(pbp, g_origin);

    rslt = pubnub_subscribe(pbp, "ch", NULL);
    expect_pnr(rslt, PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp);

    rslt = pubnub_publish(pbp, "ch", "\"Test 1\"");
    expect_pnr_maybe_started(rslt, pbp, 5*SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "ch", "\"Test 1 - 2\"");
    expect_pnr_maybe_started(rslt, pbp, 5*SECONDS, PNR_OK);

    rslt = pubnub_subscribe(pbp, "ch", NULL);
    expect_pnr_maybe_started(rslt, pbp, 5*SECONDS, PNR_OK);

    expect(pnfntst_got_messages(pbp, "\"Test 1\"", "\"Test 1 - 2\"", NULL));

    TEST_POP_DEFERRED;
    } TEST_ENDDEF


TEST_DEF(connect_and_send_over_several_channels_simultaneously) {

    static pubnub_t *pbp;
    enum pubnub_res rslt;
    pbp = pubnub_alloc();
    TEST_DEFER(pnfntst_free, pbp);
    pubnub_init(pbp, g_pubkey, g_keysub);
    pubnub_origin_set(pbp, g_origin);

    rslt = pubnub_subscribe(pbp, "ch", NULL);
    expect_pnr(rslt, PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp);

    rslt = pubnub_publish(pbp, "ch", "\"Test M1\"");
    expect_pnr_maybe_started(rslt, pbp, 5*SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "two", "\"Test M1\"");
    expect_pnr_maybe_started(rslt, pbp, 5*SECONDS, PNR_OK);

    rslt = pubnub_subscribe(pbp, "ch", NULL);
    expect_pnr_maybe_started(rslt, pbp, 5*SECONDS, PNR_OK);

    expect(pnfntst_got_messages(pbp, "\"Test M1\"", NULL));

    rslt = pubnub_subscribe(pbp, "two", NULL);
    expect_pnr_maybe_started(rslt, pbp, 5*SECONDS, PNR_OK);

    expect(pnfntst_got_messages(pbp, "\"Test M1\"", NULL));

    TEST_POP_DEFERRED;
    } TEST_ENDDEF


TEST_DEF(simple_connect_and_send_over_single_channel_in_group) {

    static pubnub_t *pbp;
    enum pubnub_res rslt;
    pbp = pubnub_alloc();
    TEST_DEFER(pnfntst_free, pbp);
    pubnub_init(pbp, g_pubkey, g_keysub);
    pubnub_origin_set(pbp, g_origin);
    
    rslt = pubnub_remove_channel_group(pbp, "gr");
    expect_pnr(rslt, PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp);

    rslt = pubnub_add_channel_to_group(pbp, "ch", "gr");
    expect_pnr_maybe_started(rslt, pbp, 5*SECONDS, PNR_OK);
    
    TEST_SLEEP_FOR(CHANNEL_REGISTRY_PROPAGATION_DELAY);
    
    rslt = pubnub_subscribe(pbp, NULL, "gr");
    expect_pnr_maybe_started(rslt, pbp, 5*SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "ch", "\"Test 1\"");
    expect_pnr_maybe_started(rslt, pbp, 5*SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "ch", "\"Test 1 - 2\"");
    expect_pnr_maybe_started(rslt, pbp, 5*SECONDS, PNR_OK);

    rslt = pubnub_subscribe(pbp, NULL, "gr");
    expect_pnr_maybe_started(rslt, pbp, 5*SECONDS, PNR_OK);

    expect(pnfntst_got_messages(pbp, "\"Test 1\"", "\"Test 1 - 2\"", NULL));

    rslt = pubnub_remove_channel_from_group(pbp, "ch", "gr");
    expect_pnr_maybe_started(rslt, pbp, 5*SECONDS, PNR_OK);

    TEST_POP_DEFERRED;
    } TEST_ENDDEF


TEST_DEF(connect_and_send_over_several_channels_in_group_simultaneously) {

    static pubnub_t *pbp;
    enum pubnub_res rslt;
    pbp = pubnub_alloc();
    TEST_DEFER(pnfntst_free, pbp);
    pubnub_init(pbp, g_pubkey, g_keysub);
    pubnub_origin_set(pbp, g_origin);

    expect_pnr(pubnub_remove_channel_group(pbp, "gr"), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp);
    rslt = pubnub_add_channel_to_group(pbp, "ch,two", "gr");
    expect_pnr_maybe_started(rslt, pbp, 5*SECONDS, PNR_OK);

    TEST_SLEEP_FOR(CHANNEL_REGISTRY_PROPAGATION_DELAY);
    rslt = pubnub_subscribe(pbp, NULL, "gr");
    expect_pnr_maybe_started(rslt, pbp, 5*SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "ch", "\"Test M1\"");
    expect_pnr_maybe_started(rslt, pbp, 5*SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "two", "\"Test M2\"");
    expect_pnr_maybe_started(rslt, pbp, 5*SECONDS, PNR_OK);

    expect(pnfntst_subscribe_and_check(pbp, NULL, "gr", 5*SECONDS, "\"Test M1\"", "ch", "\"Test M2\"", "two", NULL));

    rslt = pubnub_remove_channel_from_group(pbp, "ch,two", "gr");
    expect_pnr_maybe_started(rslt, pbp, 5*SECONDS, PNR_OK);

    TEST_POP_DEFERRED;
    } TEST_ENDDEF


TEST_DEF(connect_and_send_over_channel_in_group_and_single_channel_simultaneously) {

    static pubnub_t *pbp;
    enum pubnub_res rslt;
    pbp = pubnub_alloc();
    TEST_DEFER(pnfntst_free, pbp);
    pubnub_init(pbp, g_pubkey, g_keysub);
    pubnub_origin_set(pbp, g_origin);

    expect_pnr(pubnub_remove_channel_group(pbp, "gr"), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp);
    rslt = pubnub_add_channel_to_group(pbp, "ch", "gr");
    expect_pnr_maybe_started(rslt, pbp, 5*SECONDS, PNR_OK);

    TEST_SLEEP_FOR(CHANNEL_REGISTRY_PROPAGATION_DELAY);
	
    rslt = pubnub_subscribe(pbp, "two", "gr");
    expect_pnr_maybe_started(rslt, pbp, 5*SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "ch", "\"Test M1\"");
    expect_pnr_maybe_started(rslt, pbp, 5*SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "two", "\"Test M2\"");
    expect_pnr_maybe_started(rslt, pbp, 5*SECONDS, PNR_OK);

    expect(pnfntst_subscribe_and_check(pbp, "two", "gr", 5*SECONDS, "\"Test M1\"", "ch", "\"Test M2\"", "two", NULL));

    rslt = pubnub_remove_channel_from_group(pbp, "ch", "gr");
    expect_pnr_maybe_started(rslt, pbp, 5*SECONDS, PNR_OK);

    TEST_POP_DEFERRED;
    } TEST_ENDDEF


TEST_DEF(connect_and_send_over_channel_in_group_and_multi_channel_simultaneously) {

    static pubnub_t *pbp;
    enum pubnub_res rslt;
    pbp = pubnub_alloc();
    TEST_DEFER(pnfntst_free, pbp);
    pubnub_init(pbp, g_pubkey, g_keysub);
    pubnub_origin_set(pbp, g_origin);

    expect_pnr(pubnub_remove_channel_group(pbp, "gr"), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp);
    rslt = pubnub_add_channel_to_group(pbp, "three", "gr");
    expect_pnr_maybe_started(rslt, pbp, 5*SECONDS, PNR_OK);

    TEST_SLEEP_FOR(CHANNEL_REGISTRY_PROPAGATION_DELAY);

    rslt = pubnub_subscribe(pbp, "ch,two", "gr");
    expect_pnr_maybe_started(rslt, pbp, 5*SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "ch", "\"Test M1\"");
    expect_pnr_maybe_started(rslt, pbp, 5*SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "two", "\"Test M1\"");
    expect_pnr_maybe_started(rslt, pbp, 5*SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "three", "\"Test M1\"");
    expect_pnr_maybe_started(rslt, pbp, 5*SECONDS, PNR_OK);

    expect(pnfntst_subscribe_and_check(pbp, "ch,two", "gr", 5*SECONDS, "\"Test M1\"", "ch", "\"Test M1\"", "two", "\"Test M1\"", "three", NULL));

    rslt = pubnub_remove_channel_from_group(pbp, "three", "gr");
    expect_pnr_maybe_started(rslt, pbp, 5*SECONDS, PNR_OK);

    TEST_POP_DEFERRED;
    } TEST_ENDDEF


TEST_DEF(simple_connect_and_receiver_over_single_channel) {

    static pubnub_t *pbp;
    static pubnub_t *pbp_2;
    enum pubnub_res rslt;
    pbp = pubnub_alloc();
    TEST_DEFER(pnfntst_free, pbp);
    pubnub_init(pbp, g_pubkey, g_keysub);
    pubnub_origin_set(pbp, g_origin);
    pbp_2 = pubnub_alloc();
    TEST_DEFER(pnfntst_free, pbp_2);
    pubnub_init(pbp_2, g_pubkey, g_keysub);
    pubnub_origin_set(pbp_2, g_origin);
    pubnub_set_non_blocking_io(pbp_2);

    rslt = pubnub_subscribe(pbp_2, "ch", NULL);
    expect_pnr(rslt, PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp_2);

    rslt = pubnub_subscribe(pbp_2, "ch", NULL);
    expect_pnr(rslt, PNR_STARTED);
    rslt = pubnub_publish(pbp, "ch", "\"Test 2\"");
    expect_pnr(rslt, PNR_STARTED);
    await_timed_2(5*SECONDS, PNR_OK, pbp, PNR_OK, pbp_2);
    
    expect(pnfntst_got_messages(pbp_2, "\"Test 2\"", NULL));

    rslt = pubnub_publish(pbp, "ch", "\"Test 2 - 2\"");
    expect_pnr_maybe_started(rslt, pbp, 5*SECONDS, PNR_OK);

    rslt = pubnub_subscribe(pbp_2, "ch", NULL);
    expect_pnr_maybe_started(rslt, pbp_2, 5*SECONDS, PNR_OK);

    expect(pnfntst_got_messages(pbp_2, "\"Test 2 - 2\"", NULL));

    TEST_POP_DEFERRED;
    TEST_POP_DEFERRED;
    } TEST_ENDDEF


TEST_DEF(connect_and_receive_over_several_channels_simultaneously) {

    static pubnub_t *pbp;
    static pubnub_t *pbp_2;
    enum pubnub_res rslt;
    pbp = pubnub_alloc();
    TEST_DEFER(pnfntst_free, pbp);
    pubnub_init(pbp, g_pubkey, g_keysub);
    pubnub_origin_set(pbp, g_origin);
    pbp_2 = pubnub_alloc();
    TEST_DEFER(pnfntst_free, pbp_2);
    pubnub_init(pbp_2, g_pubkey, g_keysub);
    pubnub_origin_set(pbp_2, g_origin);

    expect_pnr(pubnub_subscribe(pbp_2, "ch,two", NULL), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp_2);

    expect_pnr(pubnub_publish(pbp, "ch", "\"Test M2\""), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp);
    rslt = pubnub_publish(pbp, "two", "\"Test M2-2\"");
    expect_pnr_maybe_started(rslt, pbp, 5*SECONDS, PNR_OK);
    rslt = pubnub_subscribe(pbp_2, "ch,two", NULL);
    expect_pnr_maybe_started(rslt, pbp_2, 5*SECONDS, PNR_OK);

    expect(pnfntst_got_messages(pbp_2, "\"Test M2\"", "\"Test M2-2\"", NULL));

    TEST_POP_DEFERRED;
    TEST_POP_DEFERRED;
    } TEST_ENDDEF


TEST_DEF(simple_connect_and_receiver_over_single_channel_in_group) {

    static pubnub_t *pbp;
    static pubnub_t *pbp_2;
    enum pubnub_res rslt;
    pbp = pubnub_alloc();
    TEST_DEFER(pnfntst_free, pbp);
    pubnub_init(pbp, g_pubkey, g_keysub);
    pubnub_origin_set(pbp, g_origin);
    pbp_2 = pubnub_alloc();
    TEST_DEFER(pnfntst_free, pbp_2);
    pubnub_init(pbp_2, g_pubkey, g_keysub);
    pubnub_origin_set(pbp_2, g_origin);
    pubnub_set_non_blocking_io(pbp_2);

    expect_pnr(pubnub_remove_channel_group(pbp_2, "gr"), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp_2);
    rslt = pubnub_add_channel_to_group(pbp_2, "ch", "gr");
    expect_pnr_maybe_started(rslt, pbp_2, 5*SECONDS, PNR_OK);

    TEST_SLEEP_FOR(CHANNEL_REGISTRY_PROPAGATION_DELAY);

    rslt = pubnub_subscribe(pbp_2, NULL, "gr");
    expect_pnr_maybe_started(rslt, pbp_2, 5*SECONDS, PNR_OK);

    expect_pnr(pubnub_subscribe(pbp_2, NULL, "gr"), PNR_STARTED);
    expect_pnr(pubnub_publish(pbp, "ch", "\"Test 2\""), PNR_STARTED);
    await_timed_2(5*SECONDS, PNR_OK, pbp, PNR_OK, pbp_2);

    expect(pnfntst_got_messages(pbp_2, "\"Test 2\"", NULL));

    rslt = pubnub_publish(pbp, "ch", "\"Test 2 - 2\"");
    expect_pnr_maybe_started(rslt, pbp, 5*SECONDS, PNR_OK);

    expect_pnr(pubnub_subscribe(pbp_2, NULL, "gr"), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp_2);

    expect(pnfntst_got_messages(pbp_2, "\"Test 2 - 2\"", NULL));

    rslt = pubnub_remove_channel_from_group(pbp, "ch", "gr");
    expect_pnr_maybe_started(rslt, pbp, 5*SECONDS, PNR_OK);

    TEST_POP_DEFERRED;
    TEST_POP_DEFERRED;
    } TEST_ENDDEF


TEST_DEF(connect_and_receive_over_several_channels_in_group_simultaneously) {

    static pubnub_t *pbp;
    static pubnub_t *pbp_2;
    enum pubnub_res rslt;
    pbp = pubnub_alloc();
    TEST_DEFER(pnfntst_free, pbp);
    pubnub_init(pbp, g_pubkey, g_keysub);
    pubnub_origin_set(pbp, g_origin);
    pbp_2 = pubnub_alloc();
    TEST_DEFER(pnfntst_free, pbp_2);
    pubnub_init(pbp_2, g_pubkey, g_keysub);
    pubnub_origin_set(pbp_2, g_origin);
    pubnub_set_non_blocking_io(pbp_2);

    expect_pnr(pubnub_remove_channel_group(pbp_2, "gr"), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp_2);
    rslt = pubnub_add_channel_to_group(pbp_2, "ch,two", "gr");
    expect_pnr_maybe_started(rslt, pbp_2, 5*SECONDS, PNR_OK);

    TEST_SLEEP_FOR(CHANNEL_REGISTRY_PROPAGATION_DELAY);

    rslt = pubnub_subscribe(pbp_2, NULL, "gr");
    expect_pnr_maybe_started(rslt, pbp_2, 5*SECONDS, PNR_OK);

    expect_pnr(pubnub_publish(pbp, "ch", "\"Test M2\""), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp);
    rslt = pubnub_publish(pbp, "two", "\"Test M2-2\"");
    expect_pnr_maybe_started(rslt, pbp, 5*SECONDS, PNR_OK);
    
    expect(pnfntst_subscribe_and_check(pbp_2, NULL, "gr", 5*SECONDS, "\"Test M2\"", "ch", "\"Test M2-2\"", "two", NULL));

    expect_pnr(pubnub_remove_channel_from_group(pbp_2, "ch,two", "gr"), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp_2);

    TEST_POP_DEFERRED;
    TEST_POP_DEFERRED;
    } TEST_ENDDEF


TEST_DEF(connect_and_receive_over_channel_in_group_and_single_channel_simultaneously) {

    static pubnub_t *pbp;
    static pubnub_t *pbp_2;
    enum pubnub_res rslt;
    pbp = pubnub_alloc();
    TEST_DEFER(pnfntst_free, pbp);
    pubnub_init(pbp, g_pubkey, g_keysub);
    pubnub_origin_set(pbp, g_origin);
    pbp_2 = pubnub_alloc();
    TEST_DEFER(pnfntst_free, pbp_2);
    pubnub_init(pbp_2, g_pubkey, g_keysub);
    pubnub_origin_set(pbp_2, g_origin);
    pubnub_set_non_blocking_io(pbp_2);

    expect_pnr(pubnub_remove_channel_group(pbp, "gr"), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp);
    rslt = pubnub_add_channel_to_group(pbp, "ch", "gr");
    expect_pnr_maybe_started(rslt, pbp, 5*SECONDS, PNR_OK);

    TEST_SLEEP_FOR(CHANNEL_REGISTRY_PROPAGATION_DELAY);

    expect_pnr(pubnub_subscribe(pbp_2, "two", "gr"), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp_2);

    rslt = pubnub_publish(pbp, "ch", "\"Test M2\"");
    expect_pnr_maybe_started(rslt, pbp, 5*SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "two", "\"Test M2-2\"");
    expect_pnr_maybe_started(rslt, pbp, 5*SECONDS, PNR_OK);

    expect(pnfntst_subscribe_and_check(pbp_2, "two", "gr", 5*SECONDS, "\"Test M2\"", "ch", "\"Test M2-2\"", "two", NULL));

    rslt = pubnub_remove_channel_from_group(pbp, "ch", "gr");
    expect_pnr_maybe_started(rslt, pbp, 5*SECONDS, PNR_OK);

    TEST_POP_DEFERRED;
    TEST_POP_DEFERRED;
    } TEST_ENDDEF


TEST_DEF(connect_and_receive_over_channel_in_group_and_multi_channel_simultaneously) {

    static pubnub_t *pbp;
    static pubnub_t *pbp_2;
    enum pubnub_res rslt;
    pbp = pubnub_alloc();
    TEST_DEFER(pnfntst_free, pbp);
    pubnub_init(pbp, g_pubkey, g_keysub);
    pubnub_origin_set(pbp, g_origin);
    pbp_2 = pubnub_alloc();
    TEST_DEFER(pnfntst_free, pbp_2);
    pubnub_init(pbp_2, g_pubkey, g_keysub);
    pubnub_origin_set(pbp_2, g_origin);
    pubnub_set_non_blocking_io(pbp_2);

    expect_pnr(pubnub_remove_channel_group(pbp_2, "gr"), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp_2);
    rslt = pubnub_add_channel_to_group(pbp_2, "three", "gr");
    expect_pnr_maybe_started(rslt, pbp_2, 5*SECONDS, PNR_OK);

    TEST_SLEEP_FOR(CHANNEL_REGISTRY_PROPAGATION_DELAY);

    rslt = pubnub_subscribe(pbp_2, "ch,two", "gr");
    expect_pnr_maybe_started(rslt, pbp_2, 5*SECONDS, PNR_OK);

    expect_pnr(pubnub_publish(pbp, "ch", "\"Test M2\""), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp);
    rslt = pubnub_publish(pbp, "two", "\"Test M2-2\"");
    expect_pnr_maybe_started(rslt, pbp, 5*SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "three", "\"Test M2-3\"");
    expect_pnr_maybe_started(rslt, pbp, 5*SECONDS, PNR_OK);

    expect(pnfntst_subscribe_and_check(pbp_2, "ch,two", "gr", 5*SECONDS, "\"Test M2\"", "ch", "\"Test M2-2\"", "two", "\"Test M2-3\"", "three", NULL));

    expect_pnr(pubnub_remove_channel_from_group(pbp_2, "three", "gr"), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp_2);

    TEST_POP_DEFERRED;
    TEST_POP_DEFERRED;
    } TEST_ENDDEF


TEST_DEF(broken_connection_test) {
    static pubnub_t *pbp;
    enum pubnub_res rslt;
    pbp = pubnub_alloc();
    TEST_DEFER(pnfntst_free, pbp);
    pubnub_init(pbp, g_pubkey, g_keysub);
    pubnub_origin_set(pbp, g_origin);

    expect_pnr(pubnub_subscribe(pbp, "ch", NULL), PNR_STARTED);
    await_timed(6*SECONDS, PNR_OK, pbp);

    rslt = pubnub_publish(pbp, "ch", "\"Test 3\"");
    expect_pnr_maybe_started(rslt, pbp, 6*SECONDS, PNR_OK);

    rslt = pubnub_subscribe(pbp, "ch", NULL);
    expect_pnr_maybe_started(rslt, pbp, 6*SECONDS, PNR_OK);

    expect(pnfntst_got_messages(pbp, "\"Test 3\"", NULL));

    rslt = pubnub_publish(pbp, "ch", "\"Test 3 - 2\"");
    expect_pnr_maybe_started(rslt, pbp, 6*SECONDS, PNR_OK);

    printf("Please disconnect from Internet. Press Enter when done.");
    await_console();
    expect_pnr(pubnub_subscribe(pbp, "ch", NULL), PNR_STARTED);
    await_timed(6*SECONDS, PNR_ADDR_RESOLUTION_FAILED, pbp);
    printf("Please reconnect to Internet. Press Enter when done.");
    await_console();
    expect_pnr(pubnub_subscribe(pbp, "ch", NULL), PNR_STARTED);
    await_timed(6*SECONDS, PNR_OK, pbp);
    expect(pnfntst_got_messages(pbp, "\"Test 3 - 2\"", NULL));

    printf("Please disconnect from Internet. Press Enter when done.");
    await_console();
    expect_pnr(pubnub_publish(pbp, "ch", "\"Test 3 - 3\""), PNR_STARTED);
    await_timed(6*SECONDS, PNR_ADDR_RESOLUTION_FAILED, pbp);

    printf("Please reconnect to Internet. Press Enter when done.");
    await_console();
    expect_pnr(pubnub_publish(pbp, "ch", "\"Test 3 - 4\""), PNR_STARTED);
    await_timed(6*SECONDS, PNR_OK, pbp);
    rslt = pubnub_subscribe(pbp, "ch", NULL);
    expect_pnr_maybe_started(rslt, pbp, 6*SECONDS, PNR_OK);

    expect(pnfntst_got_messages(pbp, "\"Test 3 - 4\"", NULL));

    TEST_POP_DEFERRED;
    } TEST_ENDDEF


TEST_DEF(broken_connection_test_multi) {
    static pubnub_t *pbp;
    enum pubnub_res rslt;
    pbp = pubnub_alloc();
    TEST_DEFER(pnfntst_free, pbp);
    pubnub_init(pbp, g_pubkey, g_keysub);
    pubnub_origin_set(pbp, g_origin);

    expect_pnr(pubnub_subscribe(pbp, "ch,two", NULL), PNR_STARTED);
    await_timed(6*SECONDS, PNR_OK, pbp);

    rslt = pubnub_publish(pbp, "ch", "\"Test 3\"");
    expect_pnr_maybe_started(rslt, pbp, 6*SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "two", "\"Test 4\"");
    expect_pnr_maybe_started(rslt, pbp, 6*SECONDS, PNR_OK);

    rslt = pubnub_subscribe(pbp, "ch,two", NULL);
    expect_pnr_maybe_started(rslt, pbp, 6*SECONDS, PNR_OK);

    expect(pnfntst_got_messages(pbp, "\"Test 3\"", "\"Test 4\"", NULL));

    rslt = pubnub_publish(pbp, "ch", "\"Test 3 - 2\"");
    expect_pnr_maybe_started(rslt, pbp, 6*SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "two", "\"Test 4 - 2\"");
    expect_pnr_maybe_started(rslt, pbp, 6*SECONDS, PNR_OK);

    printf("Please disconnect from Internet. Press Enter when done.");
    await_console();
    expect_pnr(pubnub_subscribe(pbp, "ch,two", NULL), PNR_STARTED);
    await_timed(6*SECONDS, PNR_ADDR_RESOLUTION_FAILED, pbp);
    printf("Please reconnect to Internet. Press Enter when done.");
    await_console();
    expect_pnr(pubnub_subscribe(pbp, "ch,two", NULL), PNR_STARTED);
    await_timed(6*SECONDS, PNR_OK, pbp);
    expect(pnfntst_got_messages(pbp, "\"Test 3 - 2\"", "\"Test 4 - 2\"", NULL));

    rslt = pubnub_publish(pbp, "ch", "\"Test 3 - 4\"");
    expect_pnr_maybe_started(rslt, pbp, 6*SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "two", "\"Test 4 - 4\"");
    expect_pnr_maybe_started(rslt, pbp, 6*SECONDS, PNR_OK);

    rslt = pubnub_subscribe(pbp, "ch,two", NULL);
    expect_pnr_maybe_started(rslt, pbp, 6*SECONDS, PNR_OK);

    expect(pnfntst_got_messages(pbp, "\"Test 3 - 4\"", "\"Test 4 - 4\"", NULL));

    TEST_POP_DEFERRED;
    } TEST_ENDDEF


TEST_DEF(broken_connection_test_group) {
    static pubnub_t *pbp;
    enum pubnub_res rslt;
    pbp = pubnub_alloc();
    TEST_DEFER(pnfntst_free, pbp);
    pubnub_init(pbp, g_pubkey, g_keysub);
    pubnub_origin_set(pbp, g_origin);

    expect_pnr(pubnub_remove_channel_group(pbp, "gr"), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp);
    rslt = pubnub_add_channel_to_group(pbp, "ch", "gr");
    expect_pnr_maybe_started(rslt, pbp, 5*SECONDS, PNR_OK);

    TEST_SLEEP_FOR(CHANNEL_REGISTRY_PROPAGATION_DELAY);

    rslt = pubnub_subscribe(pbp, NULL, "gr");
    expect_pnr_maybe_started(rslt, pbp, 6*SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "ch", "\"Test 3\"");
    expect_pnr_maybe_started(rslt, pbp, 6*SECONDS, PNR_OK);

    rslt = pubnub_subscribe(pbp, NULL, "gr");
    expect_pnr_maybe_started(rslt, pbp, 6*SECONDS, PNR_OK);

    expect(pnfntst_got_messages(pbp, "\"Test 3\"", NULL));

    rslt = pubnub_publish(pbp, "ch", "\"Test 3 - 2\"");
    expect_pnr_maybe_started(rslt, pbp, 6*SECONDS, PNR_OK);

    printf("Please disconnect from Internet. Press Enter when done.");
    await_console();
    expect_pnr(pubnub_subscribe(pbp, NULL, "gr"), PNR_STARTED);
    await_timed(6*SECONDS, PNR_ADDR_RESOLUTION_FAILED, pbp);
    printf("Please reconnect to Internet. Press Enter when done.");
    await_console();
    expect_pnr(pubnub_subscribe(pbp, NULL, "gr"), PNR_STARTED);
    await_timed(6*SECONDS, PNR_OK, pbp);
    expect(pnfntst_got_messages(pbp, "\"Test 3 - 2\"", NULL));

    printf("Please disconnect from Internet. Press Enter when done.");
    await_console();
    expect_pnr(pubnub_publish(pbp, "ch", "\"Test 3 - 3\""), PNR_STARTED);
    await_timed(6*SECONDS, PNR_ADDR_RESOLUTION_FAILED, pbp);

    printf("Please reconnect to Internet. Press Enter when done.");
    await_console();
    expect_pnr(pubnub_publish(pbp, "ch", "\"Test 3 - 4\""), PNR_STARTED);
    await_timed(6*SECONDS, PNR_OK, pbp);
    rslt = pubnub_subscribe(pbp, NULL, "gr");
    expect_pnr_maybe_started(rslt, pbp, 6*SECONDS, PNR_OK);

    expect(pnfntst_got_messages(pbp, "\"Test 3 - 4\"", NULL));

    rslt = pubnub_remove_channel_from_group(pbp, "ch", "gr");
    expect_pnr_maybe_started(rslt, pbp, 5*SECONDS, PNR_OK);

    TEST_POP_DEFERRED;
    } TEST_ENDDEF


TEST_DEF(broken_connection_test_multi_in_group) {
    static pubnub_t *pbp;
    enum pubnub_res rslt;
    pbp = pubnub_alloc();
    TEST_DEFER(pnfntst_free, pbp);
    pubnub_init(pbp, g_pubkey, g_keysub);
    pubnub_origin_set(pbp, g_origin);

    expect_pnr(pubnub_remove_channel_group(pbp, "gr"), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp);
    rslt = pubnub_add_channel_to_group(pbp, "ch,two", "gr");
    expect_pnr_maybe_started(rslt, pbp, 5*SECONDS, PNR_OK);

    TEST_SLEEP_FOR(CHANNEL_REGISTRY_PROPAGATION_DELAY);

    rslt = pubnub_subscribe(pbp, "ch,two", NULL);
    expect_pnr_maybe_started(rslt, pbp, 6*SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "ch", "\"Test 3\"");
    expect_pnr_maybe_started(rslt, pbp, 6*SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "two", "\"Test 4\"");
    expect_pnr_maybe_started(rslt, pbp, 6*SECONDS, PNR_OK);

    rslt = pubnub_subscribe(pbp, "ch,two", NULL);
    expect_pnr_maybe_started(rslt, pbp, 6*SECONDS, PNR_OK);

    expect(pnfntst_got_messages(pbp, "\"Test 3\"", "\"Test 4\"", NULL));

    rslt = pubnub_publish(pbp, "ch", "\"Test 3 - 2\"");
    expect_pnr_maybe_started(rslt, pbp, 6*SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "two", "\"Test 4 - 2\"");
    expect_pnr_maybe_started(rslt, pbp, 6*SECONDS, PNR_OK);

    printf("Please disconnect from Internet. Press Enter when done.");
    await_console();
    expect_pnr(pubnub_subscribe(pbp, "ch,two", NULL), PNR_STARTED);
    await_timed(6*SECONDS, PNR_ADDR_RESOLUTION_FAILED, pbp);
    printf("Please reconnect to Internet. Press Enter when done.");
    await_console();
    expect_pnr(pubnub_subscribe(pbp, "ch,two", NULL), PNR_STARTED);
    await_timed(6*SECONDS, PNR_OK, pbp);
    expect(pnfntst_got_messages(pbp, "\"Test 3 - 2\"", "\"Test 4 - 2\"", NULL));

    rslt = pubnub_publish(pbp, "ch", "\"Test 3 - 4\"");
    expect_pnr_maybe_started(rslt, pbp, 6*SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "two", "\"Test 4 - 4\"");
    expect_pnr_maybe_started(rslt, pbp, 6*SECONDS, PNR_OK);

    rslt = pubnub_subscribe(pbp, "ch,two", NULL);
    expect_pnr_maybe_started(rslt, pbp, 6*SECONDS, PNR_OK);

    expect(pnfntst_got_messages(pbp, "\"Test 3 - 4\"", "\"Test 4 - 4\"", NULL));

    rslt = pubnub_remove_channel_from_group(pbp, "ch,two", "gr");
    expect_pnr_maybe_started(rslt, pbp, 5*SECONDS, PNR_OK);

    TEST_POP_DEFERRED;
    } TEST_ENDDEF


TEST_DEF(broken_connection_test_group_in_group_out) {
    static pubnub_t *pbp;
    enum pubnub_res rslt;
    pbp = pubnub_alloc();
    TEST_DEFER(pnfntst_free, pbp);
    pubnub_init(pbp, g_pubkey, g_keysub);
    pubnub_origin_set(pbp, g_origin);

    expect_pnr(pubnub_remove_channel_group(pbp, "gr"), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp);
    rslt = pubnub_add_channel_to_group(pbp, "ch", "gr");
    expect_pnr_maybe_started(rslt, pbp, 5*SECONDS, PNR_OK);

    TEST_SLEEP_FOR(CHANNEL_REGISTRY_PROPAGATION_DELAY);

    rslt = pubnub_subscribe(pbp, "two", "gr");
    expect_pnr_maybe_started(rslt, pbp, 6*SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "ch", "\"Test 3\"");
    expect_pnr_maybe_started(rslt, pbp, 6*SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "two", "\"Test 4\"");
    expect_pnr_maybe_started(rslt, pbp, 6*SECONDS, PNR_OK);

    rslt = pubnub_subscribe(pbp, "two", "gr");
    expect_pnr_maybe_started(rslt, pbp, 6*SECONDS, PNR_OK);

    expect(pnfntst_got_messages(pbp, "\"Test 3\"", "\"Test 4\"", NULL));

    rslt = pubnub_publish(pbp, "ch", "\"Test 3 - 2\"");
    expect_pnr_maybe_started(rslt, pbp, 6*SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "two", "\"Test 4 - 2\"");
    expect_pnr_maybe_started(rslt, pbp, 6*SECONDS, PNR_OK);

    printf("Please disconnect from Internet. Press Enter when done.");
    await_console();
    expect_pnr(pubnub_subscribe(pbp, "two", "gr"), PNR_STARTED);
    await_timed(6*SECONDS, PNR_ADDR_RESOLUTION_FAILED, pbp);

    printf("Please reconnect to Internet. Press Enter when done.");
    await_console();
    expect_pnr(pubnub_subscribe(pbp, "two", "gr"), PNR_STARTED);
    await_timed(6*SECONDS, PNR_OK, pbp);
    expect(pnfntst_got_messages(pbp, "\"Test 3 - 2\"", "\"Test 4 - 2\"", NULL));

    rslt = pubnub_publish(pbp, "ch", "\"Test 3 - 4\"");
    expect_pnr_maybe_started(rslt, pbp, 6*SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "two", "\"Test 4 - 4\"");
    expect_pnr_maybe_started(rslt, pbp, 6*SECONDS, PNR_OK);

    rslt = pubnub_subscribe(pbp, "two", "gr");
    expect_pnr_maybe_started(rslt, pbp, 6*SECONDS, PNR_OK);

    expect(pnfntst_got_messages(pbp, "\"Test 3 - 4\"", "\"Test 4 - 4\"", NULL));

    rslt = pubnub_remove_channel_from_group(pbp, "ch", "gr");
    expect_pnr_maybe_started(rslt, pbp, 6*SECONDS, PNR_OK);

    TEST_POP_DEFERRED;
    } TEST_ENDDEF


TEST_DEF(broken_connection_test_group_multichannel_out) {
    static pubnub_t *pbp;
    enum pubnub_res rslt;
    pbp = pubnub_alloc();
    TEST_DEFER(pnfntst_free, pbp);
    pubnub_init(pbp, g_pubkey, g_keysub);
    pubnub_origin_set(pbp, g_origin);

    expect_pnr(pubnub_remove_channel_group(pbp, "gr"), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp);
    rslt = pubnub_add_channel_to_group(pbp, "three", "gr");
    expect_pnr_maybe_started(rslt, pbp, 5*SECONDS, PNR_OK);

    TEST_SLEEP_FOR(CHANNEL_REGISTRY_PROPAGATION_DELAY);

    rslt = pubnub_subscribe(pbp, "ch,two", "gr");
    expect_pnr_maybe_started(rslt, pbp, 6*SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "ch", "\"Test 3\"");
    expect_pnr_maybe_started(rslt, pbp, 6*SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "two", "\"Test 4\"");
    expect_pnr_maybe_started(rslt, pbp, 6*SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "three", "\"Test 5\"");
    expect_pnr_maybe_started(rslt, pbp, 6*SECONDS, PNR_OK);

    rslt = pubnub_subscribe(pbp, "ch,two", "gr");
    expect_pnr_maybe_started(rslt, pbp, 6*SECONDS, PNR_OK);

    expect(pnfntst_got_messages(pbp, "\"Test 3\"", "\"Test 4\"", "\"Test 5\"", NULL));

    rslt = pubnub_publish(pbp, "ch", "\"Test 3 - 2\"");
    expect_pnr_maybe_started(rslt, pbp, 6*SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "two", "\"Test 4 - 2\"");
    expect_pnr_maybe_started(rslt, pbp, 6*SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "three", "\"Test 5 - 2\"");
    expect_pnr_maybe_started(rslt, pbp, 6*SECONDS, PNR_OK);

    printf("Please disconnect from Internet. Press Enter when done.");
    await_console();
    expect_pnr(pubnub_subscribe(pbp, "ch,two", "gr"), PNR_STARTED);
    await_timed(6*SECONDS, PNR_ADDR_RESOLUTION_FAILED, pbp);
    printf("Please reconnect to Internet. Press Enter when done.");
    await_console();
    expect_pnr(pubnub_subscribe(pbp, "ch,two", "gr"), PNR_STARTED);
    await_timed(6*SECONDS, PNR_OK, pbp);
    expect(pnfntst_got_messages(pbp, "\"Test 3 - 2\"", "\"Test 4 - 2\"", "\"Test 5 - 2\"", NULL));

    rslt = pubnub_publish(pbp, "ch", "\"Test 3 - 4\"");
    expect_pnr_maybe_started(rslt, pbp, 6*SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "two", "\"Test 4 - 4\"");
    expect_pnr_maybe_started(rslt, pbp, 6*SECONDS, PNR_OK);

    rslt = pubnub_publish(pbp, "three", "\"Test 5 - 4\"");
    expect_pnr_maybe_started(rslt, pbp, 6*SECONDS, PNR_OK);

    rslt = pubnub_subscribe(pbp, "ch,two", "gr");
    expect_pnr_maybe_started(rslt, pbp, 6*SECONDS, PNR_OK);

    expect(pnfntst_got_messages(pbp, "\"Test 3 - 4\"", "\"Test 4 - 4\"", "\"Test 5 - 4\"", NULL));

    rslt = pubnub_remove_channel_from_group(pbp, "three", "gr");
    expect_pnr_maybe_started(rslt, pbp, 5*SECONDS, PNR_OK);

    TEST_POP_DEFERRED;
    } TEST_ENDDEF
