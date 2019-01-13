/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"

#include "pubnub_assert.h"
#include "pubnub_log.h"

#include <string.h>

int pubnub_url_encode(char* buffer, char const* what, size_t buffer_size)
{
    int i = 0;

    PUBNUB_ASSERT_OPT(buffer != NULL);
    PUBNUB_ASSERT_OPT(what != NULL);

    *buffer='\0';
    while (what[0]) {
        /* RFC 3986 Unreserved characters plus few
         * safe reserved ones. */
        size_t okspan = strspn(
            what,
            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.~"
            ",=:;@[]");
        if (okspan > 0) {
            if (okspan >= (unsigned)(buffer_size - i - 1)) {
                PUBNUB_LOG_ERROR("Error:|Url-encoded string is longer than permited.\n"
                                 "      |buffer_size = %u\n"
                                 "      |url-encoded_stretch = \"%s\"\n"
                                 "      |url-encoded-stretch_length = %u\n"
                                 "      |rest_of_the_string_to_be_encoded = \"%s\"\n"
                                 "      |length_of_the_rest_of_the_string_to_be_encoded = %u\n",
                                 (unsigned)buffer_size,
                                 buffer,
                                 (unsigned)strlen(buffer),
                                 what,
                                 (unsigned)strlen(what));
                return -1;
            }
            memcpy(buffer + i, what, okspan);
            i += okspan;
            buffer[i] = '\0';
            what += okspan;
        }
        if (what[0]) {
            /* %-encode a non-ok character. */
            char enc[4] = { '%' };
            enc[1]      = "0123456789ABCDEF"[(unsigned char)what[0] / 16];
            enc[2]      = "0123456789ABCDEF"[(unsigned char)what[0] % 16];
            if (3 > buffer_size - i - 1) {
                PUBNUB_LOG_ERROR("Error:|Url-encoded string is longer than permited.\n"
                                 "      |buffer_size = %u\n"
                                 "      |url-encoded_stretch = \"%s\"\n"
                                 "      |url-encoded-stretch_length = %u\n"
                                 "      |rest_of_the_string_to_be_encoded = \"%s\"\n"
                                 "      |length_of_the_rest_of_the_string_to_be_encoded = %u\n",
                                 (unsigned)buffer_size,
                                 buffer,
                                 (unsigned)strlen(buffer),
                                 what,
                                 (unsigned)strlen(what));
                return -1;
            }
            /* Last copied character is '\0' */
            memcpy(buffer + i, enc, 4);
            i += 3;
            ++what;
        }
    }

    return i;
}
