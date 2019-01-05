/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_MUTEX_HPP
#define INC_PUBNUB_MUTEX_HPP

#if PUBNUB_USE_EXTERN_C
extern "C" {
#endif
#include "core/pubnub_mutex.h"
#if PUBNUB_USE_EXTERN_C
}
#endif

namespace pubnub {

    class lock_guard {
        pubnub_mutex_t &d_m;
    public:
        lock_guard(pubnub_mutex_t &m) : d_m(m) { pubnub_mutex_lock(d_m); }
        ~lock_guard() { pubnub_mutex_unlock(d_m); }
    };

}

#endif // !defined INC_PUBNUB_MUTEX_HPP
