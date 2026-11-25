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

#include <pthread.h>

#include <new>
#include <stdexcept>


namespace pubnub {

class pthread_lock_guard {
public:
    pthread_lock_guard(pthread_mutex_t* mutex)
        : d_mutex(mutex)
    {
        pthread_mutex_lock(d_mutex);
    }
    ~pthread_lock_guard() { pthread_mutex_unlock(d_mutex); }

private:
    pthread_mutex_t* d_mutex;
};


static void futres_callback(pubnub_t*         pb,
                            enum pubnub_trans trans,
                            enum pubnub_res   result,
                            void*             user_data);

/// Serializes (un)registering the outcome callbacks, so that a future
/// result doesn't unregister the callback of a newer one on the same
/// context. Lock order: this lock, then the context monitor. `d_mutex` of
/// a future result must not be held while calling the C API, because the
/// C core calls the outcome callback, which takes it, under the monitor.
static pthread_mutex_t m_registration_mutex = PTHREAD_MUTEX_INITIALIZER;

#if __cplusplus < 201103L
typedef pubnub::caller_keeper then_function;
/// Moves @p from to @p to (assigning a `caller_keeper` moves)
static void take(then_function& to, then_function& from) { to = from; }
#else
typedef std::function<void(context&, pubnub_res)> then_function;
/// Moves @p from to the empty @p to
static void take(then_function& to, then_function& from) { to.swap(from); }
#endif


class futres::impl {
    friend class futres;

public:
    impl(pubnub_t* pb, context& ctx, pubnub_res initial)
        : d_triggered(false)
        , d_pb(pb)
        , d_ctx(&ctx)
        , d_then_dispatched(false)
        , d_have_thread_id(false)
        , d_result(initial)
        , d_outcome(initial)
    {
        pthread_mutex_init(&d_mutex, NULL);
        pthread_cond_init(&d_cond, NULL);
        if (initial == PNR_STARTED) {
            {
                pthread_lock_guard lck(&m_registration_mutex);
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
        pthread_lock_guard lck(&m_registration_mutex);
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
            pthread_lock_guard lck(&d_mutex);
            while (!d_triggered && PNR_STARTED == d_result) {
                pthread_cond_wait(&d_cond, &d_mutex);
            }
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
            pthread_lock_guard lck(&d_mutex);
            over = d_triggered || PNR_STARTED != d_result;
        }
        return over ? end_await() : pubnub_last_result(d_pb);
    }

    /// Records the outcome and starts the continuation, if it is set.
    /// Only the first call has any effect.
    void signal(pubnub_res rslt)
    {
        pthread_lock_guard lck(&d_mutex);
        if (d_triggered) {
            return;
        }
        d_outcome   = rslt;
        d_triggered = true;
        pthread_cond_broadcast(&d_cond);
        dispatch_then();
    }
    bool is_ready() const
    {
        pthread_lock_guard lck(&d_mutex);
        return d_triggered;
    }
    void then(then_function f)
    {
        pthread_lock_guard lck(&d_mutex);
        if (d_then_dispatched) {
            return;
        }
        take(d_thenf, f);
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
        d_then_dispatched          = true;
        futres_callback_data* data = new (std::nothrow) futres_callback_data;
        if (NULL == data) {
            return;
        }
        take(data->f, d_thenf);
        data->ctx    = d_ctx;
        data->result = d_outcome;
        if (0 == pthread_create(&d_thread_id, NULL, then_thread, data)) {
            d_have_thread_id = true;
        }
        else {
            delete data;
        }
    }
    void wait4_then_thread_to_start()
    {
        bool should_await;
        {
            pthread_lock_guard lck(&d_mutex);
            should_await = !d_triggered && !!d_thenf;
        }
        if (should_await) {
            end_await();
        }
    }
    void wait4_then_thread_to_finish()
    {
        if (d_have_thread_id) {
            if (pthread_equal(d_thread_id, pthread_self())) {
                // Destroyed by its own continuation
                pthread_detach(d_thread_id);
            }
            else {
                void* retval;
                pthread_join(d_thread_id, &retval);
            }
        }
    }

    struct futres_callback_data {
        then_function f;
        context*      ctx;
        pubnub_res    result;
    };

    static void* then_thread(void* data)
    {
        futres_callback_data* d = static_cast<futres_callback_data*>(data);
        d->f(*d->ctx, d->result);
        delete d;
        return NULL;
    }

    mutable pthread_mutex_t d_mutex;
    bool                    d_triggered;
    pthread_cond_t          d_cond;

    then_function d_thenf;
    /// The C Pubnub context that we are "wrapping"
    pubnub_t* d_pb;
    /// The C++ Pubnub context passed to the continuation
    context*   d_ctx;
    bool       d_then_dispatched;
    pthread_t  d_thread_id;
    bool       d_have_thread_id;
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

#if __cplusplus >= 201103L

void futres::then(std::function<void(pubnub::context&, pubnub_res)> f)
{
    d_pimpl->then(f);
}

#else

void futres::thenx(caller_keeper kiper)
{
    d_pimpl->then(kiper);
}

#endif

} // namespace pubnub
