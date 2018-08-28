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


/** DNS message header */
struct DNS_HEADER {
    /** identification number */
    uint16_t id;

    /** Options. This is a bitfield, IETF bit-numerated:

       |0 |1 2 3 4| 5| 6| 7| 8| 9|10|11|12 13 14 15|
       |QR|OPCODE |AA|TC|RD|RA| /|AD|CD|   RCODE   |

       - QR query-response bit (0->query, 1->response)
       - OPCODE 0->query, 1->inverse query, 2->status
       - AA authorative answer
       - TC truncation (1 message was truncated, 0 otherwise)
       - RD recursion desired
       - RA recursion available
       - AD authenticated data
       - CD checking disabled
       - RCODE response type 0->no error, 1->format error, 2-> server fail,
       3->name eror, 4->not implemented, 5->refused
     */
    uint16_t options;

    /** number of question entries */
    uint16_t q_count;
    /** number of answer entries */
    uint16_t ans_count;
    /** number of authority entries */
    uint16_t auth_count;
    /** number of resource entries */
    uint16_t add_count;
};

enum DNSoptionsMask {
    dnsoptRCODEmask  = 0x000F,
    dnsoptCDmask     = 0x0010,
    dnsoptADmask     = 0x0020,
    dnsoptRAmask     = 0x0080,
    dnsoptRDmask     = 0x0100,
    dnsoptTCmask     = 0x0200,
    dnsoptAAmask     = 0x0400,
    dnsoptOPCODEmask = 0x7800,
    dnsoptQRmask     = 0x8000,
};


/** Constant sized fields of query structure */
struct QUESTION {
    uint16_t qtype;
    uint16_t qclass;
};

/** Constant sized fields of the resource record structure */
#pragma pack(push, 1)
struct R_DATA {
    uint16_t type;
    uint16_t _class;
    uint32_t ttl;
    uint16_t data_len;
};
#pragma pack(pop)

enum DNSqueryType {
    dnsA     = 1,
    dnsNS    = 2,
    dnsCNAME = 5,
    dnsSOA   = 6,
    dnsWKS   = 11,
    dnsPTR   = 12,
    dnsMX    = 15,
    dnsSRV   = 33,
    dnsA6    = 38,
    dnsANY   = 255
};

enum DNSopcode { dnsocQUERY, dnsocIQUERY, dnsocSTATUS };

enum DNSqclass { dnsqclassInternet = 1 };


/** Do the DNS QNAME (host) encoding. This strange kind of "run-time
    length encoding" will convert `"www.google.com"` to
    `"\3www\6google\3com"`.
 */
static unsigned char* dns_qname_encode(uint8_t* dns, size_t n, uint8_t const* host)
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
                return NULL;
            }

            *lpos = (uint8_t)(d - 1);
            lpos += d;

            if ('\0' == hc) {
                break;
            }
        }
    }

    return dns;
}


/* Do the DNS label decoding. Apart from the RLE decoding of
   `3www6google3com0` -> `www.google.com`, it also has a
   "(de)compression" scheme in which a label can be shared with
   another in the same buffer.
*/
static unsigned char* dns_label_decode(uint8_t*       label,
                                       size_t         n,
                                       uint8_t const* src,
                                       uint8_t const* buffer,
                                       size_t         buffer_size,
                                       size_t*        o_bytes_to_skip)
{
    uint8_t*             dest = label;
    uint8_t const* const end  = label + n;

    PUBNUB_ASSERT_OPT(n > 0);
    PUBNUB_ASSERT_OPT(src != NULL);
    PUBNUB_ASSERT_OPT(buffer != NULL);
    PUBNUB_ASSERT_OPT(label != NULL);
    PUBNUB_ASSERT_OPT(o_bytes_to_skip != NULL);

    *o_bytes_to_skip = 0;
    while (dest < end) {
        uint8_t b = *src;
        if (b & 0xC0) {
            uint16_t offset = (b & 0x3F) * 256 + src[1];
            if (0 == *o_bytes_to_skip) {
                *o_bytes_to_skip = dest - label + 2;
            }
            if (offset >= buffer_size) {
                *dest = '\0';
                break;
            }
            src = buffer + offset;
        }
        else if (0 == b) {
            break;
        }
        else {
            if (dest != label) {
                *dest++ = '.';
            }
            if (dest + b >= end) {
                *dest = '\0';
                break;
            }
            memcpy(dest, src + 1, b);
            dest[b] = '\0';
            dest += b;
            src += b + 1;
        }
    }
    if (*o_bytes_to_skip == 0) {
        *o_bytes_to_skip = dest - label;
    }
    return label;
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
    uint8_t            buf[4096];
    struct DNS_HEADER* dns   = (struct DNS_HEADER*)buf;
    uint8_t*           qname = buf + sizeof *dns;
    struct QUESTION*   qinfo;
    int                sent_to;
    int                to_send;

    dns->id        = htons(33); /* in lack of a better ID */
    dns->options   = htons(dnsoptRDmask);
    dns->q_count   = htons(1);
    dns->ans_count = dns->auth_count = dns->add_count = 0;

    TRACE_SOCKADDR("Sending DNS query to: ", dest);
    dns_qname_encode(qname, sizeof buf - sizeof *dns, host);

    qinfo = (struct QUESTION*)(buf + sizeof *dns + strlen((const char*)qname) + 1);
    qinfo->qtype  = htons(dnsA);
    qinfo->qclass = htons(dnsqclassInternet);
    to_send       = sizeof *dns + strlen((char*)qname) + 1 + sizeof *qinfo;
    sent_to       = sendto(skt, (char*)buf, to_send, 0, dest, sizeof *dest);

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
    uint8_t            buf[8192];
    struct DNS_HEADER* dns   = (struct DNS_HEADER*)buf;
    uint8_t*           qname = buf + sizeof *dns;
    uint8_t*           reader;
    int                i, msg_size;
    unsigned           addr_size = sizeof *dest;

    msg_size = recvfrom(skt, (char*)buf, sizeof buf, 0, dest, CAST & addr_size);
    if (msg_size <= 0) {
        return socket_would_block() ? +1 : -1;
    }

    reader = buf + sizeof *dns + strlen((const char*)qname) + 1
             + sizeof(struct QUESTION);

    PUBNUB_LOG_TRACE("DNS response has: %hu Questions, %hu Answers, %hu "
                     "Auth. Servers, %hu Additional records.\n",
                     ntohs(dns->q_count),
                     ntohs(dns->ans_count),
                     ntohs(dns->auth_count),
                     ntohs(dns->add_count));

    for (i = 0; i < ntohs(dns->ans_count); ++i) {
        uint8_t        name[256];
        size_t         to_skip;
        struct R_DATA* prdata;
        size_t         r_data_len;

        dns_label_decode(name, sizeof name, reader, buf, msg_size, &to_skip);
        prdata     = (struct R_DATA*)(reader + to_skip);
        r_data_len = ntohs(prdata->data_len);
        reader += to_skip + sizeof *prdata;

        PUBNUB_LOG_TRACE(
            "DNS answer: %s, to_skip:%zu, type=%hu, data_len=%zu\n",
            name,
            to_skip,
            ntohs(prdata->type),
            r_data_len);

        if (ntohs(prdata->type) == dnsA) {
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
