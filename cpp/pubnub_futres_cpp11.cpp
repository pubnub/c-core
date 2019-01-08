/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub.hpp"

#if PUBNUB_USE_EXTERN_C
extern "C" {
#endif
#include "core/pubnub_ntf_callback.h"
#include "core/pubnub_assert.h"
#if PUBNUB_USE_EXTERN_C
}
#endif

#include <thread>
#include <condition_variable>

#include <stdexcept>


namespace pubnub {

static void futres_callback(pubnub_t *pb,
                            enum pubnub_trans trans,
                            enum pubnub_res result,
                            void *user_data);

class futres::impl {
    friend class futres;
public:
    impl(pubnub_t *pb, pubnub_res initial) :
        d_triggered(false),
        d_pb(pb),
        d_parent(nullptr),
        d_result(initial) {
        if (initial != PNR_IN_PROGRESS) {
            std::unique_lock<std::mutex> lk(d_mutex);
            if (PNR_OK != pubnub_register_callback(d_pb, futres_callback, this)) {
                throw std::logic_error("Failed to register callback");
            }
        }
    }
    ~impl() {
        wait4_then_thread_to_start();
        wait4_then_thread_to_finish();
        (void)pubnub_register_callback(d_pb, NULL, NULL);
    }
    void start_await() {
        std::lock_guard<std::mutex> lk(d_mutex);
        d_triggered = false;
    }
    pubnub_res end_await() {
        std::unique_lock<std::mutex> lk(d_mutex);
        if (PNR_STARTED == d_result) {
            d_cond.wait(lk, [&] { return d_triggered; });
            return d_result = pubnub_last_result(d_pb);
        }
        else {
            return d_result;
        }
    }
    pubnub_res last_result() {
        std::unique_lock<std::mutex> lk(d_mutex);
        if (PNR_STARTED == d_result) {
            return d_result = pubnub_last_result(d_pb);
        }
        else {
            return d_result;
        }
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
    /// The C Pubnub context that we are "wrapping"
    pubnub_t* d_pb;
    
    std::function<void(context&, pubnub_res)> d_thenf;
    futres *d_parent;
    std::thread d_thread;
    pubnub_res d_result;
};
    
    
static void futres_callback(pubnub_t *pb,
                            enum pubnub_trans trans,
                            enum pubnub_res result,
                            void *user_data)
{
    futres::impl *p = static_cast<futres::impl*>(user_data);
    p->signal(result);
}


futres::futres(pubnub_t *pb, context &ctx, pubnub_res initial) :
    d_ctx(ctx), d_pimpl(new impl(pb, initial))
{
}
    

#if __cplusplus < 201103L
futres::futres(futres const &x) :
    d_ctx(x.d_ctx),
    d_pimpl(new impl(x.d_pimpl->d_pb, x.d_pimpl->d_result))
{
}
#endif


futres::~futres() 
{
    delete d_pimpl;
}


pubnub_res futres::last_result()
{
    return d_pimpl->last_result();
}

 
void futres::start_await()
{
    d_pimpl->start_await();
}

pubnub_res futres::end_await()
{
   return d_pimpl->end_await();
}


bool futres::valid() const
{
    return (d_pimpl != NULL) && (d_pimpl->d_pb != NULL);
}


bool futres::is_ready() const
{
    return d_pimpl->is_ready();
}


tribool futres::should_retry() const
{
    return pubnub_should_retry(d_pimpl->last_result());
}


void futres::then(std::function<void(context &, pubnub_res)> f)
{
    d_pimpl->then(f, this);
}

}
