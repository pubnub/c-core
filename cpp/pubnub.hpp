/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_HPP
#define      INC_PUBNUB_HPP


/** @file pubnub.hpp
 *
 * The header that C++ Pubnub API/SDK client users use/include. This
 * is a wrapper of the Pubnub C client, not a "native" C++
 * implementation.
 */

#include <functional>

//extern "C" {
#include "pubnub_api_types.h"
#include "pubnub_assert.h"
//}

namespace pubnub {
    class context;

#if __cplusplus < 201103L
    // See what we had to deal with? :)

    template <class R, class T, class A, class B>
    class mem_fun2_t : public std::binary_function <A,B,R> {
        R (T::*pmem)(A, B);
        T *d_t;
    public:
        explicit mem_fun2_t ( R (T::*p)(A, B), T* t) : pmem(p), d_t(t) {}
        R operator() (A a, B b) const { return (d_t->*pmem)(a, b); }
    };

    template <class R, class T, class A, class B>
    class const_mem_fun2_t : public std::binary_function <A,B,R> {
        R (T::*pmem)(A, B) const;
        T *d_t;
    public:
        explicit const_mem_fun2_t ( R (T::*p)(A, B) const, T* t) : pmem(p), d_t(t) {}
        R operator() (A a, B b) const { return (d_t->*pmem)(a, b); }
    };

    template <class R, class T, class A, class B>
    class mem_fun2_ref_t : public std::binary_function <A,B,R> {
        R (T::*pmem)(A, B);
        T &d_t;
    public:
        explicit mem_fun2_ref_t ( R (T::*p)(A, B), T& t) : pmem(p), d_t(t) {}
        R operator() (A a, B b) const { return (d_t.*pmem)(a, b); }
    };

    template <class R, class T, class A, class B>
    class const_mem_fun2_ref_t : public std::binary_function <A,B,R> {
        R (T::*pmem)(A, B) const;
        T &d_t;
    public:
        explicit const_mem_fun2_ref_t(R (T::*p)(A, B) const, T& t) : pmem(p), d_t(t) {}
        R operator() (A a, B b) const { return (d_t.*pmem)(a, b); }
    };

    template <class R, class T, class A, class B>
    mem_fun2_t<R,T,A,B> mem_fun(R (T::*f)(A,B), T* t) {
        return mem_fun2_t<R,T,A,B>(f, t);
    }
    template <class R, class T, class A, class B>
    const_mem_fun2_t<R,T,A,B> mem_fun(R (T::*f)(A,B) const, T* t) {
        return const_mem_fun2_t<R,T,A,B>(f, t);
    }
    template <class R, class T, class A, class B>
    mem_fun2_ref_t<R,T,A,B> mem_fun_ref(R (T::*f)(A,B), T& t) {
        return mem_fun2_ref_t<R,T,A,B>(f, t);
    }
    template <class R, class T, class A, class B>
    const_mem_fun2_ref_t<R,T,A,B> mem_fun_ref(R (T::*f)(A,B) const, T& t) {
        return const_mem_fun2_ref_t<R,T,A,B>(f, t);
    }
    struct caller {
        virtual void callme(context &ctx, pubnub_res result) = 0;
        virtual ~caller() {}
    };
    template<class T> class caller_adaptor : public caller {
    public:
        caller_adaptor(T t) : d_t(t) {}
        void callme(context &ctx, pubnub_res result) { d_t(ctx, result);}
        caller *clone() { return new caller_adaptor(d_t); }
        virtual ~caller_adaptor() {}
    private:
        T d_t;
    };
    class caller_keeper {
        caller *d_pcaller;
    public:
        caller_keeper() : d_pcaller(0) {}
        caller_keeper(caller *pc) : d_pcaller(pc) {
            PUBNUB_ASSERT_OPT(pc != 0);
        }
        caller_keeper(caller_keeper& x) : d_pcaller(x.d_pcaller) { 
            x.d_pcaller = 0; 
        }
        void operator=(caller_keeper& x) {
            d_pcaller = x.d_pcaller;
            x.d_pcaller = 0; 
        }
        ~caller_keeper() { delete d_pcaller; }
        void operator()(context &ctx, pubnub_res result) { 
            d_pcaller->callme(ctx, result);
        }
        bool operator!() const { return 0 == d_pcaller; }
    };
#endif

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

        futres(pubnub_t *pb, context &ctx, pubnub_res initial);
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
        pubnub_res await() {
            start_await();
            return end_await();
        }

        // C++11 std::future<> compatible API

        /// Same as await()
        pubnub_res get() {
            return await();
        }

        /// Return whether this object is valid
        bool valid() const;

        /// Just wait for the transaction to end, don't get the
        /// outcome
        void wait() /*const*/ {
            await();
        }

        // C++17 (somewhat) compatbile API

        /// Pass a function, function object (or lambda in C++11)
        /// which accepts a Pubnub context and pubnub_res and it will
        /// be called when the transaction ends.
#if (__cplusplus >= 201103L) || (_MSC_VER >= 1600)
        void then(std::function<void(pubnub::context &, pubnub_res)> f);
#else
        template<class T>
        void then(T f) {
            caller_adaptor<T> x(f);
            caller *p = x.clone();
            caller_keeper k(p);
            thenx(k);
        }
    private:
        void thenx(caller_keeper kiper);
    public:
#endif

        /// Returns true if the transaction is over, else otherwise
        bool is_ready() const;

        // We can construct from a temporary
#if __cplusplus >= 201103L
        futres(futres &&x) :
            d_pb(x.d_pb), d_ctx(x.d_ctx), d_result(x.d_result), d_pimpl(x.d_pimpl) {
            x.d_pb = nullptr;
            x.d_pimpl = nullptr;
        }
#else
        futres(futres const &x);
#endif

    private:
        // Pubnub future result is non-copyable
        futres(futres &x);

        /// The C Pubnub context that we are "wrapping"
        pubnub_t *d_pb;

        /// The C++ Pubnub context of this future result
        context &d_ctx;

        /// The current result
        pubnub_res d_result;

        /// The implementation of the synchronization
        /// between the Pubnub callback and this "future result"
        impl *d_pimpl;

#if __cplusplus >= 201103L
        std::function<void(context&, pubnub_res)> d_thenf;
#endif
	};
}

#include "pubnub_common.hpp"


#endif  // !defined INC_PUBNUB_HPP
