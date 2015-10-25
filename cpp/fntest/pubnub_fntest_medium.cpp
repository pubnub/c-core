/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_fntest_medium.hpp"
#include "pubnub_fntest.hpp"

#include <thread>


using namespace pubnub;

const std::chrono::seconds Td(5);
const std::chrono::milliseconds T_chan_registry_propagation(1000);


void complex_send_and_receive_over_several_channels_simultaneously(std::string const &pubkey, std::string const &keysub)
{
    context pbp(pubkey, keysub);
    context pbp_2(pubkey, keysub);

    SENSE(pbp.subscribe("two,three")).in(Td) == PNR_OK;
    SENSE(pbp_2.subscribe("ch")).in(Td) == PNR_OK;

    pbp_2.set_blocking_io(non_blocking);
    SENSE(pbp_2.publish("two", "\"Test M3\"")).in(Td) == PNR_OK;
    (SENSE(pbp.publish("ch", "\"Test M3\"")) &&
     SENSE(pbp_2.publish("three", "\"Test M3 - 1\""))
        ).in(Td) == PNR_OK;
     
    (SENSE(pbp.subscribe("two,three")) && SENSE(pbp_2.subscribe("ch"))
        ).in(Td) == PNR_OK;

    EXPECT(got_messages(pbp, {"\"Test M3\"", "\"Test M3 - 1\""})) == true;
    EXPECT(got_messages(pbp_2, {"\"Test M3\""})) == true;
}
