/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub.hpp"


using namespace pubnub;


subloop::subloop(context &ctx, std::string channel) 
    : d_ctx(ctx)
    , d_channel(channel)
    , d_delivering(false)
{
}


subloop::~subloop()
{
}


pubnub_res subloop::fetch(std::string &o_msg)
{
    if (d_delivering) {
        o_msg = d_ctx.get();
        d_delivering = !o_msg.empty();
        return PNR_OK;
    }

    futres ftr = d_ctx.subscribe(d_channel);
    pubnub_res pbres = ftr.await();
    if (PNR_OK == pbres) {
        o_msg = d_ctx.get();
        d_delivering = !o_msg.empty();
    }

    return pbres;
}


#if __cplusplus >= 201103L

void subloop::loop(std::function<int(std::string, context&, pubnub_res)> f)
{
    std::string msg;
    while (0 == f(msg, d_ctx, fetch(msg))) {
        continue;
    }
}

#endif
