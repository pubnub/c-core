/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "lib/pubnub_dns_codec.h"
#include "core/pubnub_assert.h"
#include "core/pubnub_log.h"

#include "cgreen/cgreen.h"
#include "cgreen/mocks.h"

#include <assert.h>
#include <string.h>
#include <setjmp.h>

/* A less chatty cgreen :) */

#define attest assert_that
#define equals is_equal_to
#define streqs is_equal_to_string
#define differs is_not_equal_to
#define strdifs is_not_equal_to_string
#define ptreqs(val) is_equal_to_contents_of(&(val), sizeof(val))
#define ptrdifs(val) is_not_equal_to_contents_of(&(val), sizeof(val))
#define sets(par, val) will_set_contents_of_parameter(par, &(val), sizeof(val))
#define sets_ex will_set_contents_of_parameter
#define returns will_return


enum DNSRecordType {
    /** 32-bit IPv4 address in network byte order. */
    RecordTypeA = 1,
    /** An encoded node name. */
    RecordTypePTR =	12,
    /** The format of the data of a TXT record is context specific. */
    RecordTypeTXT =	16,
    /** 128-bit IPV6 address in network byte order. */
    RecordTypeAAAA = 28,
    /** Service locator */
    RecordTypeSRV =	33
};

enum dnsQuestionClass {
    QclassInternet = 1
};

/** Pieces used by the tests **/ 
static const char encoded_domain_name[] = "\7encoded\3two\6server\6domain\4name\1q\2up\0";
static const char encoded_abc_domain_name[] = "\1a\1b\1c\0";

static const char just_offset[] = "\377\0";
static const char offset_within_header[] = "\300\5";
static const char offset_beyond_boudary[] = "\377\377";
static const char bad_offset_format[] = "\177\0";
/* Can be used only for the first question(or answer without questions) in a row.
   (Right after the DNS message header.) */
static const char encoded_label_start_with_offset_to_itself[] = "\3www\300\14";
static const char label_start_encoded_badly_with_offset_to_itself[] = "\4www\300\14";
static const char encoded_piece1[] = "\3www\300\161";
static const char encoded_piece2[] = "\21originServerLabel\300\277";
static       char encoded_piece21[] = 
    "\77[<-------the_Longest_Label_Stretch-------63_characters------->]\301\277";
static const char encoded_long_piece1[] = 
    "\62It_is_only_with_the_heart_that_one_can_see_rightly"
    "\51What_is_essential_is_invisible_to_the_eye\321\76";
static const char encoded_long_piece2[] =
    "\40The_unexpected_is_always_upon_us"
    "\70And_of_all_the_gifts_arrayed_before_me_this_one_thought_"
    "\57_at_this_moment_in_my_life_is_the_most_precious"
    "\25And_so_we_begin_again\322\65";
/* encoded label longer than 256 characters finishing with offset */
static       char encoded_long_piece21[] =
    "\76Destiny_is_a_place_where_both_good_and_evel_wait_and_yet_their"
    "\75_very_equalty_negates_their_power_For_it_is_the_very_deeds_of"
    "\77weak_and_mortal_men_that_may_tip_the_scales_oneWay_or_the_other"
    "\73And_then_the_world_shall_know_and_you_shall_know_which_way_"
    "\42the_fates_have_chosen_you_shall_go\322\65";
static const char encoded_piece3[] = "\4name\320\177";
static const char encoded_piece31[] = "\10e-branch\4name\320\177";
static const char encoded_piece4[] = "\6domain\0";


#define BUFFER_LENGTH 8192
#define DNS_MESSAGE_HEADER_SIZE 12
#define OFFSET_FLAGS 2
#define OFFSET_QUESTION_COUNT 4
#define OFFSET_ANSWER_COUNT 6
#define NS_COUNT_OFFSET 8
#define AR_COUNT_OFFSET 10
#define TYPE_AND_CLASS_FIELDS_SIZE 4
#define TTL_FIELD_SIZE 4
#define RECORD_DATA_LENGTH_FIELD_SIZE 2

/** Test buffer */
static uint8_t m_buf[BUFFER_LENGTH];
/** Length of the formed test message */ 
static size_t m_msg_size;
/** Test message length embracing its encoded label parts */
static size_t m_msg_label_pieces_length;
/** Offset while packing encoded label */
static size_t m_offset;

enum DNSHeaderFlagMasks {
    ResponseQueryFlagMask = 0x8000,
    OpcodeMask            = 0x7800,
    TC_FlagMask           = 0x0200,
    ZeroMask              = 0x0070,
    ResponseCodeMask      = 0x000F
};

#define QUERY    !RESPONSE
#define RESPONSE true
#define make_dns_header_M(is_response, question_count, answer_count)\
    do {                                                          \
        assert(0 == m_msg_size);                                  \
        /* Flags part of the heder is used in a different manner while testing response and query.*/\
        m_buf[OFFSET_FLAGS] |= ResponseQueryFlagMask >> 8;        \
        if (is_response) {                                        \
            /* Response */                                        \
            m_buf[OFFSET_FLAGS + 1] &= ~ResponseCodeMask;         \
        }                                                         \
        else {                                                    \
            /* Query */                                           \
            m_buf[OFFSET_FLAGS] |= (OpcodeMask >> 8) | (TC_FlagMask >> 8);\
            m_buf[OFFSET_FLAGS + 1] |= ZeroMask | ResponseCodeMask;\
        }                                                         \
        m_buf[OFFSET_QUESTION_COUNT] = question_count >> 8;       \
        m_buf[OFFSET_QUESTION_COUNT + 1] = question_count & 0xFF; \
        m_buf[OFFSET_ANSWER_COUNT] = answer_count >> 8;           \
        m_buf[OFFSET_ANSWER_COUNT + 1] = answer_count & 0xFF;     \
        m_msg_size = DNS_MESSAGE_HEADER_SIZE;                     \
    } while(0)


static void set_offset(uint8_t* name, size_t length)
{
    if((1 < length) && (0xc0 == (0xc0 & name[length - 2]))) {
        m_offset = (~0xc0 & name[length - 2])*256 + name[length - 1];
        PUBNUB_LOG_TRACE("m_offset = %zu\n", m_offset);
    }
    return;
}                                                         

#define length_M(name) (sizeof name - 1)

#define append_question_M(name)                                   \
    do {                                                          \
        size_t length = length_M(name);                           \
        assert(m_msg_size +                                       \
               length +                                           \
               TYPE_AND_CLASS_FIELDS_SIZE < BUFFER_LENGTH);       \
        memcpy(m_buf + m_msg_size, name, length);                 \
        m_msg_size += length + TYPE_AND_CLASS_FIELDS_SIZE;        \
        set_offset((uint8_t*)(name), length);                     \
    } while(0)

#define place_encoded_label_piece_M(name)                         \
    do {                                                          \
        size_t length = length_M(name);                           \
        assert(m_offset + length < BUFFER_LENGTH);                \
        memcpy(m_buf + m_offset, name, length);                   \
        if (m_msg_label_pieces_length < m_offset + length) {      \
            m_msg_label_pieces_length = m_offset + length;        \
        }                                                         \
        set_offset((uint8_t*)(name), length);                     \
    } while(0) 

#define append_answer_M(name, type, data)                         \
    do {                                                          \
        size_t length           = length_M(name);                 \
        size_t recordDataLength = sizeof data;                    \
        assert(m_msg_size +                                       \
               length +                                           \
               TYPE_AND_CLASS_FIELDS_SIZE +                       \
               TTL_FIELD_SIZE +                                   \
               RECORD_DATA_LENGTH_FIELD_SIZE +                    \
               recordDataLength < BUFFER_LENGTH);                 \
        memcpy(m_buf + m_msg_size, name, length);                 \
        set_offset((uint8_t*)(name), length);                     \
        m_msg_size += length;                                     \
        m_buf[m_msg_size] = (type) >> 8;                          \
        m_buf[m_msg_size + 1] = (type) & 0xFF;                    \
        m_msg_size += TYPE_AND_CLASS_FIELDS_SIZE + TTL_FIELD_SIZE;\
        m_buf[m_msg_size] = recordDataLength >> 8;                \
        m_buf[m_msg_size + 1] = recordDataLength & 0xFF;          \
        m_msg_size += RECORD_DATA_LENGTH_FIELD_SIZE;              \
        memcpy(m_buf + m_msg_size, data, recordDataLength);       \
        m_msg_size += recordDataLength;                           \
    } while(0)

/* Makes sense calling after inserting extra label pieces into test buffer for decoding*/
static void resize_msg(void)
{
    if (m_msg_size < m_msg_label_pieces_length) {
        m_msg_size = m_msg_label_pieces_length;
    }
    return;
}

#define append_request_question_M(encoded_name, type, class)      \
    do {                                                          \
        size_t length = length_M(encoded_name);                   \
        assert(m_msg_size +                                       \
               length +                                           \
               TYPE_AND_CLASS_FIELDS_SIZE < BUFFER_LENGTH);       \
        memcpy(m_buf + m_msg_size, encoded_name, length);         \
        m_msg_size += length;                                     \
        m_buf[m_msg_size]     = type >> 8;                        \
        m_buf[m_msg_size + 1] = type && 0xFF;                     \
        m_buf[m_msg_size + 2] = class >> 8;                       \
        m_buf[m_msg_size + 3] = class && 0xFF;                    \
        m_msg_size += TYPE_AND_CLASS_FIELDS_SIZE;                 \
    } while(0)

/* Assert "catching" */
static bool        m_expect_Assert;
static jmp_buf     m_Assert_exp_jmpbuf;
static char const* m_expect_Assert_file;


void assert_handler(char const* s, const char* file, long i)
{
    printf("%s:%ld: Pubnub assert failed '%s'\n", file, i, s);
}

void test_assert_handler(char const* s, const char* file, long i)
{
    assert_handler(s, file, i);

    attest(m_expect_Assert);
    attest(m_expect_Assert_file, streqs(file));
    if (m_expect_Assert) {
        m_expect_Assert = false;
        longjmp(m_Assert_exp_jmpbuf, 1);
    }
}

#define expect_assert_in(expr, file)                               \
    {                                                              \
        m_expect_Assert      = true;                               \
        m_expect_Assert_file = file;                               \
        int val              = setjmp(m_Assert_exp_jmpbuf);        \
        if (0 == val)                                              \
            expr;                                                  \
        attest(!m_expect_Assert);                                  \
    }

Describe(pubnub_dns_codec);

BeforeEach(pubnub_dns_codec)
{
    pubnub_assert_set_handler((pubnub_assert_handler_t)assert_handler);
    m_msg_size = m_msg_label_pieces_length = m_offset = 0;
    m_buf[OFFSET_FLAGS + 1] = m_buf[OFFSET_FLAGS] = 0;
}

AfterEach(pubnub_dns_codec)
{
    PUBNUB_LOG_TRACE("========================================================\n");
}


Ensure(pubnub_dns_codec, decodes_strange_response_2_questions_2_answers)
{
    /* Resolved Ipv4 address */
    uint8_t data[] = {1,2,3,4};
    struct pubnub_ipv4_address key_addr;
    struct pubnub_ipv4_address resolved_addr;

    memset(&resolved_addr, '\0', sizeof resolved_addr);
    memset(&key_addr, '\0', sizeof key_addr);
    memcpy(key_addr.ipv4, data, sizeof key_addr.ipv4);
    /* Assembling test message(response from DNS server) with 2 questions and 2 answers.
       Not very complete though.
    */
    make_dns_header_M(RESPONSE, 2, 2);
    append_question_M(just_offset);
    append_question_M(encoded_piece1);
    append_answer_M(encoded_domain_name, RecordTypeA, data);
    append_answer_M(encoded_piece2, RecordTypeTXT, data);

    attest(pubnub_pick_resolved_address(m_buf, m_msg_size, &resolved_addr), equals(0));
    attest(memcmp(&resolved_addr, &key_addr, sizeof resolved_addr), equals(0));
}

Ensure(pubnub_dns_codec, decodes_strange_response_wrong_answers)
{
    /* Resolved Ipv4 address */
    uint8_t data[] = {1,2,3,4};
    struct pubnub_ipv4_address key_addr;
    struct pubnub_ipv4_address resolved_addr;

    memset(&resolved_addr, '\0', sizeof resolved_addr);
    memset(&key_addr, '\0', sizeof key_addr);
    make_dns_header_M(RESPONSE, 1, 2);
    append_question_M(just_offset);
    append_answer_M(encoded_domain_name, RecordTypeSRV, data);
    append_answer_M(encoded_abc_domain_name, RecordTypeTXT, data);

    attest(pubnub_pick_resolved_address(m_buf, m_msg_size, &resolved_addr), equals(-1));
    attest(memcmp(&resolved_addr, &key_addr, sizeof resolved_addr), equals(0));

    /* Message shortened to finish within last answers label */
    attest(pubnub_pick_resolved_address(m_buf, m_msg_size - 20, &resolved_addr), equals(-1));
}

Ensure(pubnub_dns_codec, decodes_label_too_long_to_fit_in_modules_buffer)
{
    /* Resolved Ipv4 address */
    uint8_t data[] = {10,20,30,40};
    struct pubnub_ipv4_address key_addr;
    struct pubnub_ipv4_address resolved_addr;

    memset(&resolved_addr, '\0', sizeof resolved_addr);
    memset(&key_addr, '\0', sizeof key_addr);
    memcpy(key_addr.ipv4, data, sizeof key_addr.ipv4);

    make_dns_header_M(RESPONSE, 1, 1);
    /** name ends with offset */
    encoded_long_piece21[length_M(encoded_long_piece21) - 2] = '\322';
    append_question_M(encoded_long_piece21);
    append_answer_M(encoded_long_piece21, RecordTypeA, data);

    attest(pubnub_pick_resolved_address(m_buf, m_msg_size, &resolved_addr), equals(0));
    attest(memcmp(&resolved_addr, &key_addr, sizeof resolved_addr), equals(0));

    /* Message shortened to end within the question label */
    attest(pubnub_pick_resolved_address(m_buf, 280, &resolved_addr), equals(-1));
}

Ensure(pubnub_dns_codec, decodes_name_too_long_for_modules_buffer_ending_with_bad_offset_format)
{
    /* Resolved Ipv4 address */
    uint8_t data[] = {100,200,30,40};
    struct pubnub_ipv4_address resolved_addr;

    make_dns_header_M(RESPONSE, 1, 1);
    /* Label finishes with bad offset format */
    encoded_long_piece21[length_M(encoded_long_piece21) - 2] = '\100';
    append_question_M(encoded_long_piece21);
    append_answer_M(encoded_long_piece21, RecordTypeA, data);

    attest(pubnub_pick_resolved_address(m_buf, m_msg_size, &resolved_addr), equals(-1));
}

Ensure(pubnub_dns_codec, decodes_name_too_long_for_modules_buffer_finishing_with_0)
{
    /* Resolved Ipv4 address */
    uint8_t data[] = {100,200,30,40};
    struct pubnub_ipv4_address key_addr;
    struct pubnub_ipv4_address resolved_addr;

    memset(&resolved_addr, '\0', sizeof resolved_addr);
    memset(&key_addr, '\0', sizeof key_addr);
    memcpy(key_addr.ipv4, data, sizeof key_addr.ipv4);

    make_dns_header_M(RESPONSE, 1, 1);
    /* Label finishes with NULL character instead of offset */
    encoded_long_piece21[length_M(encoded_long_piece21) - 2] = '_';
    encoded_long_piece21[length_M(encoded_long_piece21) - 1] = '\0';
    /* Changing the length of the last label stretch(+ 1 byte) */
    encoded_long_piece21[length_M(encoded_long_piece21) - 37] = '\43';
    append_question_M(encoded_long_piece21);
    append_answer_M(encoded_long_piece21, RecordTypeA, data);

    attest(pubnub_pick_resolved_address(m_buf, m_msg_size, &resolved_addr), equals(0));
    attest(memcmp(&resolved_addr, &key_addr, sizeof resolved_addr), equals(0));
    /* Returning previous length of the last label stretch */
    encoded_long_piece21[length_M(encoded_long_piece21) - 37] = '\42';
}

Ensure(pubnub_dns_codec, decodes_another_spooky_response)
{
    /* Resolved Ipv4 address */
    uint8_t data[] = {4,3,2,1};
    struct pubnub_ipv4_address key_addr;
    struct pubnub_ipv4_address resolved_addr;

    memset(&resolved_addr, '\0', sizeof resolved_addr);
    memset(&key_addr, '\0', sizeof key_addr);
    memcpy(key_addr.ipv4, data, sizeof key_addr.ipv4);
    /* Assembling test message(response from DNS server).
       Not very complete, nor sensible, but has its usable part.
    */
    make_dns_header_M(RESPONSE, 1, 2);
    append_question_M(encoded_abc_domain_name);
    append_answer_M(encoded_piece1, RecordTypeTXT, data);
    append_answer_M(encoded_domain_name, RecordTypeA, data);
    append_answer_M(encoded_piece2, RecordTypePTR, data);

    attest(pubnub_pick_resolved_address(m_buf, m_msg_size, &resolved_addr), equals(0));
    attest(memcmp(&resolved_addr, &key_addr, sizeof resolved_addr), equals(0));
}

Ensure(pubnub_dns_codec, decodes_response_encoded_label_splitted)
{
    /* Resolved Ipv4 address */
    uint8_t data[] = {192,168,40,37};
    struct pubnub_ipv4_address key_addr;
    struct pubnub_ipv4_address resolved_addr;

    memset(&resolved_addr, '\0', sizeof resolved_addr);
    memset(&key_addr, '\0', sizeof key_addr);
    memcpy(key_addr.ipv4, data, sizeof key_addr.ipv4);
    /* Ignores the fact that response has no questions */
    make_dns_header_M(RESPONSE, 0, 2);
    append_answer_M(encoded_piece21, RecordTypeSRV, data);
    PUBNUB_LOG_TRACE("------->forming encoded label:\n");
    append_answer_M(encoded_piece1, RecordTypeA, data);
    place_encoded_label_piece_M(encoded_piece2);
    place_encoded_label_piece_M(encoded_piece31);
    place_encoded_label_piece_M(encoded_piece4);
    PUBNUB_LOG_TRACE("------->encoded label formed:\n");
    resize_msg();

    attest(pubnub_pick_resolved_address(m_buf, m_msg_size, &resolved_addr), equals(0));
    attest(memcmp(&resolved_addr, &key_addr, sizeof resolved_addr), equals(0));
}

Ensure(pubnub_dns_codec, handles_response_no_usable_answer)
{
    /* Resolved IpvX address */
    uint8_t data[] = {192,168,1,2,17};
    uint8_t data2[] = {192,168,1,2};
    struct pubnub_ipv4_address key_addr;
    struct pubnub_ipv4_address resolved_addr;

    memset(&resolved_addr, '\0', sizeof resolved_addr);
    memset(&key_addr, '\0', sizeof key_addr);

    make_dns_header_M(RESPONSE, 1, 2);
    /* Message shorter than its header?! */
    attest(pubnub_pick_resolved_address(m_buf,
                                        DNS_MESSAGE_HEADER_SIZE - 1,
                                        &resolved_addr),
           equals(-1));
    /* Reducing the offset to contaminate second answers label
     */
    encoded_piece21[length_M(encoded_piece21) - 2] = '\300';
    append_question_M(encoded_piece21);
    /* Message doesn't contain its first question?! */
    attest(pubnub_pick_resolved_address(m_buf,
                                        DNS_MESSAGE_HEADER_SIZE + TYPE_AND_CLASS_FIELDS_SIZE - 1,
                                        &resolved_addr),
           equals(-1));
    /* Message doesn't contain type, nor class question fields?!
    */
    attest(pubnub_pick_resolved_address(m_buf,
                                        m_msg_size - TYPE_AND_CLASS_FIELDS_SIZE,
                                        &resolved_addr),
           equals(-1));
    /* Message doesn't contain complete question name(offset incomplete(1))?!
    */
    attest(pubnub_pick_resolved_address(m_buf,
                                        m_msg_size - TYPE_AND_CLASS_FIELDS_SIZE - 1,
                                        &resolved_addr),
           equals(-1));
    /* Message doesn't contain complete question name(offset missing(2))?!
    */
    attest(pubnub_pick_resolved_address(m_buf,
                                        m_msg_size - TYPE_AND_CLASS_FIELDS_SIZE - 2,
                                        &resolved_addr),
           equals(-1));
    /* Message doesn't contain complete question name(offset missing(2) and last character in
       label stretch(1))?!
    */
    attest(pubnub_pick_resolved_address(m_buf,
                                        m_msg_size - TYPE_AND_CLASS_FIELDS_SIZE - 3,
                                        &resolved_addr),
           equals(-1));
    /* Message doesn't contain its first answer?! */
    attest(pubnub_pick_resolved_address(m_buf,
                                        m_msg_size +
                                        TYPE_AND_CLASS_FIELDS_SIZE +
                                        TTL_FIELD_SIZE +
                                        RECORD_DATA_LENGTH_FIELD_SIZE - 1,
                                        &resolved_addr),
           equals(-1));

    append_answer_M(encoded_long_piece1, RecordTypeAAAA, data);
    /* Message doesn't contain complete answer name?! */
    attest(pubnub_pick_resolved_address(m_buf,
                                        m_msg_size
                                        - sizeof data
                                        - TYPE_AND_CLASS_FIELDS_SIZE
                                        - TTL_FIELD_SIZE
                                        - RECORD_DATA_LENGTH_FIELD_SIZE - 1,
                                        &resolved_addr),
           equals(-1));
    /* answer doesn't contain resource data fields */
    attest(pubnub_pick_resolved_address(m_buf,
                                        m_msg_size
                                        - sizeof data
                                        - TYPE_AND_CLASS_FIELDS_SIZE
                                        - TTL_FIELD_SIZE
                                        - RECORD_DATA_LENGTH_FIELD_SIZE,
                                        &resolved_addr),
           equals(-1));
    /* Message doesn't contain complete answer?! */
    attest(pubnub_pick_resolved_address(m_buf,
                                        m_msg_size - 1,
                                        &resolved_addr),
           equals(-1));
    place_encoded_label_piece_M(offset_within_header);
    /* Won't find the answer due to contaminated answers label */
    append_answer_M(encoded_piece21, RecordTypeA, data2);
    place_encoded_label_piece_M(bad_offset_format);
    resize_msg();

    attest(pubnub_pick_resolved_address(m_buf, m_msg_size, &resolved_addr), equals(-1));
    attest(memcmp(&resolved_addr, &key_addr, sizeof resolved_addr), equals(0));

    /* returning to safe offset value */
    encoded_piece21[length_M(encoded_piece21) - 2] = '\301';
}

Ensure(pubnub_dns_codec, handles_label_offset_to_itself_preventing_infinite_loop)
{
    /* Resolved Ipv4 address */
    uint8_t data[] = {192,168,1,1};
    struct pubnub_ipv4_address key_addr;
    struct pubnub_ipv4_address resolved_addr;

    memset(&resolved_addr, '\0', sizeof key_addr);
    memset(&key_addr, '\0', sizeof key_addr);
    memcpy(key_addr.ipv4, data, sizeof key_addr.ipv4);

    make_dns_header_M(RESPONSE, 1, 1);
    append_question_M(encoded_label_start_with_offset_to_itself);
    PUBNUB_LOG_TRACE("------->forming encoded label:\n");
    append_answer_M(encoded_piece1, RecordTypeA, data);
    place_encoded_label_piece_M(encoded_piece21);
    place_encoded_label_piece_M(encoded_piece4);
    PUBNUB_LOG_TRACE("------->encoded label formed:\n");
    resize_msg();

    attest(pubnub_pick_resolved_address(m_buf, m_msg_size, &resolved_addr), equals(0));
    attest(memcmp(&resolved_addr, &key_addr, sizeof resolved_addr), equals(0));
}

Ensure(pubnub_dns_codec, handles_response_with_0_answers)
{
    struct pubnub_ipv4_address key_addr;
    struct pubnub_ipv4_address resolved_addr;

    memset(&resolved_addr, '\0', sizeof key_addr);
    memset(&key_addr, '\0', sizeof key_addr);

    make_dns_header_M(RESPONSE, 1, 0);
    append_question_M(encoded_piece31);

    attest(pubnub_pick_resolved_address(m_buf, m_msg_size, &resolved_addr), equals(-1));
    attest(memcmp(&resolved_addr, &key_addr, sizeof resolved_addr), equals(0));
}

Ensure(pubnub_dns_codec, handles_response_reporting_error)
{
    struct pubnub_ipv4_address resolved_addr;

    /* This kind of response header reports en issue: RCODE != 0 */
    make_dns_header_M(QUERY, 1, 0);
    append_question_M(encoded_piece31);
    attest(pubnub_pick_resolved_address(m_buf, m_msg_size, &resolved_addr), equals(-1));
}

Ensure(pubnub_dns_codec, handles_response_with_no_QR_flag_set)
{
    struct pubnub_ipv4_address resolved_addr;

    make_dns_header_M(RESPONSE, 1, 0);
    m_buf[OFFSET_FLAGS] ^= ResponseQueryFlagMask >> 8;
    append_question_M(offset_beyond_boudary);
    attest(pubnub_pick_resolved_address(m_buf, m_msg_size, &resolved_addr), equals(-1));
}

Ensure(pubnub_dns_codec, handles_splitted_label_too_long_for_modules_buffer)
{
    /* Resolved Ipv4 address */
    uint8_t data[] = {192,168,2,5};
    struct pubnub_ipv4_address key_addr;
    struct pubnub_ipv4_address resolved_addr;

    memset(&resolved_addr, '\0', sizeof key_addr);
    memset(&key_addr, '\0', sizeof key_addr);
    memcpy(key_addr.ipv4, data, sizeof key_addr.ipv4);

    make_dns_header_M(RESPONSE, 0, 2);
    /* Setting Authority and Additional Record counts in DNS header and making shure
       pubnub client analisis ignores them 
    */
    m_buf[NS_COUNT_OFFSET] = 0xFF;
    m_buf[AR_COUNT_OFFSET] = 0xFF;
    PUBNUB_LOG_TRACE("------->forming encoded label:\n");
    append_answer_M(encoded_long_piece1, RecordTypeAAAA, data);
    place_encoded_label_piece_M(encoded_piece21);
    place_encoded_label_piece_M(encoded_long_piece2);
    place_encoded_label_piece_M(encoded_piece4);
    PUBNUB_LOG_TRACE("------->encoded label formed:\n");
    append_answer_M(encoded_long_piece1, RecordTypeA, data);
    resize_msg();

    attest(pubnub_pick_resolved_address(m_buf, m_msg_size, &resolved_addr), equals(0));
    attest(memcmp(&resolved_addr, &key_addr, sizeof resolved_addr), equals(0));
}

/* If there is a badly encoded label(some label stretch shorter or longer than it indicates
   its encoded length - let alone the offset) anywhere within response before usable answer,
   the answer won't be found.
 */
Ensure(pubnub_dns_codec, handles_response_label_encoded_badly)
{
    /* Resolved Ipv4 address */
    uint8_t data[] = {192,168,0,0};
    struct pubnub_ipv4_address resolved_addr;
    make_dns_header_M(RESPONSE, 1, 1);
    append_question_M(label_start_encoded_badly_with_offset_to_itself);
    append_answer_M(encoded_piece31, RecordTypeA, data);
    attest(pubnub_pick_resolved_address(m_buf, m_msg_size, &resolved_addr), equals(-1));
}

Ensure(pubnub_dns_codec, handles_response_RecordType_and_DataLength_mismatch)
{
    /* Resolved IpvX address */
    uint8_t data[]  = {255,255,0,0,0};
    uint8_t data2[] = {255,255,0,0};
    struct pubnub_ipv4_address resolved_addr;
    make_dns_header_M(RESPONSE, 1, 2);
    append_question_M(encoded_abc_domain_name);
    append_answer_M(encoded_piece3, RecordTypeA, data);
    append_answer_M(encoded_piece3, RecordTypeA, data2);
    attest(pubnub_pick_resolved_address(m_buf, m_msg_size, &resolved_addr), equals(0));
}

Ensure(pubnub_dns_codec, makes_valid_DNS_query_request)
{
    /* Server name */
    char const name[] = "rambambuli.panchi";
    char const name_encoded[] = "\12rambambuli\6panchi\0";
    /* Buffer provided for the query request to be made */
    uint8_t buf[50];
    int to_send;

    make_dns_header_M(QUERY, 1, 0);
    append_request_question_M(name_encoded, RecordTypeA, QclassInternet);
    attest(pubnub_prepare_dns_request(buf, sizeof buf, name, &to_send), equals(0));
    attest(to_send, equals(m_msg_size));
    PUBNUB_LOG_TRACE("to_send = %d\n", to_send);
    attest((buf[OFFSET_FLAGS] & m_buf[OFFSET_FLAGS]) ||
           (buf[OFFSET_FLAGS + 1] & m_buf[OFFSET_FLAGS + 1]), equals(false));
    attest(memcmp(buf + OFFSET_QUESTION_COUNT,
                  m_buf + OFFSET_QUESTION_COUNT,
                  m_msg_size - OFFSET_QUESTION_COUNT),
           equals(0));
}

Ensure(pubnub_dns_codec, handles_buffer_too_small_for_query_request)
{
    /* Server name */
    char const name[] = "rambambuli.panchi.a.to";
    char const name_encoded[] = "\12rambambuli\6panchi\1a\2to\0";
    /* This is just about right buffer size for the test at hand */
    uint8_t buf[40];
    int to_send;

    make_dns_header_M(QUERY, 1, 0);
    append_request_question_M(name_encoded, RecordTypeA, QclassInternet);
    /* Shorter buffer */
    attest(pubnub_prepare_dns_request(buf, sizeof buf - 1, name, &to_send), equals(-1));

    attest(pubnub_prepare_dns_request(buf, sizeof buf, name, &to_send), equals(0));
    PUBNUB_LOG_TRACE("to_send = %d\n", to_send);
    attest(to_send, equals(m_msg_size));
    attest((buf[OFFSET_FLAGS] & m_buf[OFFSET_FLAGS]) ||
           (buf[OFFSET_FLAGS + 1] & m_buf[OFFSET_FLAGS + 1]), equals(false));
    attest(memcmp(buf + OFFSET_QUESTION_COUNT,
                  m_buf + OFFSET_QUESTION_COUNT,
                  m_msg_size - OFFSET_QUESTION_COUNT),
           equals(0));
}

Ensure(pubnub_dns_codec, handles_name_label_stretch_too_long)
{
    /* Server name */
    char const name[] = "to.a.CrambambulipanchiLabelStretch_tooLongAnd_Just_a_little_bitLonger.domain";
    uint8_t buf[100];
    int to_send;
    attest(pubnub_prepare_dns_request(buf, sizeof buf, name, &to_send), equals(-1));
    PUBNUB_LOG_TRACE("to_send = %d\n", to_send);
}

Ensure(pubnub_dns_codec, handles_name_label_stretch_with_no_length)
{
    /* Server name */
    char name[] = ".CrambambulipanchiLongLabelStretch.domain";
    uint8_t buf[100];
    int to_send;
    attest(pubnub_prepare_dns_request(buf, sizeof buf, name, &to_send), equals(-1));
    PUBNUB_LOG_TRACE("to_send = %d\n", to_send);
    /* Cannot encode en empty string */
    *name = '\0';
    attest(pubnub_prepare_dns_request(buf, sizeof buf, name, &to_send), equals(-1));
}

/* Verify ASSERT gets fired */

Ensure(pubnub_dns_codec, fires_asserts_on_illegal_parameters)
{
    int to_send;
    struct pubnub_ipv4_address resolved_addr;
    pubnub_assert_set_handler((pubnub_assert_handler_t)test_assert_handler);
    expect_assert_in(pubnub_prepare_dns_request(NULL, 10, "pubsub.pubnub.com", &to_send),
                     "pubnub_dns_codec.c");
    expect_assert_in(pubnub_prepare_dns_request(m_buf, 3, NULL, &to_send), "pubnub_dns_codec.c");
    expect_assert_in(pubnub_prepare_dns_request(m_buf, 5, "pubsub.pubnub.com", NULL),
                     "pubnub_dns_codec.c");
    expect_assert_in(pubnub_pick_resolved_address(NULL, m_msg_size, &resolved_addr),
                     "pubnub_dns_codec.c");
    expect_assert_in(pubnub_pick_resolved_address(m_buf, m_msg_size, NULL),
                     "pubnub_dns_codec.c");
}
