/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_fntest.h"

#include "pubnub_coreapi.h"
#include "pubnub_assert.h"
#include "pubnub_internal.h"

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
    while (missing) {
        size_t i;
        char const *msg = pubnub_get(p);
        if (NULL == msg) {
            break;
        }
        for (i = 0; i < count; ++i) {
            if (strcmp(msg, aMsgs[i]) == 0) {
                missing &= ~(0x01 << i);
                break;
            }
        }
    }

    return !missing;
}


bool pnfntst_got_message_on_channel(pubnub_t *p, char const *message, char const *channel)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(p));
    PUBNUB_ASSERT_OPT(NULL != message);
    PUBNUB_ASSERT_OPT(NULL != channel);

    return (strcmp(pubnub_get(p), message) == 0) && (strcmp(pubnub_get_channel(p), channel) == 0);
}
