/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub.hpp"

extern "C" {
#include "pubnub_ntf_callback.h"
#include "pubnub_assert.h"
}

#include <thread>
#include <condition_variable>

#include <stdexcept>


namespace pubnub {

class futres::impl {
public:
    impl() : d_triggered(false), d_parent(nullptr) {
    }
    ~impl() {
        wait4_then_thread_to_start();
        wait4_then_thread_to_finish();
    }
    void start_await() {
        std::lock_guard<std::mutex> lk(d_mutex);
        d_triggered = false;
    }
    void end_await() {
        std::unique_lock<std::mutex> lk(d_mutex);
        d_cond.wait(lk, [&] { return d_triggered; });
    }
    void signal(pubnub_res rslt) {
        {
            std::lock_guard<std::mutex> lk(d_mutex);
            if (d_thenf && d_parent) {
                d_thread = std::thread([&]{ d_thenf(d_parent->d_ctx, rslt); });
            }
            d_triggered = true;
        }
        d_cond.notify_one();
        
    }
    bool is_ready() const {
        std::lock_guard<std::mutex> lk(d_mutex);
        return d_triggered;
    }
    void then(std::function<void(context &, pubnub_res)> f, futres *parent) {
        PUBNUB_ASSERT_OPT(parent != nullptr);
        std::lock_guard<std::mutex> lk(d_mutex);
        d_thenf = f;
        d_parent = parent;
    }
    
private:
    void wait4_then_thread_to_start() {
        auto should_await = [=] {
            std::lock_guard<std::mutex> lk(d_mutex);
            return !d_triggered && d_thenf && d_parent;
        }();
        if (should_await) {
            end_await();
        }
    }
    void wait4_then_thread_to_finish() {
        if (d_thread.joinable()) {
            d_thread.join();
        }
    }

    mutable std::mutex d_mutex;
    bool d_triggered;
    std::condition_variable d_cond;
    
    std::function<void(context&, pubnub_res)> d_thenf;
    futres *d_parent;
    std::thread d_thread;
};
    
    
static void futres_callback(pubnub_t *pb, enum pubnub_trans trans, enum pubnub_res result, void *user_data)
{
    futres::impl *p = static_cast<futres::impl*>(user_data);
    p->signal(result);
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
    (void)pubnub_register_callback(d_pb, NULL, NULL);
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

void futres::then(std::function<void(context &, pubnub_res)> f)
{
    d_pimpl->then(f, this);
}

}
