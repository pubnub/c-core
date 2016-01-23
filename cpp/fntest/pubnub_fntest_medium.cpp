/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_fntest_medium.hpp"
#include "pubnub_fntest.hpp"

#include <thread>


using namespace pubnub;

const std::chrono::seconds Td(5);
const std::chrono::milliseconds T_chan_registry_propagation(1000);


void complex_send_and_receive_over_several_channels_simultaneously(std::string const &pubkey, std::string const &keysub, std::string const &origin)
{
    context pbp(pubkey, keysub, origin);
    context pbp_2(pubkey, keysub, origin);

    SENSE(pbp.subscribe("two,three")).in(Td) == PNR_OK;
    SENSE(pbp_2.subscribe("ch")).in(Td) == PNR_OK;

    pbp_2.set_blocking_io(non_blocking);
    SENSE(pbp_2.publish("two", "\"Test M3\"")).in(Td) == PNR_OK;
    (SENSE(pbp.publish("ch", "\"Test M3\"")) &&
     SENSE(pbp_2.publish("three", "\"Test M3 - 1\""))
        ).in(Td) == PNR_OK;
     
    (SENSE(pbp.subscribe("two,three")) && SENSE(pbp_2.subscribe("ch"))
        ).in(Td) == PNR_OK;

    EXPECT_TRUE(got_messages(pbp, {"\"Test M3\"", "\"Test M3 - 1\""}));
    EXPECT_TRUE(got_messages(pbp_2, {"\"Test M3\""}));
}


void complex_send_and_receive_over_channel_plus_group_simultaneously(std::string const &pubkey, std::string const &keysub, std::string const &origin)
{
    context pbp(pubkey, keysub, origin);
    context pbp_2(pubkey, keysub, origin);

    SENSE(pbp.remove_channel_group("gr")).in(Td) == PNR_OK;
    SENSE(pbp.add_channel_to_group("two,three", "gr")).in(Td) == PNR_OK;

    std::this_thread::sleep_for(T_chan_registry_propagation);
    
    (SENSE(pbp.subscribe("", "gr")) && SENSE(pbp_2.subscribe("ch"))
        ).in(Td) == PNR_OK;
    
    SENSE(pbp.publish("two", "\"Test M3\"")).in(Td) == PNR_OK;
    (SENSE(pbp_2.publish("ch", "\"Test M3\"")) && 
        SENSE(pbp.publish("three", "\"Test M3 - 1\""))
        ).in(Td) == PNR_OK;
    
    (SENSE(pbp.subscribe("", "gr")) && SENSE(pbp_2.subscribe("ch"))
        ).in(Td) == PNR_OK;

    EXPECT_TRUE(got_messages(pbp, {"\"Test M3\"", "\"Test M3 - 1\""}));
    EXPECT_TRUE(got_messages(pbp_2, {"\"Test M3\""}));
    
    SENSE(pbp.remove_channel_from_group("two,three", "gr")).in(Td) == PNR_OK;
}


void connect_disconnect_and_connect_again(std::string const &pubkey, std::string const &keysub, std::string const &origin)
{
    context pbp(pubkey, keysub, origin);
    std::chrono::milliseconds rel_time = Td;
    pubnub_res result = PNR_STARTED;

    SENSE(pbp.subscribe("ch")).in(Td) == PNR_OK;

    auto futr = pbp.publish("ch", "\"Test M4\"");
    pbp.cancel();
    EXPECT_TRUE(pubnub::wait_for(futr, rel_time, result));
    EXPECT(result) == PNR_CANCELLED;

    SENSE(pbp.publish("ch", "\"Test M4 - 2\"")).in(Td) == PNR_OK;
    SENSE(pbp.subscribe("ch")).in(Td) == PNR_OK;
    EXPECT_TRUE(got_messages(pbp, {"\"Test M4 - 2\""}));

    pbp.set_blocking_io(non_blocking);
    auto futr_2 = pbp.subscribe("ch");
    pbp.cancel();
    rel_time = Td;
    result = PNR_STARTED;
    EXPECT_TRUE(pubnub::wait_for(futr_2, rel_time, result));
    EXPECT(result) == PNR_CANCELLED;
    
    SENSE(pbp.publish("ch", "\"Test M4 - 3\"")).in(Td) == PNR_OK;
    SENSE(pbp.subscribe("ch")).in(Td) == PNR_OK;
    EXPECT_TRUE(got_messages(pbp, {"\"Test M4 - 3\""}));
}


void connect_disconnect_and_connect_again_group(std::string const &pubkey, std::string const &keysub, std::string const &origin)
{
    context pbp(pubkey, keysub, origin);
    std::chrono::milliseconds rel_time = Td;
    pubnub_res result = PNR_STARTED;
    
    SENSE(pbp.remove_channel_group("gr")).in(Td) == PNR_OK;
    SENSE(pbp.add_channel_to_group("ch", "gr")).in(Td) == PNR_OK;

    std::this_thread::sleep_for(T_chan_registry_propagation);
    
    SENSE(pbp.subscribe("", "gr")).in(Td) == PNR_OK;
    
    auto futr = pbp.publish("ch", "\"Test M4\"");
    pbp.cancel();
    EXPECT_TRUE(pubnub::wait_for(futr, rel_time, result));
    EXPECT(result) == PNR_CANCELLED;

    SENSE(pbp.publish("ch", "\"Test M4 - 2\"")).in(Td) == PNR_OK;
    SENSE(pbp.subscribe("", "gr")).in(Td) == PNR_OK;
    EXPECT_TRUE(got_messages(pbp, {"\"Test M4 - 2\""}));

    pbp.set_blocking_io(non_blocking);
    auto futr_2 = pbp.subscribe("", "gr");
    pbp.cancel();
    rel_time = Td;
    result = PNR_STARTED;
    EXPECT_TRUE(pubnub::wait_for(futr_2, rel_time, result));
    EXPECT(result) == PNR_CANCELLED;

    SENSE(pbp.publish("ch", "\"Test M4 - 3\"")).in(Td) == PNR_OK;
    SENSE(pbp.subscribe("", "gr")).in(Td) == PNR_OK;
    EXPECT_TRUE(got_messages(pbp, {"\"Test M4 - 3\""}));

    SENSE(pbp.remove_channel_from_group("ch", "gr")).in(Td) == PNR_OK;
}


void connect_disconnect_and_connect_again_combo(std::string const &pubkey, std::string const &keysub, std::string const &origin)
{
    context pbp(pubkey, keysub, origin);
    context pbp_2(pubkey, keysub, origin);
    std::chrono::milliseconds rel_time = Td;
    pubnub_res result = PNR_STARTED;

    SENSE(pbp.remove_channel_group("gr")).in(Td) == PNR_OK;
    SENSE(pbp.add_channel_to_group("ch", "gr")).in(Td) == PNR_OK;

    std::this_thread::sleep_for(T_chan_registry_propagation);

    SENSE(pbp.subscribe("", "gr")).in(Td) == PNR_OK;

    auto futr = pbp.publish("ch", "\"Test M4\"");
    pbp.cancel();
    EXPECT_TRUE(pubnub::wait_for(futr, rel_time, result));
    EXPECT(result) == PNR_CANCELLED;

    auto futr_2 = pbp_2.publish("two", "\"Test M5\"");
    pbp_2.cancel();
    EXPECT_TRUE(pubnub::wait_for(futr_2, rel_time, result));
    EXPECT(result) == PNR_CANCELLED;

    (SENSE(pbp.publish("ch", "\"Test M4 - 2\"")) && 
     SENSE(pbp_2.publish("two", "\"Test M5 - 2\""))
        ).in(Td) == PNR_OK;
    SENSE(pbp.subscribe("two", "gr")).in(Td) == PNR_OK;
    EXPECT_TRUE(got_messages(pbp, {"\"Test M4 - 2\"", "\"Test M5 - 2\""}));

    pbp.set_blocking_io(non_blocking);

    auto futr_3 = pbp.publish("two", "gr");
    pbp.cancel();
    EXPECT_TRUE(pubnub::wait_for(futr_3, rel_time, result));
    EXPECT(result) == PNR_CANCELLED;

    SENSE(pbp.publish("ch", "\"Test M4 - 3\"")).in(Td) == PNR_OK;
    SENSE(pbp.publish("two", "\"Test M5 - 3\"")).in(Td) == PNR_OK;
    SENSE(pbp.subscribe("two", "gr")).in(Td) == PNR_OK;
    EXPECT_TRUE(got_messages(pbp, {"\"Test M4 - 3\"", "\"Test M5 - 3\""}));

    SENSE(pbp.remove_channel_from_group("ch", "gr")).in(Td) == PNR_OK;
}


void wrong_api_usage(std::string const &pubkey, std::string const &keysub, std::string const &origin)
{
    context pbp(pubkey, keysub, origin);
    std::chrono::milliseconds rel_time = Td;
    pubnub_res result = PNR_STARTED;
    
    SENSE(pbp.subscribe("ch")).in(Td) == PNR_OK;

    auto futr = pbp.publish("ch", "\"Test \"");
    SENSE(pbp.publish("ch", "\"Test - 2\"")).in(Td) == PNR_IN_PROGRESS;
    SENSE(pbp.subscribe("ch")).in(Td) == PNR_IN_PROGRESS;
    EXPECT_TRUE(pubnub::wait_for(futr, rel_time, result));
    EXPECT(result) == PNR_OK;

    pbp.set_blocking_io(non_blocking);
    auto futr_2 = pbp.subscribe("ch");
    rel_time = Td;
    result = PNR_STARTED;
    SENSE(pbp.subscribe("ch")).in(Td) == PNR_IN_PROGRESS;
    SENSE(pbp.publish("ch", "\"Test - 2\"")).in(Td) == PNR_IN_PROGRESS;

    pbp.cancel();
    EXPECT_TRUE(pubnub::wait_for(futr_2, rel_time, result));
    EXPECT(result) == PNR_CANCELLED;

    SENSE(pbp.subscribe("")).in(Td) == PNR_INVALID_CHANNEL;
}


void handling_errors_from_pubnub(std::string const &pubkey, std::string const &keysub, std::string const &origin)
{
    context pbp(pubkey, keysub, origin);
    
    SENSE(pbp.publish("ch", "\"Test ")).in(Td) == PNR_PUBLISH_FAILED;
    EXPECT(pbp.last_http_code()) == 400;
    EXPECT(pbp.parse_last_publish_result()) == PNPUB_INVALID_JSON;
    
    SENSE(pbp.publish(",", "\"Test \"")).in(Td) == PNR_PUBLISH_FAILED;
    EXPECT(pbp.last_http_code()) == 400;
    EXPECT(pbp.parse_last_publish_result()) == PNPUB_INVALID_CHAR_IN_CHAN_NAME;
}

