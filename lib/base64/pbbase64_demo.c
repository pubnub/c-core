/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pbbase64.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


#define ENC_INPUT_MED "The quick brown fox jumps over lazy dog."
#define ENC_RESULT_MED                                                         \
    "VGhlIHF1aWNrIGJyb3duIGZveCBqdW1wcyBvdmVyIGxhenkgZG9nLg=="

#define ENC_INPUT_SHORT "Make it so!"
#define ENC_RESULT_SHORT "TWFrZSBpdCBzbyE="

static void do_encode(void)
{
    int                   result;
    pubnub_bymebl_t const to_encode = { (uint8_t*)ENC_INPUT_MED,
                                        sizeof ENC_INPUT_MED - 1 };
    char                  encoded[sizeof ENC_RESULT_MED];
    size_t                n = sizeof encoded / sizeof encoded[0];

    result = pbbase64_encode_std(to_encode, encoded, &n);
    printf("Input `%s`\n"
           "encoded to `%s`\n"
           "expected   `%s`.\n",
           to_encode.ptr,
           encoded,
           ENC_RESULT_MED);
    assert(0 == result);
    assert(n == (sizeof encoded / sizeof encoded[0]) - 1);
    assert(0 == strcmp(encoded, ENC_RESULT_MED));
}


static void do_encode_alloc(void)
{
    pubnub_bymebl_t       result;
    pubnub_bymebl_t const to_encode = { (uint8_t*)ENC_INPUT_SHORT,
                                        sizeof ENC_INPUT_SHORT - 1 };

    result = pbbase64_encode_alloc_std(to_encode);
    printf("Input `%s`\n"
           "encoded to `%s`\n"
           "expected   `%s`.\n",
           to_encode.ptr,
           result.ptr,
           ENC_RESULT_SHORT);
    assert(result.ptr != NULL);
    assert(result.size == sizeof ENC_RESULT_SHORT - 1);
    assert(0 == strcmp((char*)result.ptr, ENC_RESULT_SHORT));
    free(result.ptr);
}


static void do_decode(void)
{
    int  result;
    char decoded[sizeof ENC_INPUT_MED + 3 * ((sizeof ENC_INPUT_MED % 3) != 0)];
    pubnub_bymebl_t decoded_mebl = { (uint8_t*)&decoded,
                                     sizeof decoded / sizeof decoded[0] };

    result = pbbase64_decode_std(
        ENC_RESULT_MED, sizeof ENC_RESULT_MED - 1, &decoded_mebl);
    assert(decoded_mebl.size == sizeof ENC_INPUT_MED - 1);
    decoded[decoded_mebl.size] = '\0';
    printf("Input `%s`\n"
           "decoded to `%s`\n"
           "expected   `%s`.\n",
           ENC_RESULT_MED,
           decoded,
           ENC_INPUT_MED);
    assert(0 == result);
    assert(0 == strcmp(decoded, ENC_INPUT_MED));
}


int main()
{
    puts("==================");
    do_encode();
    puts("------------------");
    do_encode_alloc();

    puts("__________________");
    do_decode();
    puts("==================");
}
