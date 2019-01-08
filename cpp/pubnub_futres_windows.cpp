/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub.hpp"
#include "pubnub_mutex.hpp"

#if PUBNUB_USE_EXTERN_C
extern "C" {
#endif
#include "core/pubnub_ntf_callback.h"
#include "core/pubnub_assert.h"
#if PUBNUB_USE_EXTERN_C
}
#endif

#include <windows.h>
#include <process.h>

#include <stdexcept>


namespace pubnub {

static void futres_callback(pubnub_t*         pb,
                            enum pubnub_trans trans,
                            enum pubnub_res   result,
                            void*             user_data);

class futres::impl {
    friend class futres;
public:
    impl(pubnub_t *pb, pubnub_res initial) :
        d_wevent(CreateEvent(NULL, TRUE, FALSE, NULL)),
        d_pb(pb),
        d_parent(nullptr),
        d_thread(0),
        d_result(initial)
    {
        pubnub_mutex_init(d_mutex);
        if (NULL == d_wevent) {
            throw std::runtime_error("Failed to create a Windows event handle");
        }
        if (initial != PNR_IN_PROGRESS) {
            lock_guard lck(d_mutex);
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
        pubnub_mutex_destroy(d_mutex);
        CloseHandle(d_wevent);
    }
    void start_await() { ResetEvent(d_wevent); }
    pubnub_res end_await() {
        pubnub_res res;
        {
            lock_guard lck(d_mutex);
            res = d_result;
        }
        if (PNR_STARTED == res) {
            WaitForSingleObject(d_wevent, INFINITE);
            lock_guard lck(d_mutex);
            return d_result = pubnub_last_result(d_pb);
        }
        else {
            return res;
        }
    }
    pubnub_res last_result() {
        lock_guard lck(d_mutex);
        if (PNR_STARTED == d_result) {
            return d_result = pubnub_last_result(d_pb);
        }
        else {
            return d_result;
        }
    }
    static unsigned __stdcall do_the_then(void* parg)
    {
        futres::impl* that = static_cast<futres::impl*>(parg);
        that->d_thenf(that->d_parent->d_ctx, that->d_result);
        return 0;
    }
    void signal(pubnub_res rslt)
    {
        lock_guard lck(d_mutex);
        if (!!d_thenf && d_parent) {
            d_result = rslt;
            d_thread = (HANDLE)_beginthreadex(NULL, 0, do_the_then, this, 0, NULL);
        }
        SetEvent(d_wevent);
    }
    bool is_ready() const
    {
        return WAIT_OBJECT_0 == WaitForSingleObject(d_wevent, 0);
    }
    void then(
#if (__cplusplus < 201103L) && (_MSC_VER < 1600)
        pubnub::caller_keeper f,
#else
        std::function<void(context&, pubnub_res)> f,
#endif
        futres* parent)
    {
        PUBNUB_ASSERT_OPT(parent != nullptr);
        lock_guard lck(d_mutex);
        d_thenf  = f;
        d_parent = parent;
    }

private:
    void wait4_then_thread_to_start()
    {
        bool should_await;
        {
            lock_guard lck(d_mutex);
            should_await = !is_ready() && !!d_thenf && d_parent;
        }
        if (should_await) {
            end_await();
        }
    }
    void wait4_then_thread_to_finish()
    {
        if (d_thread) {
            WaitForSingleObject(d_thread, INFINITE);
        }
    }

    HANDLE         d_wevent;
    pubnub_mutex_t d_mutex;
    /// The C Pubnub context that we are "wrapping"
    pubnub_t* d_pb;

#if (__cplusplus < 201103L) && (_MSC_VER < 1600)
    pubnub::caller_keeper d_thenf;
#else
    std::function<void(context&, pubnub_res)> d_thenf;
#endif
    futres*    d_parent;
    HANDLE     d_thread;
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

#if (__cplusplus >= 201103L) || (_MSC_VER >= 1600)

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
