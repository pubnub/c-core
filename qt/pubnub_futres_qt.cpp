/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub.hpp"

//extern "C" {
#include "pubnub_ntf_callback.h"
//}

#include <condition_variable>

#include <stdexcept>


namespace pubnub {

class futres::impl {
public:
    impl() : d_triggered(false) {
    }
    void start_await() {
        std::lock_guard<std::mutex> lk(d_mutex);
        d_triggered = false;
    }
    void end_await() {
        std::unique_lock<std::mutex> lk(d_mutex);
        d_cond.wait(lk, [&] { return d_triggered; });
    }
    void signal() {
        {
            std::lock_guard<std::mutex> lk(d_mutex);
            d_triggered = true;
        }
        d_cond.notify_one();
    }
    bool is_ready() const {
        std::lock_guard<std::mutex> lk(d_mutex);
        return d_triggered;
    }

private:
    mutable std::mutex d_mutex;
    bool d_triggered;
    std::condition_variable d_cond;
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


#if __cplusplus < 201103L
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
