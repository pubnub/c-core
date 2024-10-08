/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pbref_counter.h"

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "core/pubnub_mutex.h"


// ----------------------------------
//              Types
// ----------------------------------

/** Reference counter type definition. */
struct pbref_counter {
    /** Number of references to the tracked shared resource. */
    int count;
    /** Counter value access lock. */
    pubnub_mutex_t mutw;
};


// ----------------------------------
//             Functions
// ----------------------------------

pbref_counter_t* pbref_counter_alloc(void)
{
    pbref_counter_t* refc = malloc(sizeof(pbref_counter_t));
    if (NULL == refc) { return NULL; }

    pubnub_mutex_init(refc->mutw);
    refc->count = 1;

    return refc;
}

size_t pbref_counter_increment(pbref_counter_t* counter)
{
    pubnub_mutex_lock(counter->mutw);
    const size_t count = ++counter->count;
    pubnub_mutex_unlock(counter->mutw);

    return count;
}

size_t pbref_counter_decrement(pbref_counter_t* counter)
{
    size_t count = 0;
    pubnub_mutex_lock(counter->mutw);
    if (0 != counter->count) { count = --counter->count; }
    pubnub_mutex_unlock(counter->mutw);

    return count;
}

size_t pbref_counter_free(pbref_counter_t* counter)
{
    pubnub_mutex_lock(counter->mutw);
    size_t count = counter->count;
    if (count > 0 && 0 == (count = --counter->count)) {
        pubnub_mutex_unlock(counter->mutw);
        pubnub_mutex_destroy(counter->mutw);
        free(counter);
    }
    else { pubnub_mutex_unlock(counter->mutw); }
    return count;
}