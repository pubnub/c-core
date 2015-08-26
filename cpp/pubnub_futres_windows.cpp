/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub.hpp"

//extern "C" {
#include "pubnub_ntf_callback.h"
//}

#include <windows.h>

#include <stdexcept>


namespace pubnub {

class futres::impl {
public:
    impl() : d_wevent(CreateEvent(NULL, TRUE, FALSE, NULL)) {
    }
    void start_await() {
        ResetEvent(d_wevent);
    }
    void end_await() {
        WaitForSingleObject(d_wevent, INFINITE);
    }
    void signal() {
        SetEvent(d_wevent);
    }
    bool is_ready() const {
        return WAIT_OBJECT_0 == WaitForSingleObject(d_wevent, 0);
    }

private:
    HANDLE d_wevent;
};


static void futres_callback(pubnub_t *pb, enum pubnub_trans trans, enum pubnub_res result, void *user_data)
{
    futres::impl *p = static_cast<futres::impl*>(user_data);
    p->signal();
}


futres::futres(pubnub_t *pb, context &ctx, pubnub_res initial) :
    d_pb(pb), d_ctx(ctx), d_result(initial), d_pimpl(new impl)
{
    if (PNR_OK != pubnub_register_callback(d_pb, futres_callback, d_pimpl)) {
        throw std::logic_error("Failed to register callback");
    }
}


#if _cplusplus < 201103L
futres::futres(futres const &x) :
	d_pb(x.d_pb), d_ctx(x.d_ctx), d_result(x.d_result), d_pimpl(new impl)
{
    if (PNR_OK != pubnub_register_callback(d_pb, futres_callback, d_pimpl)) {
        throw std::logic_error("Failed to register callback");
    }
}
#endif


futres::~futres()
{
    delete d_pimpl;
}


pubnub_res futres::last_result()
{
    return pubnub_last_result(d_pb);
}

 
void futres::start_await()
{
    d_pimpl->start_await();
}

pubnub_res futres::end_await()
{
    d_pimpl->end_await();
    return pubnub_last_result(d_pb);
}


bool futres::valid() const
{
    return (d_pb != NULL) && (d_pimpl != NULL);
}


bool futres::is_ready() const
{
    return d_pimpl->is_ready();
}

}
