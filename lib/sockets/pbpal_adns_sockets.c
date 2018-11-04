/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "lib/sockets/pbpal_adns_sockets.h"

#include "pubnub_internal.h"

#include "core/pubnub_assert.h"
#include "core/pubnub_log.h"

#if !defined(_WIN32)
#include <arpa/inet.h>
#define CAST
#else
#include <ws2tcpip.h>
#define CAST (int*)
#endif

#include <stdint.h>
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
#define RESOURCE_DATA_TYPE_OFFSET 0

/** Offset of the data length sub-field of the RESOURCE DATA */
#define RESOURCE_DATA_DATA_LEN_OFFSET 8

/** Question/query types */
enum DNSqueryType {
    /** Address - IPv4 */
    dnsA = 1,
    /** Name server */
    dnsNS = 2,
    /** Canonical name */
    dnsCNAME = 5,
    /** Start of authority */
    dnsSOA = 6,
    /** Pointer (to another location in the name space ) */
    dnsPTR = 12,
    /** Mail exchange (responsible for handling e-mail sent to the
     * domain */
    dnsMX = 15,
    /** IPv6 address - 128 bit */
    dnsAAAA = 28,
    /** Service locator */
    dnsSRV = 33,
    /** All cached records */
    dnsANY = 255
};

/** Question/query class */
enum DNSqclass { dnsqclassInternet = 1 };


/** Do the DNS QNAME (host) encoding. This strange kind of "run-time
    length encoding" will convert `"www.google.com"` to
    `"\3www\6google\3com"`.
 */
static int dns_qname_encode(uint8_t* dns, size_t n, uint8_t const* host)
{
    uint8_t*             dest = dns + 1;
    uint8_t*             lpos;
    uint8_t const* const end = dns + n;

    PUBNUB_ASSERT_OPT(n > 0);
    PUBNUB_ASSERT_OPT(host != NULL);
    PUBNUB_ASSERT_OPT(dns != NULL);

    lpos  = dns;
    *lpos = '\0';
    while (dest < end) {
        char hc = *host++;
        if ((hc != '.') && (hc != '\0')) {
            *dest++ = hc;
        }
        else {
            size_t d = dest - lpos;
            *dest++  = '\0';
            if (d > 63) {
                /* label too long */
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


/* Do the DNS label decoding. Apart from the RLE decoding of
   `3www6google3com0` -> `www.google.com`, it also has a
   "(de)compression" scheme in which a label can be shared with
   another in the same buffer.
*/
static int dns_label_decode(uint8_t*       decoded,
                            size_t         n,
                            uint8_t const* src,
                            uint8_t const* buffer,
                            size_t         buffer_size,
                            size_t*        o_bytes_to_skip)
{
    uint8_t*             dest   = decoded;
    uint8_t const* const end    = decoded + n;
    uint8_t const*       reader = src;

    PUBNUB_ASSERT_OPT(n > 0);
    PUBNUB_ASSERT_OPT(src != NULL);
    PUBNUB_ASSERT_OPT(buffer != NULL);
    PUBNUB_ASSERT_OPT(decoded != NULL);
    PUBNUB_ASSERT_OPT(o_bytes_to_skip != NULL);

    *o_bytes_to_skip = 0;
    while (dest < end) {
        uint8_t b = *reader;
        if (b & 0xC0) {
            uint16_t offset = (b & 0x3F) * 256 + reader[1];
            if (0 == *o_bytes_to_skip) {
                *o_bytes_to_skip = reader - src + 2;
            }
            if (offset >= buffer_size) {
                PUBNUB_LOG_ERROR("Error in DNS label/name decoding - offset=%d "
                                 ">= buffer_size=%lu\n",
                                 offset,
                                 buffer_size);
                *dest = '\0';
                return -1;
            }
            reader = buffer + offset;
        }
        else if (0 == b) {
            if (0 == *o_bytes_to_skip) {
                *o_bytes_to_skip = reader - src + 1;
            }
            return 0;
        }
        else {
            if (dest != decoded) {
                *dest++ = '.';
            }
            if (dest + b >= end) {
                PUBNUB_LOG_ERROR("Error in DNS label/name decoding - dest=%p + "
                                 "b=%d >= end=%p\n",
                                 dest,
                                 b,
                                 end);
                *dest = '\0';
                return -1;
            }
            memcpy(dest, reader + 1, b);
            dest[b] = '\0';
            dest += b;
            reader += b + 1;
        }
    }

    PUBNUB_LOG_ERROR(
        "Destination for decoding DNS label/name too small, n=%lu\n", n);

    return -1;
}


#if PUBNUB_LOG_LEVEL >= PUBNUB_LOG_LEVEL_TRACE
#define TRACE_SOCKADDR(str, addr)                                              \
    do {                                                                       \
        char M_h_[50];                                                         \
        char M_s_[20];                                                         \
        getnameinfo(addr,                                                      \
                    sizeof *addr,                                              \
                    M_h_,                                                      \
                    sizeof M_h_,                                               \
                    M_s_,                                                      \
                    sizeof M_s_,                                               \
                    NI_NUMERICHOST | NI_NUMERICSERV);                          \
        PUBNUB_LOG_TRACE(str "%s:%s\n", M_h_, M_s_);                           \
    } while (0)
#else
#define TRACE_SOCKADDR(str, addr)
#endif


int send_dns_query(int skt, struct sockaddr const* dest, unsigned char* host)
{
    uint8_t buf[4096];
    int     sent_to;
    int     to_send;

    buf[HEADER_ID_OFFSET]              = 0;
    buf[HEADER_ID_OFFSET + 1]          = 33; /* in lack of a better ID */
    buf[HEADER_OPTIONS_OFFSET]         = dnsoptRDmask >> 8;
    buf[HEADER_OPTIONS_OFFSET + 1]     = 0;
    buf[HEADER_QUERY_COUNT_OFFSET]     = 0;
    buf[HEADER_QUERY_COUNT_OFFSET + 1] = 1;
    memset(buf + HEADER_ANSWER_COUNT_OFFSET,
           0,
           HEADER_SIZE - HEADER_ANSWER_COUNT_OFFSET);

    TRACE_SOCKADDR("Sending DNS query to: ", dest);
    to_send = dns_qname_encode(buf + HEADER_SIZE, sizeof buf - HEADER_SIZE, host);
    if (to_send <= 0) {
        return -1;
    }
    to_send += HEADER_SIZE;
    buf[to_send]     = 0;
    buf[to_send + 1] = dnsA;
    buf[to_send + 2] = 0;
    buf[to_send + 3] = dnsqclassInternet;
    to_send += QUESTION_DATA_SIZE;

    sent_to = sendto(skt, (char*)buf, to_send, 0, dest, sizeof *dest);
    if (sent_to <= 0) {
        return socket_would_block() ? +1 : -1;
    }
    else if (to_send != sent_to) {
        PUBNUB_LOG_ERROR("sendto() sent %d out of %d bytes!\n", sent_to, to_send);
        return -1;
    }
    return 0;
}


int read_dns_response(int skt, struct sockaddr* dest, struct sockaddr_in* resolved_addr)
{
    uint8_t  buf[8192];
    uint8_t* reader;
    int      msg_size;
    unsigned addr_size = sizeof *dest;
    size_t   q_count;
    size_t   ans_count;
    size_t   i;

    msg_size = recvfrom(skt, (char*)buf, sizeof buf, 0, dest, CAST & addr_size);
    if (msg_size <= 0) {
        return socket_would_block() ? +1 : -1;
    }
    reader = buf + HEADER_SIZE;

    q_count = buf[HEADER_QUERY_COUNT_OFFSET] * 256
              + buf[HEADER_QUERY_COUNT_OFFSET + 1];
    ans_count = buf[HEADER_ANSWER_COUNT_OFFSET] * 256
                + buf[HEADER_ANSWER_COUNT_OFFSET + 1];
    PUBNUB_LOG_TRACE(
        "DNS response has: %hu Questions, %hu Answers.\n", q_count, ans_count);
    if (q_count != 1) {
        PUBNUB_LOG_INFO("Strange DNS response, we sent one question, but DNS "
                        "response doesn't have one question.\n");
    }
    for (i = 0; i < q_count; ++i) {
        uint8_t name[256];
        size_t  to_skip;

        if (0 != dns_label_decode(name, sizeof name, reader, buf, msg_size, &to_skip)) {
            return -1;
        }
        PUBNUB_LOG_TRACE(
            "DNS response, question name: %s, to_skip=%lu\n", name, to_skip);

        /* Could check for QUESTION data format (QType and QClass), but
           even if it's wrong, we don't know what to do with it, so,
           there's no use */
        reader += to_skip + QUESTION_DATA_SIZE;
    }
    for (i = 0; i < ans_count; ++i) {
        uint8_t  name[256];
        size_t   to_skip;
        size_t   r_data_len;
        unsigned r_data_type;

        if (0 != dns_label_decode(name, sizeof name, reader, buf, msg_size, &to_skip)) {
            return -1;
        }
        r_data_len = reader[to_skip + RESOURCE_DATA_DATA_LEN_OFFSET] * 256
                     + reader[to_skip + RESOURCE_DATA_DATA_LEN_OFFSET + 1];
        r_data_type = reader[to_skip + RESOURCE_DATA_TYPE_OFFSET] * 256
                      + reader[to_skip + RESOURCE_DATA_TYPE_OFFSET + 1];
        reader += to_skip + RESOURCE_DATA_SIZE;

        PUBNUB_LOG_TRACE(
            "DNS answer: %s, to_skip:%zu, type=%hu, data_len=%zu\n",
            name,
            to_skip,
            r_data_type,
            r_data_len);

        if (r_data_type == dnsA) {
            if (r_data_len != 4) {
                PUBNUB_LOG_WARNING("unexpected answer R_DATA length %zu\n",
                                   r_data_len);
                continue;
            }
            PUBNUB_LOG_TRACE("Got IPv4: %u.%u.%u.%u\n",
                             reader[0],
                             reader[1],
                             reader[2],
                             reader[3]);
            resolved_addr->sin_family = AF_INET;
            memcpy(&resolved_addr->sin_addr, reader, 4);
            reader += r_data_len;
            return 0;
        }
        else {
            /* Don't care about other resource types, for now */
            reader += r_data_len;
        }
    }

    /* Don't care about Authoritative Servers or Additional records, for now */

    return -1;
}


#if 0
#include <stdio.h>
#include <fcntl.h>
int main()
{
    struct sockaddr_in dest;

    int skt = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    int flags = fcntl(skt, F_GETFL, 0);
    fcntl(skt, F_SETFL, flags | O_NONBLOCK);

    dest.sin_family = AF_INET;
    dest.sin_port = htons(53);
    dest.sin_addr.s_addr = inet_addr("8.8.8.8");

    send_dns_query(skt, (struct sockaddr*)&dest, "pubsub.pubnub.com");

    fd_set read_set, write_set;
    int rslt;
    struct timeval timev = { 0, 300000 };

    FD_ZERO(&read_set);
    FD_SET(skt, &read_set);
    rslt = select(skt + 1, &read_set, NULL, NULL, &timev);
    if (-1 == rslt) {
        puts("select() Error!\n");
        return -1;
    }
    else if (rslt > 0) {
        struct sockaddr_in resolved_addr;
        printf("skt=%d, rslt=%d, timev.tv_sec=%ld, timev.tv_usec=%ld\n", skt, rslt, timev.tv_sec, timev.tv_usec);
        read_dns_response(skt, (struct sockaddr*)&dest, &resolved_addr);
    }
    else {
        puts("no select() event");
    }



    return 0;
}
#endif
