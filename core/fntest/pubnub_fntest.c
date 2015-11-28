/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_coreapi.h"
#include "pubnub_assert.h"
#include "pubnub_internal.h"

#include "pubnub_fntest.h"

#include <string.h>
#include <stdarg.h>
#include <stdint.h>


bool pnfntst_got_messages(pubnub_t *p, ...)
{
    char const *aMsgs[16];
    uint16_t missing;
    size_t count = 0;
    va_list vl;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(p));

    va_start(vl, p);
    while (count < 16) {
        char const *msg = va_arg(vl, char*);
        if (NULL == msg) {
            break;
        }
        aMsgs[count++] = msg;
    }
    va_end(vl);

    if ((0 == count) || (count > 16)) {
        return false;
    }

    missing = (0x01 << count) - 1;
    for (;;) {
        size_t i;
        char const *msg = pubnub_get(p);
        if (NULL == msg) {
            break;
        }
        for (i = 0; i < count; ++i) {
            if ((missing & (0x01 << i)) && (strcmp(msg, aMsgs[i]) == 0)) {
                missing &= ~(0x01 << i);
                break;
            }
        }
    }
    return !missing;
}


bool pnfntst_got_message_on_channel(pubnub_t *p, char const *message, char const *channel)
{
    char const *msg;
    char const *chan;
    PUBNUB_ASSERT(pb_valid_ctx_ptr(p));
    PUBNUB_ASSERT_OPT(NULL != message);
    PUBNUB_ASSERT_OPT(NULL != channel);
    
    msg = pubnub_get(p);
    chan = pubnub_get_channel(p);
    if ((NULL == msg) || (NULL == chan)) {
        return false;
    }
    return (strcmp(msg, message) == 0) && (strcmp(chan, channel) == 0);
}


bool pnfntst_subscribe_and_check(pubnub_t *p, char const *channel, char const*chgroup, unsigned ms, ...)
{
    char const *aMsgs[16];
    char const *aChan[16];
    uint16_t missing;
    size_t count = 0;
    pnfntst_timer_t *tmr;
    va_list vl;
    
    PUBNUB_ASSERT(pb_valid_ctx_ptr(p));

    va_start(vl, ms);
    while (count < 16) {
        char const *msg = va_arg(vl, char*);
        if (NULL == msg) {
            break;
        }
        aMsgs[count] = msg;
        msg = va_arg(vl, char*);
        if (NULL == msg) {
            return false;
        }
        aChan[count] = msg;
        ++count;
    }
    va_end(vl);

    if ((0 == count) || (count > 16)) {
        return false;
    }
    
    missing = (0x01 << count) - 1;
    tmr = pnfntst_alloc_timer();
    if (NULL == tmr) {
        puts("subscribe and check: timer alloc failed");
        return false;
    }
    pnfntst_start_timer(tmr, ms);
    while (pnfntst_timer_is_running(tmr) && missing) {
        enum pubnub_res pbres = pubnub_subscribe(p, channel, chgroup);
        if (PNR_STARTED != pbres) {
            puts("subscribe and check: subscribe failed");
            break;
        }
        while (pnfntst_timer_is_running(tmr)) {
            pbres = pubnub_last_result(p);
            if (pbres != PNR_STARTED) {
                break;
            }
        }
        if (pbres != PNR_OK) {
            printf("subscribe and check: subscribe error %d\n", pbres);
            break;
        }
	
        for (;;) {
            size_t i;
            char const *msg = pubnub_get(p);
            char const *chan = pubnub_get_channel(p);
            if ((NULL == msg) || (NULL == chan)) {
                break;
            }
            for (i = 0; i < count; ++i) {
                if ((missing & (0x01 << i)) && (strcmp(msg, aMsgs[i]) == 0)
                    && (strcmp(chan, aChan[i]) == 0)
                    ) {
                    missing &= ~(0x01 << i);
                    break;
                }
            }
        }
    }
    
    pnfntst_free_timer(tmr);
    
    return !missing;
}
