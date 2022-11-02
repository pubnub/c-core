/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "core/pubnub_pubsubapi.h"
#include "core/pubnub_coreapi.h"
#include "core/pubnub_assert.h"
#include "pubnub_internal.h"
#include "core/pubnub_ntf_sync.h"
#include "core/pubnub_alloc.h"

#include "pubnub_fntest.h"

#include <string.h>
#include <stdarg.h>
#include <stdint.h>


static struct PNTestParameters m_test_param;


bool pnfntst_got_messages(pubnub_t* p, ...)
{
    char const* aMsgs[16];
    uint16_t    missing;
    size_t      count = 0;
    va_list     vl;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(p));

    va_start(vl, p);
    while (count < 16) {
        char const* msg = va_arg(vl, char*);
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
        size_t      i;
        char const* msg = pubnub_get(p);
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

    if (missing) {
        printf("got messages: missing bitmap: %X\n", missing);
    }
    return !missing;
}


static bool eqstr(char const* s, char const* s2)
{
    return ((NULL == s) && (NULL == s2)) || (strcmp(s, s2) == 0);
}


bool pnfntst_got_message_on_channel(pubnub_t* p, char const* message, char const* channel)
{
    char const* msg;
    char const* chan;
    PUBNUB_ASSERT(pb_valid_ctx_ptr(p));
    PUBNUB_ASSERT_OPT(NULL != message);
    PUBNUB_ASSERT_OPT(NULL != channel);

    msg  = pubnub_get(p);
    chan = pubnub_get_channel(p);
    if ((NULL == msg) || (NULL == chan)) {
        return false;
    }
    return (strcmp(msg, message) == 0) && (strcmp(chan, channel) == 0);
}


bool pnfntst_subscribe_and_check(pubnub_t*   p,
                                 char const* channel,
                                 char const* chgroup,
                                 unsigned    ms,
                                 ...)
{
    char const*      aMsgs[16];
    char const*      aChan[16];
    uint16_t         missing;
    size_t           count = 0;
    pnfntst_timer_t* tmr;
    va_list          vl;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(p));

    va_start(vl, ms);
    while (count < 16) {
        char const* msg = va_arg(vl, char*);
        if (NULL == msg) {
            break;
        }
        aMsgs[count] = msg;
        aChan[count] = va_arg(vl, char*);
        ++count;
    }
    va_end(vl);

    if ((0 == count) || (count > 16)) {
        printf("subscribe and check: number of messages out of range: %d\n", (int)count);
        return false;
    }

    missing = (0x01 << count) - 1;
    tmr     = pnfntst_alloc_timer();
    if (NULL == tmr) {
        puts("subscribe and check: timer alloc failed");
        return false;
    }
    pnfntst_start_timer(tmr, ms);
    while (pnfntst_timer_is_running(tmr) && missing) {
        enum pubnub_res pbres = pubnub_subscribe(p, channel, chgroup);
        if (PNR_STARTED == pbres) {
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
        }
        else if (pbres != PNR_OK) {
            puts("subscribe and check: subscribe failed");
            break;
        }
        for (;;) {
            size_t      i;
            char const* msg  = pubnub_get(p);
            char const* chan = pubnub_get_channel(p);
            if (NULL == msg) {
                break;
            }
            for (i = 0; i < count; ++i) {
                if ((missing & (0x01 << i)) && (strcmp(msg, aMsgs[i]) == 0)
                    && eqstr(chan, aChan[i])) {
                    missing &= ~(0x01 << i);
                    break;
                }
            }
        }
    }

    pnfntst_free_timer(tmr);

    if (missing) {
        printf("subscribe and check: missing bitmap: %X\n", missing);
    }

    return !missing;
}


void pnfntst_free(void* p)
{
    pubnub_t* pbp = p;
    if (pbp != NULL) {
        if (pubnub_free(pbp) != 0) {
            printf("Failed to free the Pubnub context\n");
        }
    }
}


pubnub_t* pnfntst_create_ctx(void)
{
    pubnub_t* pbp = pubnub_alloc();
    if (pbp != NULL) {
        pubnub_init(pbp, m_test_param.pubkey, m_test_param.keysub);
        pubnub_origin_set(pbp, m_test_param.origin);
        pubnub_set_user_id(pbp, "test_id");
    }

    return pbp;
}

/** See
    https://support.pubnub.com/support/solutions/articles/14000043769-what-are-valid-channel-names-
*/
#define MAX_PUBNUB_CHAN_NAME 92


char* pnfntst_make_name(char const* s)
{
    unsigned grn  = rand();
    char*    rslt = malloc(MAX_PUBNUB_CHAN_NAME);
    if (NULL == rslt) {
        return rslt;
    }
    snprintf(rslt, MAX_PUBNUB_CHAN_NAME, "%s_%X", s, grn);

    return rslt;
}


struct PNTestParameters const* pnfntst_params(void)
{
    return &m_test_param;
}


int pnfntst_set_params(struct PNTestParameters const* p)
{
    m_test_param = *p;
    return 0;
}
