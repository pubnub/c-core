/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub.hpp"

#if PUBNUB_USE_EXTERN_C
extern "C" {
#endif
#include "core/pubnub_ntf_callback.h"
#if PUBNUB_USE_EXTERN_C
}
#endif

#include <pthread.h>

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


class futres::impl {
    friend class futres;

public:
    impl(pubnub_t* pb, pubnub_res initial)
        : d_triggered(false)
        , d_pb(pb)
        , d_have_thread_id(false)
        , d_result(initial)
    {
        pthread_mutex_init(&d_mutex, NULL);
        pthread_cond_init(&d_cond, NULL);
        if (initial != PNR_IN_PROGRESS) {
            pthread_lock_guard lck(&d_mutex);
            if (PNR_OK != pubnub_register_callback(d_pb, futres_callback, this)) {
                throw std::logic_error("Failed to register callback");
            }
        }
    }
    ~impl()
    {
        wait4_then_thread_to_start();
        wait4_then_thread_to_finish();
        (void)pubnub_register_callback(d_pb, NULL, NULL);
    }
    void       start_await() {}
    pubnub_res end_await()
    {
        pthread_lock_guard lck(&d_mutex);
        if (PNR_STARTED == d_result) {
            while (!d_triggered) {
                pthread_cond_wait(&d_cond, &d_mutex);
            }
            d_triggered     = false;
            return d_result = pubnub_last_result(d_pb);
        }
        else {
            return d_result;
        }
    }
    pubnub_res last_result()
    {
        pthread_lock_guard lck(&d_mutex);
        if (PNR_STARTED == d_result) {
            return d_result = pubnub_last_result(d_pb);
        }
        else {
            return d_result;
        }
    }

    void signal(pubnub_res rslt)
    {
        // SAFETY: it is safe to pass the `d_thread_id` without a lock because
        // any other usage of it requires the `d_have_thread_id` to be true
        // which is only set after the thread is created.

        futres_callback_data data = { this, rslt };
        pthread_create(&d_thread_id, NULL, signal_thread, &data);
    }
    bool is_ready() const
    {
        pthread_lock_guard lck(&d_mutex);
        return d_triggered;
    }
    void then(
#if __cplusplus < 201103L
        pubnub::caller_keeper f,
#else
        std::function<void(context&, pubnub_res)> f,
#endif
        futres* parent)
    {
        PUBNUB_ASSERT_OPT(parent != NULL);
        pthread_lock_guard lck(&d_mutex);
        d_thenf  = f;
        d_parent = parent;
    }

private:
    void wait4_then_thread_to_start()
    {
        bool should_await;
        {
            pthread_lock_guard lck(&d_mutex);
            should_await = !d_triggered && !!d_thenf && d_parent;
        }
        if (should_await) {
            end_await();
        }
    }
    void wait4_then_thread_to_finish()
    {
        if (d_have_thread_id) {
            void* retval;
            pthread_join(d_thread_id, &retval);
        }
    }

    struct futres_callback_data {
        futres::impl* p;
        pubnub_res    result;
    };

    static void* signal_thread(void* data)
    {
        futres_callback_data* d    = static_cast<futres_callback_data*>(data);
        futres::impl*         that = d->p;
        pubnub_res            rslt = d->result;
        bool                  should_call_then = false;

        {
            pthread_lock_guard lck(&that->d_mutex);
            should_call_then = !!that->d_thenf && that->d_parent;

            that->d_triggered = true;
            if (should_call_then && !that->d_have_thread_id) {
                that->d_result         = rslt;
                that->d_have_thread_id = true;
            }

            pthread_cond_signal(&that->d_cond);
        }

        if (should_call_then) {
            that->d_thenf(that->d_parent->d_ctx, rslt);
        }

        return NULL;
    }

    mutable pthread_mutex_t d_mutex;
    bool                    d_triggered;
    pthread_cond_t          d_cond;


#if __cplusplus < 201103L
    pubnub::caller_keeper d_thenf;
#else
    std::function<void(context&, pubnub_res)> d_thenf;
#endif
    /// The C Pubnub context that we are "wrapping"
    pubnub_t*  d_pb;
    futres*    d_parent;
    pthread_t  d_thread_id;
    bool       d_have_thread_id;
    pubnub_res d_result;
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
    , d_pimpl(new impl(pb, initial))
{
}


#if __cplusplus < 201103L
futres::futres(futres const& x)
    : d_ctx(x.d_ctx)
    , d_pimpl(new impl(x.d_pimpl->d_pb, x.d_pimpl->d_result))
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
    d_pimpl->then(f, this);
}

#else

void futres::thenx(caller_keeper kiper)
{
    d_pimpl->then(kiper, this);
}

#endif

} // namespace pubnub
