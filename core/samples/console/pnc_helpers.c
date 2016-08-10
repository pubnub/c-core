/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#include "pnc_helpers.h"

#include "pubnub_assert.h"

#include <ctype.h>
#include <string.h>


int strcicmp(char const *a, char const *b)
{
    for (;; a++, b++) {
        int d = tolower(*a) - tolower(*b);
        if ((d != 0) || !*a) {
            return d;
        }
    }
}


bool equals_ic(char const *s1, char const *s2)
{
    return strcicmp(s1, s2) == 0;
}


char *chomp(char *s)
{
    size_t n;
    
    PUBNUB_ASSERT_OPT(s != NULL);

    n = strlen(s);
    while (n > 0) {
        switch (s[n-1]) {
        case '\r':
        case '\n':
            s[--n] = '\0';
            break;
        default:
            return s;
        }
    }
    return s;
}
