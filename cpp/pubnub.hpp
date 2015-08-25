/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_HPP
#define      INC_PUBNUB_HPP


/** @file pubnub.hpp
 *
 * The header that C++ Pubnub API/SDK client users use/include. This
 * is a wrapper of the Pubnub C client, not a "native" C++
 * implementation.
 */


//extern "C" {
#include "pubnub_api_types.h"
//}

namespace pubnub {
    class context;

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

        /// Awaits the end of transaction and returns its final
        /// result (outcome).
        pubnub_res await() {
            start_await();
            return end_await();
        }

        // C++11 std::future<> compatible API
        pubnub_res get() {
            return await();
        }
        bool valid() const;
        void wait() /*const*/ {
            await();
        }

        // C++17 (somewhat) compatbile API
        template<typename F>
        void then(F f) {
            f(d_ctx, await());
        }
        bool is_ready() const;
        
    private:
        // Pubnub future result is non-copyable
        //futres(futres const &x);

        /// The C Pubnub context that we are "wrapping"
        pubnub_t *d_pb;

        /// The C++ Pubnub context of this future result
        context &d_ctx;

        /// The current result
        pubnub_res d_result;

        /// The implementation of the synchronization
        /// between the Pubnub callback and this "future result"
        impl *d_pimpl;
    };
}

#include "pubnub_common.hpp"


#endif  // !defined INC_PUBNUB_HPP
