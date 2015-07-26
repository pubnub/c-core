/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_fntest.h"

#include "pubnub_posix_sync.h"

#define SECONDS 1000


TEST_DEF(basic_conn_pub) {

    static pubnub_t *pbp;
    pbp = pubnub_alloc();
    TEST_DEFER(pubnub_free, pbp);
    pubnub_init(pbp, "demo", "demo");
    
    expect_pnr(pubnub_subscribe(pbp, "ch", NULL), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp);

    expect_pnr(pubnub_publish(pbp, "ch", "\"Test 1\""), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp);

    expect_pnr(pubnub_publish(pbp, "ch", "\"Test 1 - 2\""), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp);

    expect_pnr(pubnub_subscribe(pbp, "ch", NULL), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp);

    expect(pnfntst_got_messages(pbp, "\"Test 1\"", "\"Test 1 - 2\"", NULL));

    TEST_POP_DEFERRED;
    } TEST_ENDDEF


TEST_DEF(basic_conn_rx) {

    static pubnub_t *pbp;
    static pubnub_t *pbp_2;
    pbp = pubnub_alloc();
    TEST_DEFER(pubnub_free, pbp);
    pubnub_init(pbp, "demo", "demo");
    pbp_2 = pubnub_alloc();
    TEST_DEFER(pubnub_free, pbp_2);
    pubnub_init(pbp_2, "demo", "demo");

    expect_pnr(pubnub_subscribe(pbp_2, "ch", NULL), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp_2);

    pubnub_set_non_blocking_io(pbp_2);
    expect_pnr(pubnub_subscribe(pbp_2, "ch", NULL), PNR_STARTED);
    expect_pnr(pubnub_publish(pbp, "ch", "\"Test 2\""), PNR_STARTED);
    await_timed_2(5*SECONDS, PNR_OK, pbp, PNR_OK, pbp_2);

    expect(pnfntst_got_messages(pbp_2, "\"Test 2\"", NULL));

    expect_pnr(pubnub_publish(pbp, "ch", "\"Test 2 - 2\""), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp);

    expect_pnr(pubnub_subscribe(pbp_2, "ch", NULL), PNR_STARTED);
    await_timed(5*SECONDS, PNR_OK, pbp_2);

    expect(pnfntst_got_messages(pbp_2, "\"Test 2 - 2\"", NULL));
        
    TEST_POP_DEFERRED;
    TEST_POP_DEFERRED;
    } TEST_ENDDEF
