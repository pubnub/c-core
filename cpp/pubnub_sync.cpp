/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub.hpp"

#include "pubnub_ntf_sync.h"
#include "pubnub_coreapi.h"


namespace pubnub {

futres::futres(pubnub_t *pb, context &ctx, pubnub_res initial) : 
    d_pb(pb), d_ctx(ctx), d_result(initial), d_pimpl(0) 
{
}


futres::~futres() 
{
}


pubnub_res futres::last_result()
{
  if (PNR_STARTED != d_result) {
    return d_result = pubnub_last_result(d_pb);
  }
  else {
    return d_result;
  }
}

 
pubnub_res futres::await()
{
  return d_result = pubnub_await(d_pb);
}


bool futres::valid() const
{
  return true;
}


bool futres::is_ready() const
{
  return d_result != PNR_STARTED;
}
 
   
}
