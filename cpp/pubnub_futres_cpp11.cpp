/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub.hpp"

#if PUBNUB_USE_EXTERN_C
extern "C" {
#endif
#include "core/pubnub_ntf_callback.h"
#include "core/pubnub_coreapi.h"
#if PUBNUB_USE_EXTERN_C
}
#endif

#include <thread>
#include <condition_variable>

#include <stdexcept>


namespace pubnub {

static void futres_callback(pubnub_t*         pb,
                            enum pubnub_trans trans,
                            enum pubnub_res   result,
                            void*             user_data);

/// Serializes (un)registering the outcome callbacks, so that a future
/// result doesn't unregister the callback of a newer one on the same
/// context. Lock order: this lock, then the context monitor. `d_mutex` of
/// a future result must not be held while calling the C API, because the
/// C core calls the outcome callback, which takes it, under the monitor.
static std::mutex& registration_mutex()
{
    static std::mutex m;
    return m;
}

class futres::impl {
    friend class futres;

public:
    impl(pubnub_t* pb, context& ctx, pubnub_res initial)
        : d_triggered(false)
        , d_pb(pb)
        , d_ctx(&ctx)
        , d_then_dispatched(false)
        , d_result(initial)
        , d_outcome(initial)
    {
        if (initial == PNR_STARTED) {
            {
                std::lock_guard<std::mutex> lk(registration_mutex());
                if (PNR_OK != pubnub_register_callback(d_pb, futres_callback, this)) {
                    throw std::logic_error("Failed to register callback");
                }
            }
            // The transaction may have ended before the callback was
            // registered. The C core makes the context idle and calls the
            // callback in one critical section, so an idle context means
            // the outcome is final (and `signal()` ignores a second one).
            if (pubnub_can_start_transaction(d_pb)) {
                signal(pubnub_last_result(d_pb));
            }
        }
    }
    ~impl()
    {
        wait4_then_thread_to_start();
        wait4_then_thread_to_finish();
        std::lock_guard<std::mutex> lk(registration_mutex());
        if (pubnub_get_user_data(d_pb) == this) {
            (void)pubnub_register_callback(d_pb, NULL, NULL);
        }
    }
    void       start_await() {}
    pubnub_res end_await()
    {
        pubnub_res rslt;
        bool       got_outcome = false;
        {
            std::unique_lock<std::mutex> lk(d_mutex);
            d_cond.wait(lk, [&] { return d_triggered || PNR_STARTED != d_result; });
            if (PNR_STARTED == d_result) {
                d_result    = d_outcome;
                got_outcome = true;
            }
            rslt = d_result;
        }
        if (got_outcome) {
            // Lets the C core do its bookkeeping for the outcome, as it
            // would for a C user getting it
            (void)pubnub_last_result(d_pb);
        }
        return rslt;
    }
    pubnub_res last_result()
    {
        bool over;
        {
            std::lock_guard<std::mutex> lk(d_mutex);
            over = d_triggered || PNR_STARTED != d_result;
        }
        return over ? end_await() : pubnub_last_result(d_pb);
    }
    /// Records the outcome and starts the continuation, if it is set.
    /// Only the first call has any effect.
    void signal(pubnub_res rslt)
    {
        std::lock_guard<std::mutex> lk(d_mutex);
        if (d_triggered) {
            return;
        }
        d_outcome   = rslt;
        d_triggered = true;
        d_cond.notify_all();
        dispatch_then();
    }
    bool is_ready() const
    {
        std::lock_guard<std::mutex> lk(d_mutex);
        return d_triggered;
    }
    void then(std::function<void(context&, pubnub_res)> f)
    {
        std::lock_guard<std::mutex> lk(d_mutex);
        if (d_then_dispatched) {
            return;
        }
        d_thenf = std::move(f);
        if (d_triggered) {
            dispatch_then();
        }
    }

private:
    /// Starts the continuation on its own thread, at most once. Called
    /// with `d_mutex` held, maybe from the outcome callback in the C core.
    void dispatch_then()
    {
        if (!d_thenf) {
            return;
        }
        d_then_dispatched = true;
        try {
            d_thread = std::thread(run_then, std::move(d_thenf), d_ctx, d_outcome);
        }
        catch (...) {
            // Can't run the continuation, but mustn't throw into the C core
        }
        d_thenf = nullptr;
    }
    static void run_then(std::function<void(context&, pubnub_res)> f,
                         context*                                  ctx,
                         pubnub_res                                rslt)
    {
        f(*ctx, rslt);
    }
    void wait4_then_thread_to_start()
    {
        auto should_await = [=] {
            std::lock_guard<std::mutex> lk(d_mutex);
            return !d_triggered && d_thenf;
        }();
        if (should_await) {
            end_await();
        }
    }
    void wait4_then_thread_to_finish()
    {
        if (d_thread.joinable()) {
            if (d_thread.get_id() == std::this_thread::get_id()) {
                // Destroyed by its own continuation
                d_thread.detach();
            }
            else {
                d_thread.join();
            }
        }
    }

    mutable std::mutex      d_mutex;
    bool                    d_triggered;
    std::condition_variable d_cond;
    /// The C Pubnub context that we are "wrapping"
    pubnub_t* d_pb;
    /// The C++ Pubnub context passed to the continuation
    context* d_ctx;

    std::function<void(context&, pubnub_res)> d_thenf;
    bool                                      d_then_dispatched;
    std::thread                               d_thread;
    /// PNR_STARTED until the outcome is awaited, if the transaction started
    pubnub_res d_result;
    /// The outcome, once `d_triggered`
    pubnub_res d_outcome;
};


static void futres_callback(pubnub_t*         pb,
                            enum pubnub_trans trans,
                            enum pubnub_res   result,
                            void*             user_data)
{
    futres::impl* p = static_cast<futres::impl*>(user_data);
    p->signal(result);
}


futres::futres(pubnub_t* pb, context& ctx, pubnub_res initial)
    : d_ctx(ctx)
    , d_pimpl(new impl(pb, ctx, initial))
{
}


#if __cplusplus < 201103L
futres::futres(futres const& x)
    : d_ctx(x.d_ctx)
    , d_pimpl(new impl(x.d_pimpl->d_pb, x.d_ctx, x.d_pimpl->d_result))
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


void futres::then(std::function<void(context&, pubnub_res)> f)
{
    d_pimpl->then(std::move(f));
}

} // namespace pubnub
