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
    pthread_lock_guard(pthread_mutex_t *mutex) : d_mutex(mutex) {
        pthread_mutex_lock(d_mutex);
    }
    ~pthread_lock_guard() { pthread_mutex_unlock(d_mutex); }
private:
    pthread_mutex_t *d_mutex;
};


class futres::impl {
public:
    impl() : d_triggered(false), d_have_thread_id(false) {
        pthread_mutex_init(&d_mutex, NULL);
        pthread_cond_init(&d_cond, NULL);
    }
    ~impl() {
        wait4_then_thread_to_start();
        wait4_then_thread_to_finish();
    }
    void start_await() {
        pthread_lock_guard lck(&d_mutex);
        d_triggered = false;
    }
    void end_await() {
        pthread_lock_guard lck(&d_mutex);
        while (!d_triggered) {
            pthread_cond_wait(&d_cond, &d_mutex);
        }
    }
    static void *do_the_then(void *parg) {
        futres::impl *that = static_cast<futres::impl*>(parg);
        that->d_thenf(that->d_parent->d_ctx, that->d_result);
        return 0;
    }
    void signal(pubnub_res rslt) {
        pthread_lock_guard lck(&d_mutex);
        d_triggered = true;
        if (!!d_thenf && d_parent) {
            d_result = rslt;
            d_have_thread_id = !pthread_create(&d_thread_id, NULL, do_the_then, this);
        }
        pthread_cond_signal(&d_cond);
    }
    bool is_ready() const {
        pthread_lock_guard lck(&d_mutex);
        return d_triggered;
    }
    void then(
#if __cplusplus < 201103L
        pubnub::caller_keeper f,
#else
        std::function<void(context&, pubnub_res)> f,
#endif
        futres *parent) {
        PUBNUB_ASSERT_OPT(parent != NULL);
        pthread_lock_guard lck(&d_mutex);
        d_thenf = f;
        d_parent = parent;
    }

private:
    void wait4_then_thread_to_start() {
        bool should_await;
        {
            pthread_lock_guard lck(&d_mutex);
            should_await = !d_triggered && !!d_thenf && d_parent;
        }
        if (should_await) {
            end_await();
        }
    }
    void wait4_then_thread_to_finish() {
        if (d_have_thread_id) {
            void *retval;
            pthread_join(d_thread_id, &retval);
        }
    }

    mutable pthread_mutex_t d_mutex;
    bool d_triggered;
    pthread_cond_t d_cond;


#if __cplusplus < 201103L
    pubnub::caller_keeper d_thenf;
#else
    std::function<void(context&, pubnub_res)> d_thenf;
#endif
    futres *d_parent;
    pthread_t d_thread_id;
    bool d_have_thread_id;
    pubnub_res d_result;
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

#if __cplusplus >= 201103L

void futres::then(std::function<void(pubnub::context &, pubnub_res)> f)
{
    d_pimpl->then(f, this);
}

#else

void futres::thenx(caller_keeper kiper)
{
    d_pimpl->then(kiper, this);
}

#endif

}
