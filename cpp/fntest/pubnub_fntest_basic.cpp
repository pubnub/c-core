/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_fntest_basic.hpp"

#include <thread>

using namespace pubnub;

const std::chrono::seconds Td(5);
const std::chrono::milliseconds T_chan_registry_propagation(1000);


TEST_DEF(simple_connect_and_send_over_single_channel)
{
    context           pb(pubkey, keysub, origin);
    std::string const chan(pnfntst_make_name(this_test_name_));

    SENSE(pb.subscribe(chan)).in(Td) == PNR_OK;

    SENSE(pb.publish(chan, "\"Test 1\"")).in(Td) == PNR_OK;
    SENSE(pb.publish(chan, "\"Test 1-2\"")).in(Td) == PNR_OK;

    SENSE(pb.subscribe(chan)).in(Td) == PNR_OK;
    EXPECT(got_messages(pb, {"\"Test 1\"", "\"Test 1-2\""})) == true;
}
TEST_ENDDEF

TEST_DEF(connect_and_send_over_several_channels_simultaneously)
{
    context           pb(pubkey, keysub, origin);
    std::string const chan_1st(pnfntst_make_name(this_test_name_));
    std::string const chan_2nd(pnfntst_make_name(this_test_name_));

    SENSE(pb.subscribe(chan_1st)).in(Td) == PNR_OK;

    SENSE(pb.publish(chan_1st, "\"Test M1\"")).in(Td) == PNR_OK;
    SENSE(pb.publish(chan_2nd, "\"Test M2\"")).in(Td) == PNR_OK;

    SENSE(pb.subscribe(chan_1st)).in(Td) == PNR_OK;
    EXPECT(got_messages(pb, {"\"Test M1\""})) == true;

    SENSE(pb.subscribe(chan_2nd)).in(Td) == PNR_OK;
    EXPECT(got_messages(pb, {"\"Test M2\""})) == true;
}
TEST_ENDDEF

TEST_DEF_NEED_CHGROUP(simple_connect_and_send_over_single_channel_in_group)
{
    context           pb(pubkey, keysub, origin);
    std::string const ch(pnfntst_make_name(this_test_name_));
    std::string const gr(pnfntst_make_name(this_test_name_));
    
    SENSE(pb.remove_channel_group(gr)).in(Td) == PNR_OK;
    SENSE(pb.add_channel_to_group(ch, gr)).in(Td) == PNR_OK;;
    
    std::this_thread::sleep_for(T_chan_registry_propagation);
    
    SENSE(pb.subscribe("", gr)).in(Td) == PNR_OK;

    SENSE(pb.publish(ch, "\"Test chg 1\"")).in(Td) == PNR_OK;
    SENSE(pb.publish(ch, "\"Test chg 1-2\"")).in(Td) == PNR_OK;

    SENSE(pb.subscribe("", gr)).in(Td) == PNR_OK;
    EXPECT(got_messages(pb, {"\"Test chg 1\"", "\"Test chg 1-2\""})) == true;

    SENSE(pb.remove_channel_group(gr)).in(Td) == PNR_OK;
}
TEST_ENDDEF

TEST_DEF_NEED_CHGROUP(connect_and_send_over_several_channels_in_group_simultaneously)
{
    context           pbp(pubkey, keysub, origin);
    std::string const ch(pnfntst_make_name(this_test_name_));
    std::string const two(pnfntst_make_name(this_test_name_));
    std::string const gr(pnfntst_make_name(this_test_name_));
    std::string       ch_two = ch + comma + two;

    SENSE(pbp.remove_channel_group(gr)).in(Td) == PNR_OK;
    SENSE(pbp.add_channel_to_group(ch_two, gr)).in(Td) == PNR_OK;

    std::this_thread::sleep_for(T_chan_registry_propagation);

    SENSE(pbp.subscribe("", gr)).in(Td) == PNR_OK;

    SENSE(pbp.publish(ch, "\"Test M1\"")).in(Td) == PNR_OK;
    SENSE(pbp.publish(two, "\"Test M2\"")).in(Td) == PNR_OK;

    EXPECT_TRUE(subscribe_and_check(pbp, "", gr, std::chrono::seconds(5), {{"\"Test M1\"", ch.c_str()}, {"\"Test M2\"", two.c_str()}}));

    SENSE(pbp.remove_channel_group(gr)).in(Td) == PNR_OK;
}
TEST_ENDDEF

TEST_DEF_NEED_CHGROUP(connect_and_send_over_channel_in_group_and_single_channel_simultaneously)
{
    context           pbp(pubkey, keysub, origin);
    std::string const ch(pnfntst_make_name(this_test_name_));
    std::string const two(pnfntst_make_name(this_test_name_));
    std::string const gr(pnfntst_make_name(this_test_name_));

    SENSE(pbp.remove_channel_group(gr)).in(Td) == PNR_OK;
    SENSE(pbp.add_channel_to_group(ch, gr)).in(Td) == PNR_OK;

    std::this_thread::sleep_for(T_chan_registry_propagation);
	
    SENSE(pbp.subscribe(two, gr)).in(Td) == PNR_OK;

    SENSE(pbp.publish(ch, "\"Test M1\"")).in(Td) == PNR_OK;
    SENSE(pbp.publish(two, "\"Test M2\"")).in(Td) == PNR_OK;

    EXPECT_TRUE(subscribe_and_check(pbp, two, gr, std::chrono::seconds(5), {{"\"Test M1\"", ch.c_str()}, {"\"Test M2\"", two.c_str()}}));

    SENSE(pbp.remove_channel_group(gr)).in(Td) == PNR_OK;
}
TEST_ENDDEF

TEST_DEF_NEED_CHGROUP(connect_and_send_over_channel_in_group_and_multi_channel_simultaneously)
{
    context           pbp(pubkey, keysub, origin);
    std::string const ch(pnfntst_make_name(this_test_name_));
    std::string const two(pnfntst_make_name(this_test_name_));
    std::string const three(pnfntst_make_name(this_test_name_));
    std::string const gr(pnfntst_make_name(this_test_name_));
    std::string       ch_two = ch + comma + two;

    SENSE(pbp.remove_channel_group(gr)).in(Td) == PNR_OK;
    SENSE(pbp.add_channel_to_group(three, gr)).in(Td) == PNR_OK;

    std::this_thread::sleep_for(T_chan_registry_propagation);

    SENSE(pbp.subscribe(ch_two, gr)).in(Td) == PNR_OK;

    SENSE(pbp.publish(ch, "\"Test M1\"")).in(Td) == PNR_OK;
    SENSE(pbp.publish(two, "\"Test M1\"")).in(Td) == PNR_OK;
    SENSE(pbp.publish(three, "\"Test M1\"")).in(Td) == PNR_OK;

    EXPECT_TRUE(subscribe_and_check(pbp, ch_two, gr, std::chrono::seconds(5), {{"\"Test M1\"", ch.c_str()}, {"\"Test M1\"", two.c_str()}, {"\"Test M1\"", three.c_str()}}));

    SENSE(pbp.remove_channel_group(gr)).in(Td) == PNR_OK;
}
TEST_ENDDEF

TEST_DEF(simple_connect_and_receiver_over_single_channel)
{
    context           pbp(pubkey, keysub, origin);
    context           pbp_2(pubkey, keysub, origin);
    std::string const ch(pnfntst_make_name(this_test_name_));

    SENSE(pbp_2.subscribe(ch)).in(Td) == PNR_OK;

    pbp_2.set_blocking_io(non_blocking);
    (SENSE(pbp_2.subscribe(ch)) && SENSE(pbp.publish(ch, "\"Test 2\""))
        ).in(Td) == PNR_OK;

    EXPECT_TRUE(got_messages(pbp_2, {"\"Test 2\""}));

    SENSE(pbp.publish(ch, "\"Test 2 - 2\"")).in(Td) == PNR_OK;

    SENSE(pbp_2.subscribe(ch)).in(Td) == PNR_OK;

    EXPECT_TRUE(got_messages(pbp_2, {"\"Test 2 - 2\""}));
}
TEST_ENDDEF

TEST_DEF(connect_and_receive_over_several_channels_simultaneously)
{
    context           pbp(pubkey, keysub, origin);
    context           pbp_2(pubkey, keysub, origin);
    std::string const ch(pnfntst_make_name(this_test_name_));
    std::string const two(pnfntst_make_name(this_test_name_));
    std::string       ch_two = ch + comma + two;

    SENSE(pbp_2.subscribe(ch_two)).in(Td) == PNR_OK;

    SENSE(pbp.publish(ch, "\"Test M2\"")).in(Td) == PNR_OK;
    SENSE(pbp.publish(two, "\"Test M2-2\"")).in(Td) == PNR_OK;
    SENSE(pbp_2.subscribe(ch_two)).in(Td) == PNR_OK;

    EXPECT_TRUE(got_messages(pbp_2, {"\"Test M2\"", "\"Test M2-2\""}));
}
TEST_ENDDEF

TEST_DEF_NEED_CHGROUP(simple_connect_and_receiver_over_single_channel_in_group)
{
    context           pbp(pubkey, keysub, origin);
    context           pbp_2(pubkey, keysub, origin);
    std::string const ch(pnfntst_make_name(this_test_name_));
    std::string const gr(pnfntst_make_name(this_test_name_));

    SENSE(pbp_2.remove_channel_group(gr)).in(Td) == PNR_OK;
    SENSE(pbp_2.add_channel_to_group(ch, gr)).in(Td) == PNR_OK;

    std::this_thread::sleep_for(T_chan_registry_propagation);

    SENSE(pbp_2.subscribe("", gr)).in(Td) == PNR_OK;

    pbp_2.set_blocking_io(non_blocking);
    (SENSE(pbp_2.subscribe("", gr)) && SENSE(pbp.publish(ch, "\"Test 2\""))
        ).in(Td) == PNR_OK;

    EXPECT_TRUE(got_messages(pbp_2, {"\"Test 2\""}));

    SENSE(pbp.publish(ch, "\"Test 2 - 2\"")).in(Td) == PNR_OK;
    SENSE(pbp_2.subscribe("", gr)).in(Td) == PNR_OK;

    EXPECT_TRUE(got_messages(pbp_2, {"\"Test 2 - 2\""}));

    SENSE(pbp.remove_channel_group(gr)).in(Td) == PNR_OK;
}
TEST_ENDDEF

TEST_DEF_NEED_CHGROUP(connect_and_receive_over_several_channels_in_group_simultaneously)
{
    context           pbp(pubkey, keysub, origin);
    context           pbp_2(pubkey, keysub, origin);
    std::string const ch(pnfntst_make_name(this_test_name_));
    std::string const two(pnfntst_make_name(this_test_name_));
    std::string const gr(pnfntst_make_name(this_test_name_));
    std::string       ch_two = ch + comma + two;

    SENSE(pbp_2.remove_channel_group(gr)).in(Td) == PNR_OK;
    SENSE(pbp_2.add_channel_to_group(ch_two, gr)).in(Td) == PNR_OK;

    std::this_thread::sleep_for(T_chan_registry_propagation);

    SENSE(pbp_2.subscribe("", gr)).in(Td) == PNR_OK;

    pbp_2.set_blocking_io(non_blocking);
    SENSE(pbp.publish(ch, "\"Test M2\"")).in(Td) == PNR_OK;
    SENSE(pbp.publish(two, "\"Test M2-2\"")).in(Td) == PNR_OK;
    
    EXPECT_TRUE(subscribe_and_check(pbp_2, "", gr, std::chrono::seconds(5), {{"\"Test M2\"", ch.c_str()}, {"\"Test M2-2\"", two.c_str()}}));

    SENSE(pbp_2.remove_channel_group(gr)).in(Td) == PNR_OK;
}
TEST_ENDDEF

TEST_DEF_NEED_CHGROUP(connect_and_receive_over_channel_in_group_and_single_channel_simultaneously)
{
    context           pbp(pubkey, keysub, origin);
    context           pbp_2(pubkey, keysub, origin);
    std::string const ch(pnfntst_make_name(this_test_name_));
    std::string const two(pnfntst_make_name(this_test_name_));
    std::string const gr(pnfntst_make_name(this_test_name_));

    SENSE(pbp.remove_channel_group(gr)).in(Td) == PNR_OK;
    SENSE(pbp.add_channel_to_group(ch, gr)).in(Td) == PNR_OK;

    std::this_thread::sleep_for(T_chan_registry_propagation);

    SENSE(pbp_2.subscribe(two, gr)).in(Td) == PNR_OK;

    pbp_2.set_blocking_io(non_blocking);
    SENSE(pbp.publish(ch, "\"Test M2\"")).in(Td) == PNR_OK;
    SENSE(pbp.publish(two, "\"Test M2-2\"")).in(Td) == PNR_OK;

    EXPECT_TRUE(subscribe_and_check(pbp_2, two, gr, std::chrono::seconds(5), {{"\"Test M2\"", ch.c_str()}, {"\"Test M2-2\"", two.c_str()}}));

    SENSE(pbp.remove_channel_group(gr)).in(Td) == PNR_OK;
}
TEST_ENDDEF

TEST_DEF_NEED_CHGROUP(connect_and_receive_over_channel_in_group_and_multi_channel_simultaneously)
{
    context           pbp(pubkey, keysub, origin);
    context           pbp_2(pubkey, keysub, origin);
    std::string const ch(pnfntst_make_name(this_test_name_));
    std::string const two(pnfntst_make_name(this_test_name_));
    std::string const three(pnfntst_make_name(this_test_name_));
    std::string const gr(pnfntst_make_name(this_test_name_));
    std::string       ch_two = ch + comma + two;

    SENSE(pbp_2.remove_channel_group(gr)).in(Td) == PNR_OK;
    SENSE(pbp_2.add_channel_to_group(three, gr)).in(Td) == PNR_OK;

    std::this_thread::sleep_for(T_chan_registry_propagation);

    SENSE(pbp_2.subscribe(ch_two, gr)).in(Td) == PNR_OK;

    pbp_2.set_blocking_io(non_blocking);
    SENSE(pbp.publish(ch, "\"Test M2\"")).in(Td) == PNR_OK;
    SENSE(pbp.publish(two, "\"Test M2-2\"")).in(Td) == PNR_OK;
    SENSE(pbp.publish(three, "\"Test M2-3\"")).in(Td) == PNR_OK;

    EXPECT_TRUE(subscribe_and_check(pbp_2, ch_two, gr, std::chrono::seconds(5), {{"\"Test M2\"", ch.c_str()}, {"\"Test M2-2\"", two.c_str()}, {"\"Test M2-3\"", three.c_str()}}));

    SENSE(pbp_2.remove_channel_group(gr)).in(Td) == PNR_OK;
}
TEST_ENDDEF

TEST_DEF(broken_connection_test)
{
    context           pbp(pubkey, keysub, origin);
    std::string const ch(pnfntst_make_name(this_test_name_));

    SENSE(pbp.subscribe(ch)).in(Td) == PNR_OK;

    SENSE(pbp.publish(ch, "\"Test 3\"")).in(Td) == PNR_OK;
    SENSE(pbp.subscribe(ch)).in(Td) == PNR_OK;
    EXPECT_TRUE(got_messages(pbp, {"\"Test 3\""}));

    SENSE(pbp.publish(ch, "\"Test 3 - 2\"")).in(Td) == PNR_OK;
    std::cout << "Please disconnect from Internet. Press Enter when done." << std::endl;
    await_console();
    SENSE(pbp.subscribe(ch)).in(Td) == PNR_ADDR_RESOLUTION_FAILED;
    std::cout << "Please reconnect to Internet. Press Enter when done." << std::endl;
    await_console();
    SENSE(pbp.subscribe(ch)).in(Td) == PNR_OK;
    EXPECT_TRUE(got_messages(pbp, {"\"Test 3 - 2\""}));

    std::cout << "Please disconnect from Internet. Press Enter when done." << std::endl;
    await_console();
    SENSE(pbp.publish(ch, "\"Test 3 - 3\"")).in(Td) == PNR_OK;

    std::cout << "Please reconnect to Internet. Press Enter when done." << std::endl;
    await_console();
    SENSE(pbp.publish(ch, "\"Test 3 - 4\"")).in(Td) == PNR_OK;
    SENSE(pbp.subscribe(ch)).in(Td) == PNR_OK;
    EXPECT_TRUE(got_messages(pbp, {"\"Test 3 - 4\""}));
}
TEST_ENDDEF

TEST_DEF(broken_connection_test_multi)
{
    context           pbp(pubkey, keysub, origin);
    std::string const ch(pnfntst_make_name(this_test_name_));
    std::string const two(pnfntst_make_name(this_test_name_));
    std::string       ch_two = ch + comma + two;

    SENSE(pbp.subscribe(ch_two)).in(Td) == PNR_OK;

    SENSE(pbp.publish(ch, "\"Test 3\"")).in(Td) == PNR_OK;
    SENSE(pbp.publish(two, "\"Test 4\"")).in(Td) == PNR_OK;
    SENSE(pbp.subscribe(ch_two)).in(Td) == PNR_OK;
    EXPECT_TRUE(got_messages(pbp, {"\"Test 3\"", "\"Test 4\""}));

    SENSE(pbp.publish(ch, "\"Test 3 - 2\"")).in(Td) == PNR_OK;
    SENSE(pbp.publish(ch, "\"Test 4 - 2\"")).in(Td) == PNR_OK;
    std::cout << "Please disconnect from Internet. Press Enter when done." << std::endl;
    await_console();
    SENSE(pbp.subscribe(ch_two)).in(Td) == PNR_ADDR_RESOLUTION_FAILED;
    std::cout << "Please reconnect to Internet. Press Enter when done." << std::endl;
    await_console();
    SENSE(pbp.subscribe(ch_two)).in(Td) == PNR_OK;
    EXPECT_TRUE(got_messages(pbp, {"\"Test 3 - 2\"", "\"Test 4 - 2\""}));

    SENSE(pbp.publish(ch, "\"Test 3 - 4\"")).in(Td) == PNR_OK;
    SENSE(pbp.publish(ch, "\"Test 4 - 4\"")).in(Td) == PNR_OK;
    SENSE(pbp.subscribe(ch_two)).in(Td) == PNR_OK;
    EXPECT_TRUE(got_messages(pbp, {"\"Test 3 - 4\"", "\"Test 4 - 4\""}));
}
TEST_ENDDEF

TEST_DEF_NEED_CHGROUP(broken_connection_test_group)
{
    context           pbp(pubkey, keysub, origin);
    std::string const ch(pnfntst_make_name(this_test_name_));
    std::string const gr(pnfntst_make_name(this_test_name_));

    SENSE(pbp.remove_channel_group(gr)).in(Td) == PNR_OK;
    SENSE(pbp.add_channel_to_group(ch, gr)).in(Td) == PNR_OK;

    std::this_thread::sleep_for(T_chan_registry_propagation);

    SENSE(pbp.subscribe("", gr)).in(Td) == PNR_OK;

    SENSE(pbp.publish(ch, "\"Test 3\"")).in(Td) == PNR_OK;
    SENSE(pbp.subscribe("", gr)).in(Td) == PNR_OK;
    EXPECT_TRUE(got_messages(pbp, {"\"Test 3\""}));

    SENSE(pbp.publish(ch, "\"Test 3 - 2\"")).in(Td) == PNR_OK;
    std::cout << "Please disconnect from Internet. Press Enter when done." << std::endl;
    await_console();
    SENSE(pbp.subscribe("", gr)).in(Td) == PNR_ADDR_RESOLUTION_FAILED;
    std::cout << "Please reconnect to Internet. Press Enter when done." << std::endl;
    await_console();
    SENSE(pbp.subscribe("", gr)).in(Td) == PNR_OK;
    EXPECT_TRUE(got_messages(pbp, {"\"Test 3 - 2\""}));

    std::cout << "Please disconnect from Internet. Press Enter when done." << std::endl;
    await_console();
    SENSE(pbp.publish(ch, "\"Test 3 - 3\"")).in(Td) == PNR_OK;
    std::cout << "Please reconnect to Internet. Press Enter when done." << std::endl;
    await_console();
    SENSE(pbp.publish(ch, "\"Test 3 - 4\"")).in(Td) == PNR_OK;
    SENSE(pbp.subscribe("", gr)).in(Td) == PNR_OK;
    EXPECT_TRUE(got_messages(pbp, {"\"Test 3 - 4\""}));

    SENSE(pbp.remove_channel_group(gr)).in(Td) == PNR_OK;
}
TEST_ENDDEF

TEST_DEF_NEED_CHGROUP(broken_connection_test_multi_in_group)
{
    context           pbp(pubkey, keysub, origin);
    std::string const ch(pnfntst_make_name(this_test_name_));
    std::string const two(pnfntst_make_name(this_test_name_));
    std::string const gr(pnfntst_make_name(this_test_name_));
    std::string       ch_two = ch + comma + two;

    SENSE(pbp.remove_channel_group(gr)).in(Td) == PNR_OK;
    SENSE(pbp.add_channel_to_group(ch_two, gr)).in(Td) == PNR_OK;

    std::this_thread::sleep_for(T_chan_registry_propagation);

    SENSE(pbp.subscribe(ch_two)).in(Td) == PNR_OK;

    SENSE(pbp.publish(ch, "\"Test 3\"")).in(Td) == PNR_OK;
    SENSE(pbp.publish(two, "\"Test 4\"")).in(Td) == PNR_OK;
    SENSE(pbp.subscribe(ch_two)).in(Td) == PNR_OK;
    EXPECT_TRUE(got_messages(pbp, {"\"Test 3\"", "\"Test 4\""}));

    SENSE(pbp.publish(ch, "\"Test 3 - 2\"")).in(Td) == PNR_OK;
    SENSE(pbp.publish(two, "\"Test 4 - 2\"")).in(Td) == PNR_OK;

    std::cout << "Please disconnect from Internet. Press Enter when done." << std::endl;
    await_console();
    SENSE(pbp.subscribe(ch_two)).in(Td) == PNR_ADDR_RESOLUTION_FAILED;
    std::cout << "Please reconnect to Internet. Press Enter when done." << std::endl;
    await_console();

    SENSE(pbp.subscribe(ch_two)).in(Td) == PNR_OK;
    EXPECT_TRUE(got_messages(pbp, {"\"Test 3 - 2\"", "\"Test 4 - 2\""}));

    SENSE(pbp.publish(ch, "\"Test 3 - 4\"")).in(Td) == PNR_OK;
    SENSE(pbp.publish(two, "\"Test 4 - 4\"")).in(Td) == PNR_OK;
    SENSE(pbp.subscribe(ch_two)).in(Td) == PNR_OK;
    EXPECT_TRUE(got_messages(pbp, {"\"Test 3 - 4\"", "\"Test 4 - 4\""}));

    SENSE(pbp.remove_channel_group(gr)).in(Td) == PNR_OK;
}
TEST_ENDDEF

TEST_DEF_NEED_CHGROUP(broken_connection_test_group_in_group_out)
{
    context           pbp(pubkey, keysub, origin);
    std::string const ch(pnfntst_make_name(this_test_name_));
    std::string const two(pnfntst_make_name(this_test_name_));
    std::string const gr(pnfntst_make_name(this_test_name_));

    SENSE(pbp.remove_channel_group(gr)).in(Td) == PNR_OK;
    SENSE(pbp.add_channel_to_group(ch, gr)).in(Td) == PNR_OK;

    std::this_thread::sleep_for(T_chan_registry_propagation);

    SENSE(pbp.subscribe(two)).in(Td) == PNR_OK;

    SENSE(pbp.publish(ch, "\"Test 3\"")).in(Td) == PNR_OK;
    SENSE(pbp.publish(two, "\"Test 4\"")).in(Td) == PNR_OK;
    SENSE(pbp.subscribe(two, gr)).in(Td) == PNR_OK;
    EXPECT_TRUE(got_messages(pbp, {"\"Test 3\"", "\"Test 4\""}));

    SENSE(pbp.publish(ch, "\"Test 3 - 2\"")).in(Td) == PNR_OK;
    SENSE(pbp.publish(two, "\"Test 4 - 2\"")).in(Td) == PNR_OK;

    std::cout << "Please disconnect from Internet. Press Enter when done." << std::endl;
    await_console();
    SENSE(pbp.subscribe(two, gr)).in(Td) == PNR_ADDR_RESOLUTION_FAILED;
    std::cout << "Please reconnect to Internet. Press Enter when done." << std::endl;
    await_console();

    SENSE(pbp.subscribe(two, gr)).in(Td) == PNR_ADDR_RESOLUTION_FAILED;
    EXPECT_TRUE(got_messages(pbp, {"\"Test 3 - 2\"", "\"Test 4 - 2\""}));

    SENSE(pbp.publish(ch, "\"Test 3 - 4\"")).in(Td) == PNR_OK;
    SENSE(pbp.publish(two, "\"Test 4 - 4\"")).in(Td) == PNR_OK;
    SENSE(pbp.subscribe(two, gr)).in(Td) == PNR_ADDR_RESOLUTION_FAILED;
    EXPECT_TRUE(got_messages(pbp, {"\"Test 3 - 4\"", "\"Test 4 - 4\""}));

    SENSE(pbp.remove_channel_group(gr)).in(Td) == PNR_OK;
}
TEST_ENDDEF

TEST_DEF_NEED_CHGROUP(broken_connection_test_group_multichannel_out)
{
    context           pbp(pubkey, keysub, origin);
    std::string const ch(pnfntst_make_name(this_test_name_));
    std::string const two(pnfntst_make_name(this_test_name_));
    std::string const three(pnfntst_make_name(this_test_name_));
    std::string const gr(pnfntst_make_name(this_test_name_));
    std::string       ch_two = ch + comma + two;

    SENSE(pbp.remove_channel_group(gr)).in(Td) == PNR_OK;
    SENSE(pbp.add_channel_to_group(three, gr)).in(Td) == PNR_OK;

    std::this_thread::sleep_for(T_chan_registry_propagation);

    SENSE(pbp.subscribe(ch_two)).in(Td) == PNR_OK;

    SENSE(pbp.publish(ch, "\"Test 3\"")).in(Td) == PNR_OK;
    SENSE(pbp.publish(two, "\"Test 4\"")).in(Td) == PNR_OK;
    SENSE(pbp.publish(three, "\"Test 5\"")).in(Td) == PNR_OK;
    SENSE(pbp.subscribe(ch_two, gr)).in(Td) == PNR_OK;
    EXPECT_TRUE(got_messages(pbp, {"\"Test 3\"", "\"Test 4\"", "\"Test 5\""}));

    SENSE(pbp.publish(ch, "\"Test 3 - 2\"")).in(Td) == PNR_OK;
    SENSE(pbp.publish(two, "\"Test 4 - 2\"")).in(Td) == PNR_OK;
    SENSE(pbp.publish(three, "\"Test 5 - 2\"")).in(Td) == PNR_OK;

    std::cout << "Please disconnect from Internet. Press Enter when done." << std::endl;
    await_console();
    SENSE(pbp.subscribe(ch_two, "gr")).in(Td) == PNR_ADDR_RESOLUTION_FAILED;
    std::cout << "Please reconnect to Internet. Press Enter when done." << std::endl;
    await_console();

    SENSE(pbp.subscribe(ch_two, gr)).in(Td) == PNR_OK;
    EXPECT_TRUE(got_messages(pbp, {"\"Test 3 - 2\"", "\"Test 4 - 2\"", "\"Test 5 - 2\""}));

    SENSE(pbp.publish(ch, "\"Test 3 - 4\"")).in(Td) == PNR_OK;
    SENSE(pbp.publish(two, "\"Test 4 - 4\"")).in(Td) == PNR_OK;
    SENSE(pbp.publish(three, "\"Test 5 - 4\"")).in(Td) == PNR_OK;
    SENSE(pbp.subscribe(ch_two, gr)).in(Td) == PNR_OK;
    EXPECT_TRUE(got_messages(pbp, {"\"Test 3 - 4\"", "\"Test 4 - 4\"", "\"Test 5 - 4\""}));

    SENSE(pbp.remove_channel_group(gr)).in(Td) == PNR_OK;
}
TEST_ENDDEF
