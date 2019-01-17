/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_fntest_medium.hpp"

#include <thread>


using namespace pubnub;

const std::chrono::seconds Td(5);
const std::chrono::milliseconds T_chan_registry_propagation(1000);


TEST_DEF(complex_send_and_receive_over_several_channels_simultaneously)
{
    context           pbp(pubkey, keysub, origin);
    context           pbp_2(pubkey, keysub, origin);
    std::string const ch(pnfntst_make_name(this_test_name_));
    std::string const two(pnfntst_make_name(this_test_name_));
    std::string const three(pnfntst_make_name(this_test_name_));
    std::string       two_three = two + comma + three;

    SENSE(pbp.subscribe(two_three)).in(Td) == PNR_OK;
    SENSE(pbp_2.subscribe(ch)).in(Td) == PNR_OK;

    pbp_2.set_blocking_io(non_blocking);
    SENSE(pbp_2.publish(two, "\"Test M3\"")).in(Td) == PNR_OK;
    (SENSE(pbp.publish(ch, "\"Test M3\"")) &&
     SENSE(pbp_2.publish(three, "\"Test M3-1\""))
        ).in(Td) == PNR_OK;
     
    (SENSE(pbp.subscribe(two_three)) && SENSE(pbp_2.subscribe(ch))
        ).in(Td) == PNR_OK;

    EXPECT_TRUE(got_messages(pbp, {"\"Test M3\"", "\"Test M3-1\""}));
    EXPECT_TRUE(got_messages(pbp_2, {"\"Test M3\""}));
}
TEST_ENDDEF

TEST_DEF_NEED_CHGROUP(complex_send_and_receive_over_channel_plus_group_simultaneously)
{
    context           pbp(pubkey, keysub, origin);
    context           pbp_2(pubkey, keysub, origin);
    std::string const ch(pnfntst_make_name(this_test_name_));
    std::string const two(pnfntst_make_name(this_test_name_));
    std::string const three(pnfntst_make_name(this_test_name_));
    std::string const gr(pnfntst_make_name(this_test_name_));
    std::string       two_three = two + comma + three;

    SENSE(pbp.remove_channel_group(gr)).in(Td) == PNR_OK;
    SENSE(pbp.add_channel_to_group(two_three, gr)).in(Td) == PNR_OK;

    std::this_thread::sleep_for(T_chan_registry_propagation);
    
    (SENSE(pbp.subscribe("", gr)) && SENSE(pbp_2.subscribe(ch))
        ).in(Td) == PNR_OK;
    
    SENSE(pbp.publish(two, "\"Test M3\"")).in(Td) == PNR_OK;
    (SENSE(pbp_2.publish(ch, "\"Test M3\"")) && 
        SENSE(pbp.publish(three, "\"Test M3-1\""))
        ).in(Td) == PNR_OK;
    
    (SENSE(pbp.subscribe("", gr)) && SENSE(pbp_2.subscribe(ch))
        ).in(Td) == PNR_OK;

    EXPECT_TRUE(got_messages(pbp, {"\"Test M3\"", "\"Test M3-1\""}));
    EXPECT_TRUE(got_messages(pbp_2, {"\"Test M3\""}));
    
    SENSE(pbp.remove_channel_group(gr)).in(Td) == PNR_OK;
}
TEST_ENDDEF

TEST_DEF(connect_disconnect_and_connect_again)
{
    context                   pbp(pubkey, keysub, origin);
    std::string const         ch(pnfntst_make_name(this_test_name_));
    std::chrono::milliseconds rel_time = Td;
    pubnub_res                result = PNR_STARTED;

    SENSE(pbp.subscribe(ch)).in(Td) == PNR_OK;

    auto futr = pbp.publish(ch, "\"Test M4\"");
    if(!futr.is_ready()) {
        pbp.cancel();
        EXPECT_TRUE(pubnub::wait_for(futr, rel_time, result));
        EXPECT_RESULT(futr, result) == PNR_CANCELLED;
    }
    else {
        EXPECT_RESULT(futr, futr.last_result()) == PNR_OK;
    }

    SENSE(pbp.publish(ch, "\"Test M4-2\"")).in(Td) == PNR_OK;
    SENSE(pbp.subscribe(ch)).in(Td) == PNR_OK;
/* On 'Qt' message published on a simple channel reaches its destination
 * even though the transaction is canceled.
 * Somehow it's quite probable that 'that' won't happen on posix.
 */
#if (!defined(INC_PUBNUB_QT) || !defined(_WIN32))
    if (PNR_CANCELLED == result) {
        EXPECT_TRUE(got_messages(pbp, {"\"Test M4-2\""}));
    }
    else {
#endif
        EXPECT_TRUE(got_messages(pbp, {"\"Test M4\"", "\"Test M4-2\""}));
#if (!defined(INC_PUBNUB_QT) || !defined(_WIN32))
    }
#endif
    pbp.set_blocking_io(non_blocking);
    auto futr_2 = pbp.subscribe(ch);
    pbp.cancel();
    rel_time = Td;
    result = PNR_STARTED;
    EXPECT_TRUE(pubnub::wait_for(futr_2, rel_time, result));
    EXPECT_RESULT(futr_2, result) == PNR_CANCELLED;
    
    SENSE(pbp.publish(ch, "\"Test M4 - 3\"")).in(Td) == PNR_OK;
    SENSE(pbp.subscribe(ch)).in(Td) == PNR_OK;
    EXPECT_TRUE(got_messages(pbp, {"\"Test M4 - 3\""}));
}
TEST_ENDDEF

TEST_DEF_NEED_CHGROUP(connect_disconnect_and_connect_again_group)
{
    context                   pbp(pubkey, keysub, origin);
    std::string const         ch(pnfntst_make_name(this_test_name_));
    std::string const         gr(pnfntst_make_name(this_test_name_));
    std::chrono::milliseconds rel_time = Td;
    pubnub_res                result = PNR_STARTED;
    
    SENSE(pbp.remove_channel_group(gr)).in(Td) == PNR_OK;
    SENSE(pbp.add_channel_to_group(ch, gr)).in(Td) == PNR_OK;

    std::this_thread::sleep_for(T_chan_registry_propagation);
    
    SENSE(pbp.subscribe("", gr)).in(Td) == PNR_OK;
    
    auto futr = pbp.publish(ch, "\"Test M44\"");
    if(!futr.is_ready()) {
        pbp.cancel();
        EXPECT_TRUE(pubnub::wait_for(futr, rel_time, result));
        EXPECT_RESULT(futr, result) == PNR_CANCELLED;
    }
    else {
        EXPECT_RESULT(futr, futr.last_result()) == PNR_OK;
    }

    SENSE(pbp.publish(ch, "\"Test M4-2\"")).in(Td) == PNR_OK;
    SENSE(pbp.subscribe("", gr)).in(Td) == PNR_OK;
/* On 'Qt' message published on a simple channel reaches its destination
 * even though the transaction is canceled.
 * Somehow it's quite probable that 'that' won't happen on posix.
 */
#if (!defined(INC_PUBNUB_QT) || !defined(_WIN32))
    if (PNR_CANCELLED == result) {
        EXPECT_TRUE(got_messages(pbp, {"\"Test M4-2\""}));
    }
    else {
#endif
        EXPECT_TRUE(got_messages(pbp, {"\"Test M44\"", "\"Test M4-2\""}));
#if (!defined(INC_PUBNUB_QT) || !defined(_WIN32))
    }
#endif

    pbp.set_blocking_io(non_blocking);
    auto futr_2 = pbp.subscribe("", gr);
    pbp.cancel();
    rel_time = Td;
    result = PNR_STARTED;
    EXPECT_TRUE(pubnub::wait_for(futr_2, rel_time, result));
    EXPECT_RESULT(futr_2, result) == PNR_CANCELLED;

    SENSE(pbp.publish(ch, "\"Test M4 - 3\"")).in(Td) == PNR_OK;
    SENSE(pbp.subscribe("", gr)).in(Td) == PNR_OK;
    EXPECT_TRUE(got_messages(pbp, {"\"Test M4 - 3\""}));

    SENSE(pbp.remove_channel_group(gr)).in(Td) == PNR_OK;
}
TEST_ENDDEF

TEST_DEF_NEED_CHGROUP(connect_disconnect_and_connect_again_combo)
{
    context                   pbp(pubkey, keysub, origin);
    context                   pbp_2(pubkey, keysub, origin);
    std::string const         ch(pnfntst_make_name(this_test_name_));
    std::string const         two(pnfntst_make_name(this_test_name_));
    std::string const         gr(pnfntst_make_name(this_test_name_));
    std::chrono::milliseconds rel_time = Td;
    pubnub_res                result_1 = PNR_STARTED;
    pubnub_res                result_2 = PNR_STARTED;

    SENSE(pbp.remove_channel_group(gr)).in(Td) == PNR_OK;
    SENSE(pbp.add_channel_to_group(ch, gr)).in(Td) == PNR_OK;

    std::this_thread::sleep_for(T_chan_registry_propagation);

    SENSE(pbp.subscribe("", gr)).in(Td) == PNR_OK;

    auto futr = pbp.publish(ch, "\"Test M4\"");
    if(!futr.is_ready()) {
        pbp.cancel();
        EXPECT_TRUE(pubnub::wait_for(futr, rel_time, result_1));
        EXPECT_RESULT(futr, result_1) == PNR_CANCELLED;
    }
    else {
        EXPECT_RESULT(futr, futr.last_result()) == PNR_OK;
    }

    auto futr_2 = pbp_2.publish(two, "\"Test M5\"");
    if(!futr_2.is_ready()) {
        pbp_2.cancel();
        EXPECT_TRUE(pubnub::wait_for(futr_2, rel_time, result_2));
        EXPECT_RESULT(futr_2, result_2) == PNR_CANCELLED;
    }
    else {
        EXPECT_RESULT(futr_2, futr_2.last_result()) == PNR_OK;
    }

    (SENSE(pbp.publish(ch, "\"Test M4-2\"")) && 
     SENSE(pbp_2.publish(two, "\"Test M5-2\""))
        ).in(Td) == PNR_OK;
    SENSE(pbp.subscribe(two, gr)).in(Td) == PNR_OK;
    if (PNR_CANCELLED == result_1) {
        EXPECT_TRUE(got_messages(pbp, {"\"Test M4-2\"", "\"Test M5-2\""}));
    }
    else {
        EXPECT_TRUE(got_messages(pbp, {"\"Test M4\"", "\"Test M4-2\"", "\"Test M5-2\""}));
    }

    pbp.set_blocking_io(non_blocking);

    auto futr_3 = pbp.publish(two, "msg_dz");
    if(!futr_3.is_ready()) {
        pbp.cancel();
        EXPECT_TRUE(pubnub::wait_for(futr_3, rel_time, result_1));
        EXPECT_RESULT(futr_3, result_1) == PNR_CANCELLED;
    }
    else {
        EXPECT_RESULT(futr_3, futr_3.last_result()) == PNR_OK;
    }

    SENSE(pbp.publish(ch, "\"Test M4-3\"")).in(Td) == PNR_OK;
    SENSE(pbp.publish(two, "\"Test M5-3\"")).in(Td) == PNR_OK;
    SENSE(pbp.subscribe(two, gr)).in(Td) == PNR_OK;
    EXPECT_TRUE(got_messages(pbp, {"\"Test M4-3\"", "\"Test M5-3\""}));

    SENSE(pbp.remove_channel_group(gr)).in(Td) == PNR_OK;
}
TEST_ENDDEF

TEST_DEF(wrong_api_usage)
{
    context                   pbp(pubkey, keysub, origin);
    std::string const         ch(pnfntst_make_name(this_test_name_));
    std::chrono::milliseconds rel_time = Td;
    pubnub_res                result = PNR_STARTED;
    
    SENSE(pbp.subscribe(ch)).in(Td) == PNR_OK;

    auto futr = pbp.publish(ch, "\"Test \"");
    if(!futr.is_ready()) {
        SENSE(pbp.publish(ch, "\"Test - 2\"")).in(Td) == PNR_IN_PROGRESS;
        SENSE(pbp.subscribe(ch)).in(Td) == PNR_IN_PROGRESS;
        EXPECT_TRUE(pubnub::wait_for(futr, rel_time, result));
        EXPECT_RESULT(futr, result) == PNR_OK;
    }
    else {
        EXPECT_RESULT(futr, futr.last_result()) == PNR_OK;
    }

    pbp.set_blocking_io(non_blocking);
    auto futr_2 = pbp.subscribe(ch);
    rel_time = Td;
    result = PNR_STARTED;
    SENSE(pbp.subscribe(ch)).in(Td) == PNR_IN_PROGRESS;
    SENSE(pbp.publish(ch, "\"Test - 2\"")).in(Td) == PNR_IN_PROGRESS;

    pbp.cancel();
    EXPECT_TRUE(pubnub::wait_for(futr_2, rel_time, result));
    EXPECT_RESULT(futr_2, result) == PNR_CANCELLED;

    SENSE(pbp.subscribe("")).in(Td) == PNR_INVALID_CHANNEL;
}
TEST_ENDDEF

TEST_DEF(handling_errors_from_pubnub)
{
    context           pbp(pubkey, keysub, origin);
    std::string const ch(pnfntst_make_name(this_test_name_));
    
    SENSE(pbp.publish(ch, "\"Test ")).in(Td) == PNR_PUBLISH_FAILED;
    EXPECT(pbp.last_http_code()) == 400;
    EXPECT(pbp.parse_last_publish_result()) == PNPUB_INVALID_JSON;
    
    SENSE(pbp.publish(",", "\"Test \"")).in(Td) == PNR_PUBLISH_FAILED;
    EXPECT(pbp.last_http_code()) == 400;
    EXPECT(pbp.parse_last_publish_result()) == PNPUB_INVALID_CHAR_IN_CHAN_NAME;
}
TEST_ENDDEF
