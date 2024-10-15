/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#if PUBNUB_USE_AUTO_HEARTBEAT

#include "lib/pb_strnlen_s.h"
#include "core/pubnub_assert.h"

#include <stdlib.h>
#include <string.h>

#define PUBNUB_MAX_OBJECT_LENGTH 30000

/** Removes all instances of @p member in comma separated list @p list
  */
static void remove_member(char* list, const char* member, size_t member_len)
{
    char* l_start;
    char* l_end;

    PUBNUB_ASSERT_OPT(list != NULL);

    l_end = list + pb_strnlen_s(list, PUBNUB_MAX_OBJECT_LENGTH);
    for (l_start = list; l_start < l_end;) {
        char* l_ch_end = (char*)memchr(l_start, ',', l_end - l_start);

        if (NULL == l_ch_end) {
            l_ch_end = l_end;
        }
        if (((size_t)(l_ch_end - l_start) == member_len) &&
            (memcmp(l_start, member, member_len) == 0)) {
            const size_t rest = l_end != l_ch_end ? l_end - (l_ch_end + 1) : 0;
            if (rest > 1) {
                /* Moves everything behind next comma including string end */
                memmove(l_start, l_ch_end + 1, rest);
                l_end -= l_ch_end + 1 - l_start;
            }
            else {
                /* Erases last comma if any */
                l_start[-(l_start > list)] = '\0';
            }
        }
        l_start = l_ch_end + 1;
    }
}


void pbstr_remove_from_list(char* list, const char* leave_list)
{
    const char* ll_start;
    const char* ll_end;
    
    PUBNUB_ASSERT_OPT(list != NULL);
    PUBNUB_ASSERT_OPT(leave_list != NULL);

    ll_end = leave_list + pb_strnlen_s(leave_list, PUBNUB_MAX_OBJECT_LENGTH);
    for (ll_start = leave_list; ll_start < ll_end;) {            
        const char* ch_end = (const char*)memchr(ll_start, ',', ll_end - ll_start);
        if (NULL == ch_end) {
            ch_end = ll_end;
        }
        remove_member(list, ll_start, ch_end - ll_start);
        if ('\0' == *list) {
            break;
        }
        ll_start = ch_end + 1;
    }
}


void pbstr_free_if_empty(char** list)
{
    char* l; 

    PUBNUB_ASSERT_OPT(list != NULL);
    l = *list;
    PUBNUB_ASSERT_OPT(l != NULL);
    
    if ('\0' == *l) {
        free(l);
        *list = NULL;
    }
}

#endif /* PUBNUB_USE_AUTO_HEARTBEAT */

