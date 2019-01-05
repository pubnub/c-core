/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_HPP
#define INC_PUBNUB_HPP


/** @file pubnub.hpp
 *
 * The header that C++ Pubnub API/SDK client users use/include. This
 * is a wrapper of the Pubnub C client, not a "native" C++
 * implementation.
 */

#include <functional>
#include <string>

#if PUBNUB_USE_EXTERN_C
extern "C" {
#endif
#include "core/pubnub_api_types.h"
#include "core/pubnub_assert.h"
#include "core/pubnub_helper.h"
#if PUBNUB_USE_EXTERN_C
}
#endif

namespace pubnub {
class context;

#if __cplusplus < 201103L
// See what we had to deal with? :)

template <class R, class T, class A, class B>
class mem_fun2_t : public std::binary_function<A, B, R> {
    R (T::*pmem)(A, B);
    T* d_t;

public:
    explicit mem_fun2_t(R (T::*p)(A, B), T* t)
        : pmem(p)
        , d_t(t)
    {
    }
    R operator()(A a, B b) const { return (d_t->*pmem)(a, b); }
};

template <class R, class T, class A, class B>
class const_mem_fun2_t : public std::binary_function<A, B, R> {
    R (T::*pmem)(A, B) const;
    T* d_t;

public:
    explicit const_mem_fun2_t(R (T::*p)(A, B) const, T* t)
        : pmem(p)
        , d_t(t)
    {
    }
    R operator()(A a, B b) const { return (d_t->*pmem)(a, b); }
};

template <class R, class T, class A, class B>
class mem_fun2_ref_t : public std::binary_function<A, B, R> {
    R (T::*pmem)(A, B);
    T& d_t;

public:
    explicit mem_fun2_ref_t(R (T::*p)(A, B), T& t)
        : pmem(p)
        , d_t(t)
    {
    }
    R operator()(A a, B b) const { return (d_t.*pmem)(a, b); }
};

template <class R, class T, class A, class B>
class const_mem_fun2_ref_t : public std::binary_function<A, B, R> {
    R (T::*pmem)(A, B) const;
    T& d_t;

public:
    explicit const_mem_fun2_ref_t(R (T::*p)(A, B) const, T& t)
        : pmem(p)
        , d_t(t)
    {
    }
    R operator()(A a, B b) const { return (d_t.*pmem)(a, b); }
};

template <class R, class T, class A, class B>
mem_fun2_t<R, T, A, B> mem_fun(R (T::*f)(A, B), T* t)
{
    return mem_fun2_t<R, T, A, B>(f, t);
}
template <class R, class T, class A, class B>
const_mem_fun2_t<R, T, A, B> mem_fun(R (T::*f)(A, B) const, T* t)
{
    return const_mem_fun2_t<R, T, A, B>(f, t);
}
template <class R, class T, class A, class B>
mem_fun2_ref_t<R, T, A, B> mem_fun_ref(R (T::*f)(A, B), T& t)
{
    return mem_fun2_ref_t<R, T, A, B>(f, t);
}
template <class R, class T, class A, class B>
const_mem_fun2_ref_t<R, T, A, B> mem_fun_ref(R (T::*f)(A, B) const, T& t)
{
    return const_mem_fun2_ref_t<R, T, A, B>(f, t);
}
struct caller {
    virtual void callme(context& ctx, pubnub_res result) = 0;
    virtual ~caller() {}
};
template <class T> class caller_adaptor : public caller {
public:
    caller_adaptor(T t)
        : d_t(t)
    {
    }
    void    callme(context& ctx, pubnub_res result) { d_t(ctx, result); }
    caller* clone() { return new caller_adaptor(d_t); }
    virtual ~caller_adaptor() {}

private:
    T d_t;
};
class caller_keeper {
    caller* d_pcaller;

public:
    caller_keeper()
        : d_pcaller(0)
    {
    }
    caller_keeper(caller* pc)
        : d_pcaller(pc)
    {
        PUBNUB_ASSERT_OPT(pc != 0);
    }
    caller_keeper(caller_keeper& x)
        : d_pcaller(x.d_pcaller)
    {
        x.d_pcaller = 0;
    }
    void operator=(caller_keeper& x)
    {
        d_pcaller   = x.d_pcaller;
        x.d_pcaller = 0;
    }
    ~caller_keeper() { delete d_pcaller; }
    void operator()(context& ctx, pubnub_res result)
    {
        d_pcaller->callme(ctx, result);
    }
    bool operator!() const { return 0 == d_pcaller; }
};
#endif

/** A simple C++ tribool type. It has `true`, `false` and
    `not_set`.
 */
class tribool {
public:
    enum not_set_t { not_set = 2 };
    tribool()
        : d_3log(not_set)
    {
    }
    tribool(enum pubnub_tribool v)
        : d_3log(v)
    {
    }
    tribool(bool v)
        : d_3log(v)
    {
    }
    tribool(not_set_t)
        : d_3log(not_set)
    {
    }

    tribool operator!() const
    {
        static const tribool rslt[3] = { true, false, not_set };
        return rslt[d_3log];
    }
    bool is_set() const { return d_3log != not_set; }

    tribool operator&&(bool t) const { return *this && tribool(t); }
    tribool operator&&(tribool t) const
    {
        static const tribool rslt[3][3] = { { false, false, false },
                                            { false, true, not_set },
                                            { false, not_set, not_set } };
        return rslt[d_3log][t.d_3log];
    }

    tribool operator||(bool t) const { return *this || tribool(t); }
    tribool operator||(tribool t) const
    {
        static const tribool rslt[3][3] = { { false, true, not_set },
                                            { true, true, true },
                                            { not_set, true, not_set } };
        return rslt[d_3log][t.d_3log];
    }

    tribool operator==(tribool t) const
    {
        static const tribool rslt[3][3] = { { true, false, not_set },
                                            { false, true, not_set },
                                            { not_set, not_set, not_set } };
        return rslt[d_3log][t.d_3log];
    }
    tribool operator==(bool t) const
    {
        return !is_set() ? not_set
                         : static_cast<tribool>(static_cast<bool>(d_3log) == t);
    }

    tribool operator!=(tribool t) const
    {
        static const tribool rslt[3][3] = { { false, true, not_set },
                                            { true, false, not_set },
                                            { not_set, not_set, not_set } };
        return rslt[d_3log][t.d_3log];
    }
    tribool operator!=(bool t) const
    {
        return !is_set() ? not_set
                         : static_cast<tribool>(static_cast<bool>(d_3log) != t);
    }

    operator bool() const { return true == static_cast<bool>(d_3log); }

    std::string to_string() const
    {
        static char const* rslt[3] = { "false", "true", "not-set" };
        return rslt[d_3log];
    }

private:
    int d_3log;
};

inline tribool operator==(tribool, tribool::not_set_t)
{
    return tribool::not_set;
}

inline tribool operator!=(tribool, tribool::not_set_t)
{
    return tribool::not_set;
}

/** A future (pending) result of a Pubnub
 * transaction/operation/request.  It is somewhat similar to the
 * std::future<> from C++11.
 *
 * It has the same interface for both Pubnub C client's "sync" and
 * "callback" interfaces, so the C++ user code is always the same,
 * you just select the "back-end" during the build.
 */
class futres {
public:
    /// The implementation class will be different for
    /// different platforms - C++11 being one platform
    class impl;

    futres(pubnub_t* pb, context& ctx, pubnub_res initial);
    ~futres();

    /// Gets the last (or latest) result of the transaction.
    pubnub_res last_result();

    /// Starts the await. Only useful for the callback interface
    void start_await();

    /// Ends the await of the transaction to end and returns the
    /// final result (outcome)
    pubnub_res end_await();

    /// Awaits the end of transaction and returns its final result
    /// (outcome).
    pubnub_res await()
    {
        start_await();
        return end_await();
    }

    // C++11 std::future<> compatible API

    /// Same as await()
    pubnub_res get() { return await(); }

    /// Return whether this object is valid
    bool valid() const;

    /// Just wait for the transaction to end, don't get the
    /// outcome
    void wait() /*const*/ { await(); }

        // C++17 (somewhat) compatbile API

        /// Pass a function, function object (or lambda in C++11)
        /// which accepts a Pubnub context and pubnub_res and it will
        /// be called when the transaction ends.
#if (__cplusplus >= 201103L) || (_MSC_VER >= 1600)
    void then(std::function<void(pubnub::context&, pubnub_res)> f);
#else
    template <class T> void then(T f)
    {
        caller_adaptor<T> x(f);
        caller*           p = x.clone();
        caller_keeper     k(p);
        thenx(k);
    }

private:
    void thenx(caller_keeper kiper);

public:
#endif

    /// Returns true if the transaction is over, else otherwise
    bool is_ready() const;

    /// Parses the last (or latest) result of the transaction 'publish'.
    pubnub_publish_res parse_last_publish_result();

    // We can construct from a temporary
#if __cplusplus >= 201103L
    futres(futres&& x) :
        d_ctx(x.d_ctx),
        d_pimpl(x.d_pimpl)
    {
        x.d_pimpl = nullptr;
    }
#else
    futres(futres const& x);
#endif

    /// Indicates whether we should retry the last transaction
    /// @see pubnub_should_retry()
    tribool should_retry() const;

private:
    // Pubnub future result is non-copyable
    futres(futres& x);

    /// The C++ Pubnub context of this future result
    context& d_ctx;

    /// The implementation of the synchronization
    /// between the Pubnub callback and this "future result"
    impl* d_pimpl;

#if __cplusplus >= 201103L
    std::function<void(context&, pubnub_res)> d_thenf;
#endif
};

/** A helper class for a subscribe loop. Supports both a
    "piecemal", message-by-message, interface and a
    "callback-like" interface.
 */
class subloop {
public:
    /** Creates a subscribe loop, ready to do the looping.  We
        assume that the @p ctx Pubnub context will be valid
        throughout the lifetime of this object.

        @param ctx The context to use for the subscribe loop.
        @param channel The channel to subscribe to in the loop
    */
    subloop(context& ctx, std::string channel);

    /** Rather uneventful */
    ~subloop();

    /** Fetches one message in a subscribe loop.

        @par Basic usage
        @snippet pubnub_subloop_sample.cpp Fetch - pull interface

        @param [out] o_msg String with the fetched message
        @retval PNR_OK Message received
        @retval other The error code
     */
    pubnub_res fetch(std::string& o_msg);

    /** Executes a "full loop", calling the @p f functional object
        on each message received. To stop the loop, return any
        integer != 0 from @p f.

        @par Basic usage
        @snippet pubnub_subloop_sample.cpp Loop - push interface
    */
#if __cplusplus >= 201103L
    void loop(std::function<int(std::string, context&, pubnub_res)> f);
#else
    template <class T> void loop(T f)
    {
        std::string msg;
        while (0 == f(msg, d_ctx, fetch(msg))) {
            continue;
        }
    }
#endif

private:
    /// The context on which to do the subscribe loop
    context& d_ctx;
    /// Channel on which to subscribe in the loop
    std::string d_channel;
    /// Are we currently delivering messages gotten in one
    /// subscribe
    bool d_delivering;
};
} // namespace pubnub

#include "pubnub_common.hpp"


#endif // !defined INC_PUBNUB_HPP
