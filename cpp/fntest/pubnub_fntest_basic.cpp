/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_fntest_basic.hpp"
#include "pubnub_fntest.hpp"

#include <thread>


using namespace pubnub;

const std::chrono::seconds Td(5);
const std::chrono::milliseconds T_chan_registry_propagation(1000);


void simple_connect_and_send_over_single_channel(std::string const &pubkey, std::string const &keysub)
{
    context pb(pubkey, keysub);

    SENSE(pb.subscribe("ch")).in(Td) == PNR_OK;

    SENSE(pb.publish("ch", "\"Test 1\"")).in(Td) == PNR_OK;
    SENSE(pb.publish("ch", "\"Test 1 - 2\"")).in(Td) == PNR_OK;

    SENSE(pb.subscribe("ch")).in(Td) == PNR_OK;
    EXPECT(got_messages(pb, {"\"Test 1\"", "\"Test 1 - 2\""})) == true;
}


void connect_and_send_over_several_channels_simultaneously(std::string const &pubkey, std::string const &keysub)
{
    context pb(pubkey, keysub);

    SENSE(pb.subscribe("ch")).in(Td) == PNR_OK;

    SENSE(pb.publish("ch", "\"Test M1\"")).in(Td) == PNR_OK;
    SENSE(pb.publish("two", "\"Test M2\"")).in(Td) == PNR_OK;

    SENSE(pb.subscribe("ch")).in(Td) == PNR_OK;
    EXPECT(got_messages(pb, {"\"Test M1\""})) == true;

    SENSE(pb.subscribe("two")).in(Td) == PNR_OK;
    EXPECT(got_messages(pb, {"\"Test M2\""})) == true;
}


void simple_connect_and_send_over_single_channel_in_group(std::string const &pubkey, std::string const &keysub)
{
    context pb(pubkey, keysub);
    
    SENSE(pb.remove_channel_group("gr")).in(Td) == PNR_OK;
    SENSE(pb.add_channel_to_group("ch", "gr")).in(Td) == PNR_OK;;
    
    std::this_thread::sleep_for(T_chan_registry_propagation);
    
    SENSE(pb.subscribe("", "gr")).in(Td) == PNR_OK;

    SENSE(pb.publish("ch", "\"Test chg 1\"")).in(Td) == PNR_OK;
    SENSE(pb.publish("ch", "\"Test chg 1 - 2\"")).in(Td) == PNR_OK;

    SENSE(pb.subscribe("", "gr")).in(Td) == PNR_OK;
    EXPECT(got_messages(pb, {"\"Test chg 1\"", "\"Test chg 1 - 2\""})) == true;

    SENSE(pb.remove_channel_from_group("ch", "gr")).in(Td) == PNR_OK;
}
