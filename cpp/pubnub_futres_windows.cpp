/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub.hpp"
#include "pubnub_mutex.hpp"

//extern "C" {
#include "pubnub_ntf_callback.h"
#include "pubnub_assert.h"
//}

#include <windows.h>
#include <process.h>

#include <stdexcept>


namespace pubnub {

class futres::impl {
public:
    impl() : d_wevent(CreateEvent(NULL, TRUE, FALSE, NULL))
		, d_parent(nullptr)
        , d_thread(0) {
        pubnub_mutex_init(d_mutex);
    }
    ~impl() {
        wait4_then_thread_to_start();
        wait4_then_thread_to_finish();
    }
    void start_await() {
        ResetEvent(d_wevent);
    }
    void end_await() {
        WaitForSingleObject(d_wevent, INFINITE);
    }
    static unsigned __stdcall do_the_then(void *parg) {
        futres::impl *that = static_cast<futres::impl*>(parg);
        that->d_thenf(that->d_parent->d_ctx, that->d_result);
        return 0;
    }
    void signal(pubnub_res rslt) {
        lock_guard lck(d_mutex);
        if (!!d_thenf && d_parent) {
            d_result = rslt;
            d_thread = (HANDLE)_beginthreadex(NULL, 0, do_the_then, this, 0, NULL);
        }
		SetEvent(d_wevent);
    }
    bool is_ready() const {
        return WAIT_OBJECT_0 == WaitForSingleObject(d_wevent, 0);
    }
    void then(
#if (__cplusplus < 201103L) && (_MSC_VER < 1600)
        pubnub::caller_keeper f,
#else
        std::function<void(context&, pubnub_res)> f,
#endif
        futres *parent) {
		PUBNUB_ASSERT_OPT(parent != nullptr);
        lock_guard lck(d_mutex);
		d_thenf = f;
		d_parent = parent;
	}

private:
    void wait4_then_thread_to_start() {
        bool should_await;
        {
            lock_guard lck(d_mutex);
            should_await = !is_ready() && !!d_thenf && d_parent;
        }
        if (should_await) {
            end_await();
        }
    }
    void wait4_then_thread_to_finish() {
        if (d_thread) {
            WaitForSingleObject(d_thread, INFINITE);
        }
    }

    HANDLE d_wevent;
    pubnub_mutex_t d_mutex;

#if (__cplusplus < 201103L) && (_MSC_VER < 1600)
    pubnub::caller_keeper d_thenf;
#else
    std::function<void(context&, pubnub_res)> d_thenf;
#endif
	futres *d_parent;
    HANDLE d_thread;
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

#if (__cplusplus >= 201103L) || (_MSC_VER >= 1600)

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

