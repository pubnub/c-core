/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub.hpp"
#include "pubnub_mutex.hpp"

#if PUBNUB_USE_EXTERN_C
extern "C" {
#endif
#include "core/pubnub_ntf_callback.h"
#include "core/pubnub_coreapi.h"
#if PUBNUB_USE_EXTERN_C
}
#endif

#include <windows.h>
#include <process.h>

#include <new>
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
static SRWLOCK m_registration_lock = SRWLOCK_INIT;

class registration_guard {
public:
    registration_guard() { AcquireSRWLockExclusive(&m_registration_lock); }
    ~registration_guard() { ReleaseSRWLockExclusive(&m_registration_lock); }

private:
    registration_guard(registration_guard const&);
    registration_guard& operator=(registration_guard const&);
};

#if (__cplusplus < 201103L) && (_MSC_VER < 1600)
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
        : d_wevent(CreateEvent(NULL, TRUE, FALSE, NULL))
        , d_pb(pb)
        , d_ctx(&ctx)
        , d_then_dispatched(false)
        , d_thread(0)
        , d_thread_id(0)
        , d_result(initial)
        , d_outcome(initial)
    {
        pubnub_mutex_init(d_mutex);
        if (NULL == d_wevent) {
            throw std::runtime_error("Failed to create a Windows event handle");
        }
        if (initial == PNR_STARTED) {
            {
                registration_guard lck;
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
        {
            registration_guard lck;
            if (pubnub_get_user_data(d_pb) == this) {
                (void)pubnub_register_callback(d_pb, NULL, NULL);
            }
        }
        pubnub_mutex_destroy(d_mutex);
        CloseHandle(d_wevent);
    }
    void       start_await() {}
    pubnub_res end_await()
    {
        pubnub_res res;
        {
            lock_guard lck(d_mutex);
            res = d_result;
        }
        if (PNR_STARTED != res) {
            return res;
        }
        WaitForSingleObject(d_wevent, INFINITE);
        bool got_outcome = false;
        {
            lock_guard lck(d_mutex);
            if (PNR_STARTED == d_result) {
                d_result    = d_outcome;
                got_outcome = true;
            }
            res = d_result;
        }
        if (got_outcome) {
            // Lets the C core do its bookkeeping for the outcome, as it
            // would for a C user getting it
            (void)pubnub_last_result(d_pb);
        }
        return res;
    }
    pubnub_res last_result()
    {
        bool over;
        {
            lock_guard lck(d_mutex);
            over = is_ready() || PNR_STARTED != d_result;
        }
        return over ? end_await() : pubnub_last_result(d_pb);
    }

    /// Records the outcome and starts the continuation, if it is set.
    /// Only the first call has any effect.
    void signal(pubnub_res rslt)
    {
        lock_guard lck(d_mutex);
        if (is_ready()) {
            return;
        }
        d_outcome = rslt;
        // Before the continuation starts, which may call `is_ready()`
        SetEvent(d_wevent);
        dispatch_then();
    }
    bool is_ready() const
    {
        return WAIT_OBJECT_0 == WaitForSingleObject(d_wevent, 0);
    }
    void then(then_function f)
    {
        lock_guard lck(d_mutex);
        if (d_then_dispatched) {
            return;
        }
        take(d_thenf, f);
        if (is_ready()) {
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
        d_thread     = (HANDLE)_beginthreadex(
            NULL, 0, then_thread, data, 0, &d_thread_id);
        if (NULL == d_thread) {
            delete data;
        }
    }
    void wait4_then_thread_to_start()
    {
        bool should_await;
        {
            lock_guard lck(d_mutex);
            should_await = !is_ready() && !!d_thenf;
        }
        if (should_await) {
            end_await();
        }
    }
    void wait4_then_thread_to_finish()
    {
        if (d_thread) {
            // Unless destroyed by its own continuation
            if (d_thread_id != GetCurrentThreadId()) {
                WaitForSingleObject(d_thread, INFINITE);
            }
            CloseHandle(d_thread);
            d_thread = 0;
        }
    }

    struct futres_callback_data {
        then_function f;
        context*      ctx;
        pubnub_res    result;
    };

    static unsigned __stdcall then_thread(void* parg)
    {
        futres_callback_data* d = static_cast<futres_callback_data*>(parg);
        d->f(*d->ctx, d->result);
        delete d;
        return 0;
    }

    /// Manual-reset event, set once the outcome is known
    HANDLE         d_wevent;
    pubnub_mutex_t d_mutex;
    /// The C Pubnub context that we are "wrapping"
    pubnub_t* d_pb;
    /// The C++ Pubnub context passed to the continuation
    context* d_ctx;

    then_function d_thenf;
    bool          d_then_dispatched;
    HANDLE        d_thread;
    unsigned      d_thread_id;
    /// PNR_STARTED until the outcome is awaited, if the transaction started
    pubnub_res d_result;
    /// The outcome, once `d_wevent` is set
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

#if (__cplusplus >= 201103L) || (_MSC_VER >= 1600)

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
