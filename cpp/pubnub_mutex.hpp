/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_mutex.h"

namespace pubnub {

    class lock_guard {
        pubnub_mutex_t &d_m;
    public:
        lock_guard(pubnub_mutex_t &m) : d_m(m) { pubnub_mutex_lock(d_m); }
        ~lock_guard() { pubnub_mutex_unlock(d_m); }
    };

}
