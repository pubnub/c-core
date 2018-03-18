/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PBBASE64
#define INC_PBBASE64

#include "core/pubnub_memory_block.h"

#include <stdbool.h>


/** @file pbbase64.h

    Base64 encoder/decoder.

    Why does the world need yet another Base64 encoder/decoder?
    Because most are just not flexible enough. For such a simple
    purpose, one would think that there would be an open-source widely
    available flexible library, but, there isn't. Or, well, now there
    is.

    We provide a generic encoder and decoder, which support all known
    uses of Base64.

    Decoded data is arbitrary - it doesn't have to be regular
    characters.

    Also, we are flexible with regards to input - it may be a whole
    (ASCIIZ) string, or a portion of string.

    For output allocation - user may allocate or let the
    encoder/decoder do it for her.
 */

#define COMMON_BASE64_ABC                                                      \
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"

/** Original Base64 for Privacy-Enhanced Mail (PEM) (RFC 1421, deprecated) */
#define PBBASE64_ENC_RFC1421 COMMON_BASE64_ABC "+/"

/** Base64 transfer encoding for MIME (RFC 2045)*/
#define PBBASE64_ENC_RFC2045 COMMON_BASE64_ABC "+/"

/** Standard 'base64' encoding for RFC 3548 or RFC 4648 */
#define PBBASE64_ENC_RFC3548 COMMON_BASE64_ABC "+/"

/** 'Radix-64' encoding for OpenPGP (RFC 4880) */
#define PBBASE64_ENC_RFC4880 COMMON_BASE64_ABC "+/"

/** Modified Base64 encoding for UTF-7 (RFC 1642, obsoleted) */
#define PBBASE64_ENC_RFC1642 COMMON_BASE64_ABC "+/"

/** Modified Base64 encoding for IMAP mailbox names (RFC 3501) */
#define PBBASE64_ENC_RFC3501 COMMON_BASE64_ABC "+,"

/** Standard 'base64url' with URL and Filename Safe Alphabet (RFC 4648
    §5 'Table 2: The "URL and Filename safe" Base 64 Alphabet') */
#define PBBASE64_ENC_RFC4648 COMMON_BASE64_ABC "-_"

/** Unpadded 'base64url' for "named information" URI's (RFC 6920) */
#define PBBASE64_ENC_RFC6920 COMMON_BASE64_ABC "-_"

/** Non-standard URL-safe Modification of Base64 used in YUI Library
    (Y64) */
#define PBBASE64_ENC_NON_STD_YUI COMMON_BASE64_ABC "._"

/** Modified Base64 for XML name tokens (Nmtoken) */
#define PBBASE64_ENC_XML_TOKEN COMMON_BASE64_ABC ".-"

/** Modified Base64 for XML identifiers (Name) */
#define PBBASE64_ENC_XML_IDENT COMMON_BASE64_ABC "_:"

/** Modified Base64 for Program identifiers (variant 1, non standard) */
#define PBBASE64_ENC_NON_STD_PROG_IDENT COMMON_BASE64_ABC "_-"

/** Modified Base64 for Program identifiers (variant 2, non
    standard) */
#define PBBASE64_ENC_NON_STD_PROG_IDENT_2 COMMON_BASE64_ABC "._"

/** Modified Base64 for Regular expressions  (non standard) */
#define PBBASE64_ENC_NON_STD_REGEX COMMON_BASE64_ABC "!-"

/** Non-standard URL-safe Modification of Base64 used in Freenet */
#define PBBASE64_ENC_NON_STD_URL COMMON_BASE64_ABC "~-"

#define PBBASE64_SEP_RFC1421 '='
#define PBBASE64_SEP_RFC2045 '='
#define PBBASE64_SEP_RFC3548 '='
#define PBBASE64_SEP_RFC4880 '='
#define PBBASE64_SEP_RFC1642 '\0'
#define PBBASE64_SEP_RFC3501 '\0'
#define PBBASE64_SEP_RFC4648 '='
#define PBBASE64_SEP_RFC6920 '\0'
#define PBBASE64_SEP_NON_STD_YUI '-'
#define PBBASE64_SEP_XML_TOKEN '\0'
#define PBBASE64_SEP_XML_IDENT '\0'
#define PBBASE64_SEP_NON_STD_PROG_IDENT '\0'
#define PBBASE64_SEP_NON_STD_REGEX '\0'
#define PBBASE64_SEP_NON_STD_URL '='

/** Possible "schemes" of using (line) checksum in a Base64 encoded
    string. This is rarely used.
 */
enum pbbase64_line_checksum {
    /** No line checksum - most often */
    pbbase64_no_line_checksum,
    /** Use  24 bit CRC */
    pbbase64_24b_CRC,
    /** Use Luhn checksum */
    pbbase64_luhn_checksum
};

/** Options for Base64 encoding/decoding. This is descriptive enough
    for all known Base64 encoding variants.
 */
struct pbbase64_options {
    /** The alphabet to use - first 62 characters are the same in
        all known Base64 encodings.
     */
    char const* alphabet;
    /** Separator to use - NUL character if no separator */
    char separator;
    /** Maximum encoded line length - 0 if no maximum. It the encoding
        string lenght exceeds this length (and it is > 0), then a line
        ending will be inserted in the encoded string. Not yet
        supported
    */
    size_t max_line_length;
    /** Character sequence used for ending the line. Only has meaning
        if there is a maximum encoded line length. Not yet
        supported.
    */
    char const* line_end;
    /** If `true` - ignore invalid characters, else - stop
        processing and report an error. This, obviously,
        relevant only for decoding.
    */
    bool ignore_invalid_char;
    /** The line checksum to use. Only has meaning if there is a
        maximum encoded line length. Not yet supported */
    enum pbbase64_line_checksum line_cheksum;
};


/** Helper macro to define options for RFC1421 variant of Base64 */
#define PBBASE64_RFC1421_OPTIONS                                               \
    {                                                                          \
        PBBASE64_ENC_RFC1421, PBBASE64_SEP_RFC1421, 64, "\r\n", false,         \
            pbbase64_no_line_checksum                                          \
    }

/** Helper macro to define options for RFC2045 variant of Base64 */
#define PBBASE64_RFC2045_OPTIONS                                               \
    {                                                                          \
        PBBASE64_ENC_RFC2045, PBBASE64_SEP_RFC2045, 76, "\r\n", true,          \
            pbbase64_no_line_checksum                                          \
    }

/** Helper macro to define options for RFC3548 variant of Base64, also
    known as "standard" Base64.
 */
#define PBBASE64_RFC3548_OPTIONS                                               \
    {                                                                          \
        PBBASE64_ENC_RFC3548                                                   \
        , PBBASE64_SEP_RFC3548, 0, NULL, false, pbbase64_no_line_checksum      \
    }


/** Returns the length (in bytes) of Base64 encoded string for
    a memory block of @p length. */
size_t pbbase64_encoded_length(size_t length);

/** Returns the size (in members, which is almost always the same as
    bytes, for the character array (buffer) that is passed to
    pbbase64_encode() as the destination, to encode a memory block of
    @p length. This is greater than pbbase64_encoded_length(), but
    isn't sure to be `pbbase64_encoded_length() + 1`, as there may be
    other reasons for pbbase64_encode() to need more memory in the
    encoded string buffer.

*/
size_t pbbase64_char_array_size_for_encoding(size_t length);

/** Base64 encodes the memory block @p data to user allocated string
    @p s with size @p *n, using options @p options. On output, @p *n
    will hold number of characters written.

    @return 0: OK, -1: error
*/
int pbbase64_encode(pubnub_bymebl_t                data,
                    char*                          s,
                    size_t*                        n,
                    struct pbbase64_options const* options);

/** Similar to pbbase64_encode(), but allocates the memory to decode to
    and returns it. On error, pointer will be NULL and size is undefined.
*/
pubnub_bymebl_t pbbase64_encode_alloc(pubnub_bymebl_t                data,
                                      struct pbbase64_options const* options);

/** Similar to pbbase64_encode() but uses the Base64 "standard"
    variant (RFC 3548 or RFC 4648).
 */
int pbbase64_encode_std(pubnub_bymebl_t data, char* s, size_t* n);

/** Similar to pbbase64_encode_alloc() but uses the Base64 "standard"
    variant (RFC 3548 or RFC 4648).
 */
pubnub_bymebl_t pbbase64_encode_alloc_std(pubnub_bymebl_t data);


/** Returns the length (in bytes) of Base64 decoded string of length @p n */
size_t pbbase64_decoded_length(size_t n);

/** Base64 decodes the string @p s with length @p n (does not have to
    be the lenght of @p s, can be smaller) to memory block @p data
    using options @p options. The @p data will be updated with the
    actual number of bytes written to it.

    @return 0: OK, -1: error
*/
int pbbase64_decode(char const*                    s,
                    size_t                         n,
                    pubnub_bymebl_t*               data,
                    struct pbbase64_options const* options);

/** Has the effect of pbbase64_decode(s, strlen(s), data, options) */
int pbbase64_decode_str(char const*                    s,
                        pubnub_bymebl_t*               data,
                        struct pbbase64_options const* options);

/** Similar to pbbase64_decode(), but allocates the memory to decode
    to and returns it. On error, pointer will be NULL and size is undefined.
*/
pubnub_bymebl_t pbbase64_decode_alloc(char const*                    s,
                                      size_t                         n,
                                      struct pbbase64_options const* options);

/** Has the effect of ppbase64_decode_alloc(s, strlen(s), options) */
pubnub_bymebl_t pbbase64_decode_alloc_str(char const*                    s,
                                          struct pbbase64_options const* options);

/** Similar to pbbase64_decode(), but uses the the Base64 "standard"
    variant (RFC 3548 or RFC 4648).*/
int pbbase64_decode_std(char const* s, size_t n, pubnub_bymebl_t* data);

/** Has the effect of: ppbase64_decode_std(s, strlen(s), data) */
int pbbase64_decode_std_str(char const* s, pubnub_bymebl_t* data);

/** Similar to pbbase64_decode_alloc(), but uses the the Base64
    "standard" variant (RFC 3548 or RFC 4648).*/
pubnub_bymebl_t pbbase64_decode_alloc_std(char const* s, size_t n);

/** Has the effect of: ppbase64_decode_alloc_std(s, strlen(s)) */
pubnub_bymebl_t pbbase64_decode_alloc_std_str(char const* s);


#endif /* !defined INC_PBBASE64 */
