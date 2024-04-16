/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"

#include "lib/pubnub_dns_codec.h"
#include "core/pubnub_assert.h"
#include "core/pubnub_log.h"

#include <string.h>


/** Size of DNS header, in octets */
#define HEADER_SIZE 12

/** Offset of the ID field in the DNS header */
#define HEADER_ID_OFFSET 0

/** Offset of the options field in the DNS header */
#define HEADER_OPTIONS_OFFSET 2

/** Offset of the query count field in the DNS header */
#define HEADER_QUERY_COUNT_OFFSET 4

/** Offset of the answer count field in the DNS header */
#define HEADER_ANSWER_COUNT_OFFSET 6

/** Offset of the authoritive servers count field in the DNS header */
#define HEADER_AUTHOR_COUNT_OFFSET 8

/** Offset of the additional records count field in the DNS header */
#define HEADER_ADDITI_COUNT_OFFSET 10

/** Bits for the options field of the DNS header - two octets
    in total.
 */
enum DNSoptionsMask {
    /* Response type;  0: no error, 1: format error, 2: server fail,
       3: name eror, 4: not implemented, 5: refused
    */
    dnsoptRCODEmask = 0x000F,
    /** Checking disabled */
    dnsoptCDmask = 0x0010,
    /** Authentication data */
    dnsoptADmask = 0x0020,
    /** Recursion available */
    dnsoptRAmask = 0x0080,
    /** Recursion desired */
    dnsoptRDmask = 0x0100,
    /** Truncation (1 - message was truncated, 0 - was not) */
    dnsoptTCmask = 0x0200,
    /** Authorative answer */
    dnsoptAAmask = 0x0400,
    /** 0: query, 1: inverse query, 2: status */
    dnsoptOPCODEmask = 0x7800,
    /** 0: query, 1: response */
    dnsoptQRmask = 0x8000,
};


/** Size of non-name data in the QUESTION field of a DNS mesage,
    in octets */
#define QUESTION_DATA_SIZE 4

/** Size of non-name data in the RESOURCE DATA field of a DNS mesage,
    in octets */
#define RESOURCE_DATA_SIZE 10

/** Offset of the type sub-field of the RESOURCE DATA */
#define RESOURCE_DATA_TYPE_OFFSET -10

/** Offset of the data length sub-field of the RESOURCE DATA */
#define RESOURCE_DATA_TTL_OFFSET -6

/** Offset of the data length sub-field of the RESOURCE DATA */
#define RESOURCE_DATA_DATA_LEN_OFFSET -2


/** Question/query class */
enum DNSqclass { dnsqclassInternet = 1 };


/* Maximum number of characters until the next dot('.'), or finishing NULL('\0') in array of bytes */
#define MAX_ALPHABET_STRETCH_LENGTH 63

/* Maximum passes through the loop allowed while decoding label, considering that it may contain
   up to five or six dots.
   Defined in order to detect erroneous offset pointers infinite loops.
   (Represents maximum  #offsets detected while decoding)
*/
#define MAXIMUM_LOOP_PASSES 10

/** Do the DNS QNAME (host) encoding. This strange kind of "run-time
    length encoding" will convert `"www.google.com"` to
    `"\3www\6google\3com"`.
    @param dns  Pointer to the buffer where encoded host label will be placed
    @param n    Maximum buffer length provided
    @param host Label to encode

    @return 'Encoded-dns-label-length' on success, -1 on error
 */
static int dns_qname_encode(uint8_t* dns, size_t n, char const* host)
{
    uint8_t*             dest = dns + 1;
    uint8_t*             lpos;

    PUBNUB_ASSERT_OPT(n > 0);
    PUBNUB_ASSERT_OPT(host != NULL);
    PUBNUB_ASSERT_OPT(dns != NULL);

    lpos  = dns;
    *lpos = '\0';
    for(;;) {
        char hc = *host++;
        if ((hc != '.') && (hc != '\0')) {
            *dest++ = hc;
        }
        else {
            size_t d = dest - lpos;
            *dest++  = '\0';
            if ((1 == d) || (MAX_ALPHABET_STRETCH_LENGTH < d - 1)) {
                PUBNUB_LOG_ERROR("Error: in DNS label/name encoding.\n"
                                 "host    ='%s',\n"
                                 "encoded ='%s',\n",
                                 host - (dest - 1 - dns),
                                 dns);
                if (d > 1) {
                    PUBNUB_LOG_ERROR("Label too long: stretch_length=%zu > MAX_ALPHABET_STRETCH_LENGTH=%d\n",
                                     d - 1,
                                     MAX_ALPHABET_STRETCH_LENGTH);
                }
                else {
                    PUBNUB_LOG_ERROR("Label stretch has no length.\n");
                }
                return -1;
            }

            *lpos = (uint8_t)(d - 1);
            lpos += d;

            if ('\0' == hc) {
                break;
            }
        }
    }

    return dest - dns;
}


int pbdns_prepare_dns_request(uint8_t* buf,
                              size_t buf_size,
                              char const* host,
                              int *to_send,
                              enum DNSqueryType query_type)
{
    int qname_encoded_length;
    int len = 0;

    PUBNUB_ASSERT_OPT(buf != NULL);
    PUBNUB_ASSERT_OPT(host != NULL);
    PUBNUB_ASSERT_OPT(to_send != NULL);

    /* First encoded_length + label_end('\0') make 2 extra bytes */
    if (HEADER_SIZE + strlen((char*)host) + 2 + QUESTION_DATA_SIZE > buf_size) {
        *to_send = 0;
        PUBNUB_LOG_ERROR(
            "Error: While preparing DNS request - Buffer too small! buf_size=%zu, required_size=%zu\n"
            "host_name=%s\n",
            buf_size,
            HEADER_SIZE + strlen((char*)host) + 2 + QUESTION_DATA_SIZE,
            (char*)host);
        return -1;
    }
    buf[HEADER_ID_OFFSET]              = 0;
    buf[HEADER_ID_OFFSET + 1]          = 33; /* in lack of a better ID */
    buf[HEADER_OPTIONS_OFFSET]         = dnsoptRDmask >> 8;
    buf[HEADER_OPTIONS_OFFSET + 1]     = 0;
    buf[HEADER_QUERY_COUNT_OFFSET]     = 0;
    buf[HEADER_QUERY_COUNT_OFFSET + 1] = 1;
    memset(buf + HEADER_ANSWER_COUNT_OFFSET,
           0,
           HEADER_SIZE - HEADER_ANSWER_COUNT_OFFSET);
    len += HEADER_SIZE;

    qname_encoded_length = dns_qname_encode(buf + HEADER_SIZE, buf_size - HEADER_SIZE, host);
    if (qname_encoded_length <= 0) {
        *to_send = len;
        return -1;
    }
    len += qname_encoded_length;

    buf[len]     = 0;
    buf[len + 1] = query_type;
    buf[len + 2] = 0;
    buf[len + 3] = dnsqclassInternet;
    *to_send = len + QUESTION_DATA_SIZE;

    return 0;
}

static int handle_offset(uint8_t         pass,
                         uint8_t const** o_reader,
                         uint8_t const*  buffer,
                         size_t          buffer_size)
{
    uint16_t       offset;
    uint8_t const* reader;

    PUBNUB_ASSERT_OPT(buffer != NULL);
    PUBNUB_ASSERT_OPT(o_reader != NULL);
    reader = *o_reader;
    PUBNUB_ASSERT_OPT(buffer < reader);

    if ((reader + 1) >= (buffer + buffer_size)) {
        PUBNUB_LOG_ERROR("Error: in DNS label/name decoding - "
                         "pointer in encoded label points outside the message while reading offset:\n"
                         "reader=%p + 1 >= buffer=%p + buffer_size=%zu\n",
                         reader,
                         buffer,
                         buffer_size);
        return -1;
    }
    if (pass > MAXIMUM_LOOP_PASSES) {
        PUBNUB_LOG_ERROR("Error: in DNS label/name decoding - "
                         "Too many offset jumps(%hhu).\n",
                         pass);
        return +1;
    }
    offset = (reader[0] & 0x3F) * 256 + reader[1];
    if (offset < HEADER_SIZE) {
        PUBNUB_LOG_ERROR("Error: in DNS label/name decoding - "
                         "Offset within header : offset=%hu, HEADER_SIZE=%d\n",
                         offset,
                         HEADER_SIZE);
        return +1;
    }
    if (offset >= buffer_size) {
        PUBNUB_LOG_ERROR("Error: in DNS label/name decoding - offset=%hu >= buffer_size=%zu\n",
                         offset,
                         buffer_size);
        return +1;
    }
    *o_reader = buffer + offset;
    return 0;
}

static int forced_skip(uint8_t         pass,
                       uint8_t const** o_reader,
                       uint8_t const*  buffer,
                       size_t          buffer_size)
{
    uint8_t const* reader;

    PUBNUB_ASSERT_OPT(buffer != NULL);
    PUBNUB_ASSERT_OPT(o_reader != NULL);
    PUBNUB_ASSERT_OPT(buffer < *o_reader);
    PUBNUB_ASSERT_OPT(*o_reader < (buffer + buffer_size));

    reader = *o_reader;
    for(;;) {
        uint8_t b = *reader;
        if (0xC0 == (b & 0xC0)) {
            *o_reader = reader + 2;            
            return 0;
        }
        else if (0 == b) {
            *o_reader = reader + 1;
            return 0;
        }
        else if (0 == (b & 0xC0)) {
            /* (buffer + buffer_size) points to the first octet outside the buffer,
               while (reader + b + 1) has to be inside of it
             */
            if ((reader + b + 1) >= (buffer + buffer_size)) {
                PUBNUB_LOG_ERROR(
                    "Error: reading DNS response - forced_skip() - "
                    "Pointer in encoded label points outside the message:\n"
                    "reader=%p + b=%d + 1 >= buffer=%p + buffer_size=%zu\n",
                    reader,
                    b,
                    buffer,
                    buffer_size);
                return -1;
            }
            reader += b + 1;
        }
        else {
            PUBNUB_LOG_ERROR("Error: reading DNS response - forced_skip() - "
                             "Bad offset format: b & 0xC0=%d, &b=%p\n",
                             b & 0xC0,
                             &b);
            return -1;
        }
    }
    return -1;
}

/* Do the DNS label decoding. Apart from the RLE decoding of
   `3www6google3com0` -> `www.google.com`, it also has a
   "(de)compression" scheme in which a label can be shared with
   another in the same buffer.
   @param decoded         Pointer to memory section for decoded label
   @param n               Maximum length of the section in bytes
   @param src             address of the container with the offset(position within shared buffer
                          from where reading starts)
   @param buffer          Beginning of the buffer
   @param buffer_size     Complete buffer size
   @param o_bytes_to_skip If function succeeds, points to the offset(position within buffer)
                          available for 'next' buffer access
   @return 0 on success, -1 on error
*/
static int dns_label_decode(uint8_t*       decoded,
                            size_t         n,
                            uint8_t const* src,
                            uint8_t const* buffer,
                            size_t         buffer_size,
                            size_t*        o_bytes_to_skip)
{
    uint8_t*             dest        = decoded;
    uint8_t const* const end         = decoded + n;
    uint8_t const*       reader      = src;
    uint8_t              pass        = 0;

    PUBNUB_ASSERT_OPT(n > 0);
    PUBNUB_ASSERT_OPT(src != NULL);
    PUBNUB_ASSERT_OPT(buffer != NULL);
    PUBNUB_ASSERT_OPT(src < (buffer + buffer_size));
    PUBNUB_ASSERT_OPT(decoded != NULL);
    PUBNUB_ASSERT_OPT(o_bytes_to_skip != NULL);

    *o_bytes_to_skip = 0;
    for(;;) {
        uint8_t b = *reader;
        if (0xC0 == (b & 0xC0)) {
            if (0 == *o_bytes_to_skip) {
                *o_bytes_to_skip = reader - src + 2;
            }
            if(handle_offset(++pass, &reader, buffer, buffer_size) != 0) {
                *dest = '\0';
                return -1;
            }
        }
        else if (0 == b) {
            if (0 == *o_bytes_to_skip) {
                *o_bytes_to_skip = reader - src + 1;
            }
            return 0;
        }
        else if (0 == (b & 0xC0)) {
            if (dest != decoded) {
                *dest++ = '.';
            }
            if (dest + b >= end) {
                PUBNUB_LOG_ERROR("Error: in DNS label/name decoding - dest=%p + b=%d >= end=%p - "
                                 "Destination for decoded label/name too small, n=%zu\n",
                                 dest,
                                 b,
                                 end,
                                 n);
                if (0 == *o_bytes_to_skip) {
                    if (forced_skip(pass, &reader, buffer, buffer_size) == 0) {
                        *o_bytes_to_skip = reader - src;
                    }
                }
                *dest = '\0';
                return -1;
            }
            /* (buffer + buffer_size) points to the first octet outside the buffer,
                while (reader + b + 1) has to be inside of it
             */
            if ((reader + b + 1) >= (buffer + buffer_size)) {
                PUBNUB_LOG_ERROR("Error: in DNS label/name decoding - "
                                 "Pointer in encoded label points outside the message:\n"
                                 "reader=%p + b=%d + 1 >= buffer=%p + buffer_size=%zu\n",
                                 reader,
                                 b,
                                 buffer,
                                 buffer_size);
                *dest = '\0';
                return -1;
            }
            memcpy(dest, reader + 1, b);
            dest[b] = '\0';
            dest += b;
            reader += b + 1;
        }
        else {
            PUBNUB_LOG_ERROR("Error: in DNS label/name decoding - "
                             "Bad offset format: b & 0xC0=%d, &b=%p\n",
                             b & 0xC0,
                             &b);
            *dest = '\0';
            return -1;
        }
    }
    /* The only way to reach this code, at the time of this writing, is if n == 0, which we check
       with an 'assert', but it is left here 'just in case'*/
    PUBNUB_LOG_ERROR("Error: in DNS label/name decoding - "
                     "Destination for decoding label/name too small, n=%zu\n", n);
    return -1;
}

static int read_header(uint8_t const*  buf,
                       size_t          msg_size,
                       size_t*         o_q_count,
                       size_t*         o_ans_count)
{
    uint16_t options;

    PUBNUB_ASSERT_OPT(buf != NULL);
    PUBNUB_ASSERT_OPT(o_q_count != NULL);
    PUBNUB_ASSERT_OPT(o_ans_count != NULL);

    if (HEADER_SIZE > msg_size) {
        PUBNUB_LOG_ERROR(
            "Error: DNS response is shorter than its header - msg_size=%zu, HEADER_SIZE=%d\n",
            msg_size,
            HEADER_SIZE); 
        return -1;
    }
    options = ((uint16_t)buf[HEADER_OPTIONS_OFFSET] << 8) | (uint16_t)buf[HEADER_OPTIONS_OFFSET + 1];
    if (!(options & dnsoptQRmask)) {
        PUBNUB_LOG_ERROR("Error: DNS response doesn't have QR flag set!\n"); 
        return -1;
    }
    if (options & dnsoptRCODEmask) {
        PUBNUB_LOG_ERROR("Error: DNS response reports an error - RCODE = %d\n",
                         buf[HEADER_OPTIONS_OFFSET + 1] & dnsoptRCODEmask); 
        return -1;
    }
    *o_q_count = buf[HEADER_QUERY_COUNT_OFFSET] * 256
              + buf[HEADER_QUERY_COUNT_OFFSET + 1];
    *o_ans_count = buf[HEADER_ANSWER_COUNT_OFFSET] * 256
                + buf[HEADER_ANSWER_COUNT_OFFSET + 1];
    PUBNUB_LOG_TRACE(
        "DNS response has: %zu Questions, %zu Answers.\n", *o_q_count, *o_ans_count);

    return 0;
}

static int skip_questions(uint8_t const** o_reader,
                          uint8_t const*  buf,
                          uint8_t const*  end,
                          size_t          q_count)
{
    size_t   i;
    uint8_t const* reader;

    PUBNUB_ASSERT_OPT(buf != NULL);
    PUBNUB_ASSERT_OPT(end != NULL);
    PUBNUB_ASSERT_OPT(buf < end);
    PUBNUB_ASSERT_OPT(o_reader != NULL);

    reader = *o_reader;
    for (i = 0; i < q_count; ++i) {
        uint8_t name[256];
        size_t  to_skip;

        if (reader + QUESTION_DATA_SIZE > end) {
#ifndef ESP_PLATFORM
            PUBNUB_LOG_ERROR("Error: DNS response erroneous, or incomplete:\n"
                             "reader=%p + QUESTION_DATA_SIZE=%d > buf=%p + msg_size=%ld\n",
                             reader,
                             QUESTION_DATA_SIZE,
                             buf,
                             end - buf + 1);
#else
            PUBNUB_LOG_ERROR("Error: DNS response erroneous, or incomplete:\n"
                             "reader=%p + QUESTION_DATA_SIZE=%d > buf=%p + msg_size=%d\n",
                             reader,
                             QUESTION_DATA_SIZE,
                             buf,
                             end - buf + 1);
#endif
            return -1;
        }
        /* Even if label decoding reports an error(having offsets messed up, maybe, or buffer too
           small for decoded label), 'bytes_to_skip' may be set and we can keep looking usable answer
         */
        if (dns_label_decode(name, sizeof name, reader, buf, (end - buf + 1), &to_skip) != 0) {
            if (0 == to_skip) {
                return -1;
            }
        }
        PUBNUB_LOG_TRACE("DNS response, %zu. question name: %s, to_skip=%zu\n",
                         i+1,
                         name,
                         to_skip);

        /* Could check for QUESTION data format (QType and QClass), but
           even if it's wrong, we don't know what to do with it, so,
           there's no use */
        reader += to_skip + QUESTION_DATA_SIZE;
    }
    *o_reader = reader;
    return 0;
}

static int check_answer(const uint8_t** o_reader,
                        unsigned        r_data_type,
                        size_t          r_data_len,
                        bool*           p_address_found,                
                        struct pubnub_ipv4_address* resolved_addr_ipv4
                        IPV6_ADDR_ARGUMENT_DECLARATION
                        PBDNS_OPTIONAL_PARAMS_DECLARATIONS)
{
    PUBNUB_ASSERT_OPT(o_reader != NULL);

    if ((dnsA == r_data_type) && (resolved_addr_ipv4 != NULL)) {
        const uint8_t* reader = *o_reader;
        *o_reader += r_data_len;
        if (r_data_len != 4) {
            PUBNUB_LOG_ERROR("Error: Unexpected answer R_DATA length:%zu "
                             "for DNS resolved Ipv4 address.\n",
                             r_data_len);
            return -1;
        }
        PUBNUB_LOG_TRACE("Got IPv4: %u.%u.%u.%u\n",
                         reader[0],
                         reader[1],
                         reader[2],
                         reader[3]);
        if (false == *p_address_found) {
            memcpy(resolved_addr_ipv4->ipv4, reader, 4);
            *p_address_found = true;
        }
#if PUBNUB_USE_MULTIPLE_ADDRESSES
        if (spare_addresses->n_ipv4 < PUBNUB_MAX_IPV4_ADDRESSES) {
            /* Time to live. Network byte order - big endian */
            uint32_t ttl_ipv4 = ((uint32_t)reader[RESOURCE_DATA_TTL_OFFSET] << 24) |
                                ((uint32_t)reader[RESOURCE_DATA_TTL_OFFSET + 1] << 16) |
                                ((uint32_t)reader[RESOURCE_DATA_TTL_OFFSET + 2] << 8) |
                                 (uint32_t)reader[RESOURCE_DATA_TTL_OFFSET + 3];
            if (ttl_ipv4 > 0) {
                /* We have intention remembering 2 least significant bytes(lower 16 bits:
                   2^16 == 65536(seconds), considering that transaction('subscribe') is allowed
                   to last up to five minutes(300 seconds) and we aim to use only necessary
                   amount of memory.
                 */
                if (ttl_ipv4 >= 65536) {
                    PUBNUB_LOG_WARNING("Warning: ttl for ipv4 received is out of range: "
                                       "ttl_ipv4=%u seconds\n",
                                       ttl_ipv4);
                    spare_addresses->ttl_ipv4[spare_addresses->n_ipv4] = 0xFFFF;
                }
                else {
                    PUBNUB_LOG_TRACE("ttl_ipv4= %u seconds\n", ttl_ipv4);
                    spare_addresses->ttl_ipv4[spare_addresses->n_ipv4] = (uint16_t)ttl_ipv4;
                }
                memcpy(spare_addresses->ipv4_addresses[spare_addresses->n_ipv4++].ipv4, reader, 4);
            }
        }
#endif
        return 0;
    }
#if PUBNUB_USE_IPV6
    else if ((dnsAAAA == r_data_type) && (resolved_addr_ipv6 != NULL)) {
        const uint8_t* reader = *o_reader;
        *o_reader += r_data_len;
        if (r_data_len != 16) {
            PUBNUB_LOG_ERROR("Error: Unexpected answer R_DATA length:%zu "
                             "for DNS resolved Ipv6 address.\n",
                             r_data_len);
            return -1;
        }
        /* Address representation is big endian(network byte order) */
        PUBNUB_LOG_TRACE("Got IPv6: %X:%X:%X:%X:%X:%X:%X:%X\n",
                         reader[0]*256 + reader[1],
                         reader[2]*256 + reader[3],
                         reader[4]*256 + reader[5],
                         reader[6]*256 + reader[7],
                         reader[8]*256 + reader[9],
                         reader[10]*256 + reader[11],
                         reader[12]*256 + reader[13],
                         reader[14]*256 + reader[15]);
        if (false == *p_address_found) {
            memcpy(resolved_addr_ipv6->ipv6, reader, 16);
            *p_address_found = true;
        }
#if PUBNUB_USE_MULTIPLE_ADDRESSES
        if (spare_addresses->n_ipv6 < PUBNUB_MAX_IPV6_ADDRESSES) {
            /* Time to live. Network byte order - big endian */
            uint32_t ttl_ipv6 = ((uint32_t)reader[RESOURCE_DATA_TTL_OFFSET] << 24) |
                                ((uint32_t)reader[RESOURCE_DATA_TTL_OFFSET + 1] << 16) |
                                ((uint32_t)reader[RESOURCE_DATA_TTL_OFFSET + 2] << 8) |
                                 (uint32_t)reader[RESOURCE_DATA_TTL_OFFSET + 3];
            if (ttl_ipv6 > 0) {
                if (ttl_ipv6 >= 65536) {
                    PUBNUB_LOG_WARNING("Warning: ttl for ipv6 received is out of range: "
                                       "ttl_ipv6=%u seconds\n",
                                       ttl_ipv6);
                    spare_addresses->ttl_ipv6[spare_addresses->n_ipv6] = 0xFFFF;
                }
                else {
                    PUBNUB_LOG_TRACE("ttl_ipv6= %u seconds\n", ttl_ipv6);
                    spare_addresses->ttl_ipv6[spare_addresses->n_ipv6] = (uint16_t)ttl_ipv6;
                }
                memcpy(spare_addresses->ipv6_addresses[spare_addresses->n_ipv6++].ipv6, reader, 16);
            }
        }
#endif /* PUBNUB_USE_MULTIPLE_ADDRESSES */
        return 0;
    }
#endif /* PUBNUB_USE_IPV6 */
    *o_reader += r_data_len;
    /* Don't care about other resource types, for now */
    return -1;
}

static int find_the_answer(uint8_t const* reader,
                           uint8_t const* buf,
                           uint8_t const* end,
                           size_t         ans_count,
                           struct pubnub_ipv4_address* resolved_addr_ipv4
                           IPV6_ADDR_ARGUMENT_DECLARATION
                           PBDNS_OPTIONAL_PARAMS_DECLARATIONS)
{
    size_t i;
    bool address_found = false;
    
    PUBNUB_ASSERT_OPT(buf != NULL);
    PUBNUB_ASSERT_OPT(reader != NULL);
    PUBNUB_ASSERT_OPT(end != NULL);
    PUBNUB_ASSERT_OPT(buf < reader);
    PUBNUB_ASSERT_OPT(reader < end);

    for (i = 0; i < ans_count; ++i) {
        uint8_t  name[256];
        size_t   to_skip;
        size_t   r_data_len;
        unsigned r_data_type;

        if (dns_label_decode(name, sizeof name, reader, buf, (end - buf + 1), &to_skip) != 0) {
            if (0 == to_skip) {
                return -1;
            }
        }
        reader += to_skip + RESOURCE_DATA_SIZE;
        if (reader > end) {
#ifndef ESP_PLATFORM
            PUBNUB_LOG_ERROR("Error: DNS response erroneous, or incomplete:\n"
                             "reader=%p > buf=%p + msg_size=%ld :\n"
                             "to_skip=%zu, RESOURCE_DATA_SIZE=%d\n",
                             reader,
                             buf,
                             end - buf + 1,
                             to_skip,
                             RESOURCE_DATA_SIZE);
#else
            PUBNUB_LOG_ERROR("Error: DNS response erroneous, or incomplete:\n"
                             "reader=%p > buf=%p + msg_size=%d :\n"
                             "to_skip=%zu, RESOURCE_DATA_SIZE=%d\n",
                             reader,
                             buf,
                             end - buf + 1,
                             to_skip,
                             RESOURCE_DATA_SIZE);
#endif
            return -1;
        }
        /* Resource record data offsets are negative.
           Network byte order - big endian.
         */
        r_data_len = reader[RESOURCE_DATA_DATA_LEN_OFFSET] * 256
                     + reader[RESOURCE_DATA_DATA_LEN_OFFSET + 1];
        if ((reader + r_data_len) > end) {
#ifndef ESP_PLATFORM
            PUBNUB_LOG_ERROR("Error: DNS response erroneous, or incomplete:\n"
                             "reader=%p + r_data_len=%zu > buf=%p + msg_size=%ld\n",
                             reader,
                             r_data_len,
                             buf,
                             end - buf + 1);
#else
            PUBNUB_LOG_ERROR("Error: DNS response erroneous, or incomplete:\n"
                             "reader=%p + r_data_len=%zu > buf=%p + msg_size=%d\n",
                             reader,
                             r_data_len,
                             buf,
                             end - buf + 1);
#endif
            return -1;
        }
        r_data_type = reader[RESOURCE_DATA_TYPE_OFFSET] * 256
                      + reader[RESOURCE_DATA_TYPE_OFFSET + 1];
        PUBNUB_LOG_TRACE("DNS %zu. answer: %s, to_skip:%zu, type=%u, data_len=%zu\n",
                         i+1,
                         name,
                         to_skip,
                         r_data_type,
                         r_data_len);
        if(check_answer(&reader,
                        r_data_type,
                        r_data_len,
                        &address_found,
                        resolved_addr_ipv4
                        IPV6_ADDR_ARGUMENT
                        PBDNS_OPTIONAL_PARAMS) == 0) {
#if !PUBNUB_USE_MULTIPLE_ADDRESSES
            return 0;
#endif
        }
    }
    /* Don't care about Authoritative Servers for now */
    return address_found ? 0 : -1;
}

int pbdns_pick_resolved_addresses(uint8_t const* buf,
                                  size_t msg_size,
                                  struct pubnub_ipv4_address* resolved_addr_ipv4
                                  IPV6_ADDR_ARGUMENT_DECLARATION
                                  PBDNS_OPTIONAL_PARAMS_DECLARATIONS)
{
    size_t         q_count;
    size_t         ans_count;
    uint8_t const* reader;
    uint8_t const* end;

    PUBNUB_ASSERT_OPT(buf != NULL);
#if PUBNUB_USE_IPV6
    PUBNUB_ASSERT_OPT((resolved_addr_ipv4 != NULL) || (resolved_addr_ipv6 != NULL));
#else
    PUBNUB_ASSERT_OPT(resolved_addr_ipv4 != NULL);
#endif

    if (read_header(buf, msg_size, &q_count, &ans_count) != 0) {
        return -1;
    }
    if (0 == ans_count) {
        return -1;
    }
    reader = buf + HEADER_SIZE;
    end    = buf + msg_size;
    if (skip_questions(&reader, buf, end, q_count) != 0) {
        return -1;
    }
    if (reader > end) {
        PUBNUB_LOG_ERROR("Error: DNS message incomplete - answers missing."
                         "reader=%p > buf=%p + msg_size=%zu\n",
                         reader,
                         buf,
                         msg_size);

        return -1;
    }

    return find_the_answer(reader,
                           buf,
                           end,
                           ans_count,
                           resolved_addr_ipv4
                           IPV6_ADDR_ARGUMENT
                           PBDNS_OPTIONAL_PARAMS);
}
