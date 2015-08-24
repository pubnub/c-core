/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_HPP
#define      INC_PUBNUB_HPP


//extern "C" {
#include "pubnub_api_types.h"
//}

namespace pubnub {
    class context;

    /** A future result of a Pubnub transaction/operation/request.
     * It is somewhat similar to the std::future<> from C++11.
     */
    class futres {
    public:
        /// The implementation class will be different for
        /// different platforms - C++11 being one platform
        class impl;

        futres(pubnub_t *pb, context &ctx, pubnub_res initial);
        ~futres();

        pubnub_res last_result();

        pubnub_res await();

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

        pubnub_t *d_pb;

        /// The pubnub context of this future result
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
