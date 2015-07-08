/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_GENERATE_UUID
#define      INC_PUBNUB_GENERATE_UUID

#include <stdint.h>


/** @name pubnub_generate_uuid.h

    This is an UUID generator module for Pubnub, but it is fairly
    general and can be used for other purposes. It follows the IETF
    RFC 4122 and ITU-T X.667 | ISO/IEC 9834-8
    standards/recommendations.

    It provides interfaces for 4 out of 5 algorithms defined in the
    standards. But, depending on the platform, all, some or none of
    the algorithms will be available.

    It avoids keeping state itself and leaves it to the user, in order
    to be portable.

    Some of the algorithms depend on a good quality random number
    generator to be used by either the algorithm itself, or by the
    user to provide (start/seed) input. We use (and users should,
    too):

    - RtlGenRandon() on Windows
    - `/dev/urandom` on POSIX
    
 */

struct Pubnub_UUID {
    /* Here is the generic UUID format. Byte order is MSB ("network
       order", "big-endian").

      0                   1                   2                   3
       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                          time_low                             |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |       time_mid                |         time_hi_and_version   |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |clk_seq_hi_res |  clk_seq_low  |         node (0-1)            |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                         node (2-5)                            |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+   
    */
    uint8_t uuid[16];
};

struct Pubnub_UUID_String {
    /* XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX */
    char uuid[36+1];
};

/** This generates an UUID using the v1 algorithm (time-based). This
    algorithm has some state, but, a lot of it is "node" identifier,
    which is the MAC address of your Ethernet-ish network interface,
    and most applications have one. If you don't have a MAC, you can
    use some other identifier. If it has less than 6 octets, will use
    what we have, but UUIDs will be of lesser quality. If you don't
    have an identifier to use, than you can generate a random number.
    If you don't have a random number generator, you can either give
    up, or use the pubnub_time() to obtain a high-resolution time as a
    pseudo-random number.

    Besides that, it has the @p timestamp, which is a 100ns tick timer
    that started at midnight 15 October 1582. If you have a clock,
    just convert it to this format and you're good. Since it requires
    64-bit integer support, and that is not always available, we are
    accepting it as a 8-octet array. If you don't have a clock, but
    have a timer, you can get time via pubnub_time() operation and
    later add the timer ticks to it.

    Last but not the least, there is the @p io_clock_seq, which is
    generally used if the UUID generator gets the @p i_node and
    @i_timestamp itself and also keep the state, which we don't do,
    for greater portability. We emulate this, by keeping a copy (in
    volatile memory) of the last time-stamp and seeing if it changes
    and assuming that node changes at first call, so we require the
    user to gives a random number for the clock sequence on first
    call. So, basically, on the first call, put a random value in @p
    io_clock_seq, and later just re-use the same variable, this
    function will update it as needed. If you don't have random number
    generator, you can use any pseudo-random number source (say a
    timer tick or some other event counter) - actually use as many as
    you have and mix the values (the simplest option is just to XOR
    the values you have, other is to make a message digest (MD5,
    SHA-1) of all the values).
    
    While rather complex to use, it is very portable and can be made
    to work, with effort, on pretty much any platform, without the
    need to obtain unique identifiers yourself (like you need to do
    for v3 and v5).

    @param o_uuid The place to put the generated UUID to
    @param io_clock_seq Clock Sequence - initialize to a random
    value on first call, later just reuse
    @param i_timestamp Count of 100- nanosecond intervals since
    00:00:00.00, 15 October 1582 (the date of Gregorian reform to the
    Christian calendar).
    @param i_node A 6-octet "node" identity. Designed to be
    an IEEE 802 MAC address, but if you don't have it on
    your system, you can use something else.
    
    @return 0: OK (generated), otherwise: error, algorithm not available
 */
int pubnub_generate_uuid_v1_time(struct Pubnub_UUID *o_uuid,
				 uint16_t *io_clock_seq,
				 uint8_t const i_timestamp[8],
				 uint8_t const i_node[6]
				 );

/** The name based algorithms (this - v3 and the other - v5) don't
    need any other state but the arguments they declare.
    But, they do need a hash, in this case MD5. For various
    reasons, a particular hash may not be available on
    a particular platform.

    @param uuid The place to put the generated UUID to
    @param nsid The UUID of the namespace used. We provide
    a few examples.
    @param name Pointer to the data that defines the name
    you want to use for UUID generation
    @param namelen The length of the @p name data
    @return 0: OK (generated), otherwise: error, algorithm not available
 */
int pubnub_generate_uuid_v3_name_md5(struct Pubnub_UUID *uuid,
				     struct Pubnub_UUID *nsid,
				     void *name,
				     unsigned namelen
				     );

/** The nice property of this random-base algorithm is that it needs
    no state what-so-ever. A not so nice property is that it needs a
    random number generator of good quality, and you may not have
    that on a particular platform.

    @param uuid The place to put the generated UUID to
    @return 0: OK (generated), otherwise: error, random number generator
    not available
*/
int pubnub_generate_uuid_v4_random(struct Pubnub_UUID *uuid);

/** The name based algorithms (this - v5 and the other - v3) don't
    need any other state but the arguments they declare.
    But, they do need a hash, in this case SHA-1. For various
    reasons, a particular hash may not be available on
    a particular platform.

    @param uuid The place to put the generated UUID to
    @param nsid The UUID of the namespace used. We provide
    a few examples.
    @param name Pointer to the data that defines the name
    you want to use for UUID generation
    @param namelen The length of the @p name data
    @return 0: OK (generated), otherwise: error, algorithm not available
 */
int pubnub_generate_uuid_v5_name_sha1(struct Pubnub_UUID *uuid,
				     struct Pubnub_UUID *nsid,
				     void *name,
				     unsigned namelen
				     );


/** Returns UUID as a standard HEX-based representation */
struct Pubnub_UUID_String pubnub_uuid_to_string(struct Pubnub_UUID const *uuid);

/** Compares two UUIDs (@p left and @p right) and returns:
    - 0: equal
    - <0: left < right
    - >0: left > right
*/
int pubnub_uuid_compare(struct Pubnub_UUID const *left, struct Pubnub_UUID const *right);


#endif /* !defined INC_PUBNUB_GENERATE_UUID */
