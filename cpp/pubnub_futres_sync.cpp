/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub.hpp"

#if PUBNUB_USE_EXTERN_C
extern "C" {
#endif
#include "core/pubnub_ntf_sync.h"
#include "core/pubnub_coreapi.h"
#if PUBNUB_USE_EXTERN_C
}
#endif


namespace pubnub {

futres::futres(pubnub_t *pb, context &ctx, pubnub_res initial) : 
    d_pb(pb), d_ctx(ctx), d_result(initial), d_pimpl(0) 
{
}


#if __cplusplus < 201103L
futres::futres(futres const &x) :
    d_pb(x.d_pb), d_ctx(x.d_ctx), d_result(x.d_result), d_pimpl(0)
{
}
#endif


futres::~futres() 
{
}


pubnub_res futres::last_result()
{
    if (PNR_STARTED == d_result) {
        return d_result = pubnub_last_result(d_pb);
    }
    else {
        return d_result;
    }
}

void futres::start_await()
{
    // nothing to do to "start" a sync await...
}

 
pubnub_res futres::end_await()
{
    return d_result = pubnub_await(d_pb);
}


bool futres::valid() const
{
    return d_pb != 0;
}


bool futres::is_ready() const
{
  return d_result != PNR_STARTED;
}

pubnub_publish_res futres::parse_last_publish_result()
{
    return d_ctx.parse_last_publish_result();
}
 
#if (__cplusplus >= 201103L) || (_MSC_VER >= 1600)
void futres::then(std::function<void(context &, pubnub_res)> f)
{
    f(d_ctx, await());
}
#else
void futres::thenx(caller_keeper f)
{
    f(d_ctx, await());
}
#endif

}
