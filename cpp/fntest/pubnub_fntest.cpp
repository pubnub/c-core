/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_fntest.hpp"

#include <chrono>


namespace pubnub {


std::vector<msg_on_chan> get_all_msg_on_chan(context const&pb)
{
    std::vector<msg_on_chan> result;
    for (;;) {
        msg_on_chan got(pb.get(), pb.get_channel());
        if (got.message().empty() && got.channel().empty()) {
            break;
        }
        result.emplace_back(got);
    }
    return result;
}


bool wait_for(futres &futr, std::chrono::milliseconds &rel_time, pubnub_res &pbresult)
{
    auto const t0 = std::chrono::system_clock::now();
    auto t_current = std::chrono::system_clock::now();
    while ((t_current - t0) < rel_time) {
        pbresult = futr.last_result();
        if (pbresult != PNR_STARTED) {
            rel_time -= std::chrono::duration_cast<std::chrono::milliseconds>(t_current - t0);
            return true;
        }
        t_current = std::chrono::system_clock::now();
    }
    return false;
}


bool subscribe_and_check(context &pb, std::string const&channel, const std::string &chgroup, std::chrono::milliseconds rel_time, std::vector<msg_on_chan> expected)
{
    std::sort(expected.begin(), expected.end());

    while (!expected.empty()) {
        pubnub_res pbresult;
        auto futr = pb.subscribe(channel, chgroup);
        if (!wait_for(futr, rel_time, pbresult)) {
            break;
        }
        if (pbresult != PNR_OK) {
            break;
        }
        auto got = get_all_msg_on_chan(pb);
        std::sort(got.begin(), got.end());
        inplace_set_difference(expected, got);
    }

    return expected.empty();
}


}
