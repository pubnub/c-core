/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub.hpp"

//extern "C" {
#include "pubnub_ntf_callback.h"
//}

#include <pthread.h>

#include <stdexcept>


namespace pubnub {

class futres::impl {
public:
    impl() : d_triggered(false) {
        pthread_mutex_init(&d_mutex, NULL);
        pthread_cond_init(&d_cond, NULL);
    }
    void start_await() {
        pthread_mutex_lock(&d_mutex);
        d_triggered = false;
        pthread_mutex_unlock(&d_mutex);
    }
    void end_await() {
        pthread_mutex_lock(&d_mutex);
        while (!d_triggered) {
            pthread_cond_wait(&d_cond, &d_mutex);
        }
        pthread_mutex_unlock(&d_mutex);
    }
    void signal() {
        pthread_mutex_lock(&d_mutex);
        d_triggered = true;
        pthread_cond_signal(&d_cond);
        pthread_mutex_unlock(&d_mutex);
    }
    bool is_ready() const {
        bool rslt;
        pthread_mutex_lock(&d_mutex);
        rslt = d_triggered;
        pthread_mutex_unlock(&d_mutex);
        return rslt;
    }

private:
    mutable pthread_mutex_t d_mutex;
    bool d_triggered;
    pthread_cond_t d_cond;
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
