/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "lib/base64/pbbase64.h"

#include "core/pubnub_assert.h"
#include "core/pubnub_log.h"

#include <string.h>


int pbbase64_encode(pubnub_bymebl_t                data,
                    char*                          s,
                    size_t*                        n,
                    struct pbbase64_options const* options)
{
    size_t         i;
    char*          out    = s;
    uint8_t const* in     = data.ptr;
    size_t const   length = data.size;

    PUBNUB_ASSERT_OPT(data.ptr != NULL);
    PUBNUB_ASSERT_OPT(s != NULL);
    PUBNUB_ASSERT_OPT(n != NULL);
    PUBNUB_ASSERT_OPT(options != NULL);
    PUBNUB_ASSERT_OPT(options->alphabet != NULL);

    if (*n < pbbase64_char_array_size_for_encoding(length)) {
        return -1;
    }

    for (i = 0; i < length; i += 3) {
        uint8_t b = (in[0] & 0x0FC) >> 2;
        *out++    = options->alphabet[b];
        b         = (in[0] & 0x3) << 4;
        if (i + 1 < length) {
            b |= (in[1] & 0x0F0) >> 4;
            *out++ = options->alphabet[b];
            b      = (in[1] & 0x0F) << 2;
            if (i + 2 < length) {
                b |= (in[2] & 0x0C0) >> 6;
                *out++ = options->alphabet[b];
                b      = in[2] & 0x3F;
                *out++ = options->alphabet[b];
            }
            else {
                *out++ = options->alphabet[b];
                if (options->separator) {
                    *out++ = options->separator;
                }
            }
        }
        else {
            *out++ = options->alphabet[b];
            if (options->separator) {
                *out++ = options->separator;
                *out++ = options->separator;
            }
        }
        in += 3;
    }
    *out = '\0';
    *n   = out - s;

    return 0;
}


size_t pbbase64_encoded_length(size_t length)
{
    return ((length + 2) / 3) * 4;
}


size_t pbbase64_char_array_size_for_encoding(size_t length)
{
    return pbbase64_encoded_length(length) + 1;
}


pubnub_bymebl_t pbbase64_encode_alloc(pubnub_bymebl_t                data,
                                      struct pbbase64_options const* options)
{
    pubnub_bymebl_t result;
    result.size = pbbase64_char_array_size_for_encoding(data.size);
    result.ptr  = (uint8_t*)malloc(result.size);
    if (NULL == result.ptr) {
        result.size = 0;
        return result;
    }
    if (0 != pbbase64_encode(data, (char*)result.ptr, &result.size, options)) {
        free(result.ptr);
        result.ptr = NULL;
        result.size = 0;
    }
    return result;
}


int pbbase64_encode_std(pubnub_bymebl_t data, char* s, size_t* n)
{
    struct pbbase64_options options = PBBASE64_RFC3548_OPTIONS;
    return pbbase64_encode(data, s, n, &options);
}


pubnub_bymebl_t pbbase64_encode_alloc_std(pubnub_bymebl_t data)
{
    struct pbbase64_options options = PBBASE64_RFC3548_OPTIONS;
    return pbbase64_encode_alloc(data, &options);
}


static uint8_t const decode_tab_C[256] = {
    /*  00  01  02  03  04  05  06  07  08  09  0A  0B  0C  0D  0E  0F */
    /*0*/ 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    /*1*/ 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    /*2*/ 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    /*3*/ 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
    /*4*/ 64, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14,
    /*5*/ 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
    /*6*/ 64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    /*7*/ 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
    /*8*/ 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    /*9*/ 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    /*A*/ 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    /*B*/ 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    /*C*/ 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    /*D*/ 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    /*E*/ 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    /*F*/ 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
};


size_t pbbase64_decoded_length(size_t n)
{
    return (n * 3 + 3) / 4;
}


int pbbase64_decode(char const*                    s,
                    size_t                         n,
                    pubnub_bymebl_t*               data,
                    struct pbbase64_options const* options)
{
    size_t      i;
    char const* alphabet;
    uint8_t     decode_tab[256];
    uint8_t*    out;

    PUBNUB_ASSERT_OPT(data != NULL);
    PUBNUB_ASSERT_OPT(data->ptr != NULL);
    PUBNUB_ASSERT_OPT(s != NULL);
    PUBNUB_ASSERT_OPT(options != NULL);
    alphabet = options->alphabet;
    PUBNUB_ASSERT_OPT(alphabet != NULL);
    PUBNUB_ASSERT(
        0 == strncmp(alphabet, COMMON_BASE64_ABC, sizeof COMMON_BASE64_ABC - 1));

    out = data->ptr;
    if (pbbase64_decoded_length(n) > data->size) {
        PUBNUB_LOG_ERROR("pbbase64_decode(): Buffer to decode too small, n = "
                         "%u, data->size = %u, decoded_length = %u\n",
                         (unsigned)n,
                         (unsigned)data->size,
                         (unsigned)pbbase64_decoded_length(n));
        return -1;
    }

    memcpy(decode_tab, decode_tab_C, sizeof decode_tab);
    decode_tab[(int)alphabet[62]] = 62;
    decode_tab[(int)alphabet[63]] = 63;

    for (i = 0; i < n; i += 4) {
        uint8_t word[4];
        word[0] = decode_tab[(int)*s++];
        if ((word[0] == 64) && !options->ignore_invalid_char) {
            return -12;
        }
        word[1] = decode_tab[(int)*s++];
        if ((word[1] == 64) && !options->ignore_invalid_char) {
            return -13;
        }
        *out++  = (word[0] << 2) | (word[1] >> 4);
        word[2] = decode_tab[(int)*s++];
        word[3] = decode_tab[(int)*s++];
        if (word[2] < 64) {
            *out++ = (word[1] << 4) | (word[2] >> 2);
            if (word[3] < 64) {
                *out++ = (word[2] << 6) | word[3];
            }
            else {
                if ((s[-1] != options->separator)
                    && !options->ignore_invalid_char) {
                    return -14;
                }
            }
        }
        else {
            if ((s[-2] != options->separator) && !options->ignore_invalid_char) {
                return -15;
            }
        }
    }

    data->size = out - data->ptr;

    return 0;
}


int pbbase64_decode_str(char const*                    s,
                        pubnub_bymebl_t*               data,
                        struct pbbase64_options const* options)
{
    return pbbase64_decode(s, strlen(s), data, options);
}


pubnub_bymebl_t pbbase64_decode_alloc(char const*                    s,
                                      size_t                         n,
                                      struct pbbase64_options const* options)
{
    pubnub_bymebl_t result;
    result.size = pbbase64_decoded_length(n) + 1; /* +1 "just in case" */
    result.ptr  = (uint8_t*)malloc(result.size);
    if (NULL == result.ptr) {
        result.size = 0;
        return result;
    }
    if (0 != pbbase64_decode(s, n, &result, options)) {
        free(result.ptr);
        result.size = 0;
        result.ptr = NULL;
    }
    return result;
}


pubnub_bymebl_t pbbase64_decode_alloc_str(char const*                    s,
                                          struct pbbase64_options const* options)
{
    return pbbase64_decode_alloc(s, strlen(s), options);
}


int pbbase64_decode_std(char const* s, size_t n, pubnub_bymebl_t* data)
{
    struct pbbase64_options options = PBBASE64_RFC3548_OPTIONS;
    return pbbase64_decode(s, n, data, &options);
}


int pbbase64_decode_std_str(char const* s, pubnub_bymebl_t* data)
{
    return pbbase64_decode_std(s, strlen(s), data);
}


pubnub_bymebl_t pbbase64_decode_alloc_std(char const* s, size_t n)
{
    struct pbbase64_options options = PBBASE64_RFC3548_OPTIONS;
    return pbbase64_decode_alloc(s, n, &options);
}


pubnub_bymebl_t pbbase64_decode_alloc_std_str(char const* s)
{
    return pbbase64_decode_alloc_std(s, strlen(s));
}
