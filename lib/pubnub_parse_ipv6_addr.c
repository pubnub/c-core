/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_config.h"

#if !PUBNUB_USE_IPV6
#error PUBNUB_USE_IPV6 must be defined and set to 1 before compiling this file
#endif
#include "core/pubnub_dns_servers.h"
#include "core/pubnub_assert.h"
#include "core/pubnub_log.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define FORWARD +1
#define BACKWARD -(FORWARD)

int pubnub_parse_ipv6_addr(char const* addr, struct pubnub_ipv6_address* p)
{
    uint16_t b[8] = {0};
    /* Numerical value between two colons */
    uint16_t value = 0;
    /* hex digit value */
    uint8_t digit_value;
    /* Number of hex digits in numerical value */
    int hex_digits = 0;
    /* Number of colons found in address string */
    int colons = 0;
    /* Index of hex number(between two colons) in the array of eight 'unsigned short' members */
    unsigned i = 0;
    /* Defines the step forward(+1), or backward(-1).
       Starts going forward from the beginning of the address string.
     */
    int step = FORWARD;
    /* Number of shifts to the left as multiplying by power of number 16 */
    unsigned power_of_16;
    /* Indicates if previously read character was colon */
    bool previous_colon = false;
    char const* pos = addr;
    /* Position in the address string where analisis stops */
    char const* meeting_position;
    /* Address string length */
    unsigned len;
    
    PUBNUB_ASSERT_OPT(addr != NULL);
    PUBNUB_ASSERT_OPT(p != NULL);

    if (strchr(addr, ':') == NULL) {
        PUBNUB_LOG_ERROR("Error: pubnub_parse_ipv6_addr('%s') - "
                         "No colons in the Ipv6 address string\n",
                         addr);
        return -1;
    }
    len = strlen(addr);
    meeting_position = addr + len;
    while (pos != meeting_position) {
        if (':' == *pos) {
            ++colons;
            if (colons > 7) {
                PUBNUB_LOG_ERROR("pubnub_parse_ipv6_addr('%s') - "
                                 "More than 7 colons in the Ipv6 address string\n",
                                 addr);
                return -1;
            }
            /* Saving the value between two colons */
            b[i] = value;
            /* Checks if this is the second colon in a row */ 
            if (previous_colon) {
                if (FORWARD == step) {
                    /* Will start reading from the other end of the address string
                       from right to left
                     */
                    step = BACKWARD;
                    meeting_position = pos;
                    pos = addr + len - 1;
                    i = 7;
                }
                else {
                    PUBNUB_LOG_ERROR("Error :pubnub_parse_ipv6_addr('%s') - "
                                     "Can't have more than one close pair of colons"
                                     " in the Ipv6 address string\n",
                                     addr);
                    return -1;
                }
            }
            else {
                i += step;
                pos += step;
            }
            hex_digits = 0;
            power_of_16 = 0;
            value = 0;
            previous_colon = true;
            continue;
        }
        else if (isdigit(*pos)) {
            digit_value = *pos - '0';
        }
        else if ((('a' <= *pos) && (*pos <= 'f'))
                 || (('A' <= *pos) && (*pos <= 'F'))) {
            digit_value = toupper(*pos) - 'A' + 10;
        }
        else {
            PUBNUB_LOG_ERROR("Error :pubnub_parse_ipv6_addr('%s') - "
                             "Invalid charactes in the Ipv6 address string\n",
                             addr);
            return -1;
        }
        ++hex_digits;
        if (hex_digits > 4) {
            PUBNUB_LOG_ERROR("Error :pubnub_parse_ipv6_addr('%s') - "
                             "More than 4 hex digits together in the Ipv6 address string\n",
                             addr);
            return -1;
        }
        if (FORWARD == step) {
            /* forming the value while reading from left to right */ 
            value = (value << 4) | (uint16_t)digit_value;
        }
        else {
            /* forming the value while reading from right to left */ 
            value |= ((uint16_t)digit_value << power_of_16);
            power_of_16 += 4;
        }
        pos += step;
        previous_colon = false;
    }
    if ((FORWARD == step) && (colons != 7)) {
        PUBNUB_LOG_ERROR("Error :pubnub_parse_ipv6_addr('%s') - "
                         "Ipv6 address string is incomplete\n",
                         addr);
        return -1;
    }
    if (previous_colon && (':' == *pos) && (pos != addr + len - 1)) {
        PUBNUB_LOG_ERROR("Error :pubnub_parse_ipv6_addr('%s') - "
                         "Can't have more than one close pair of colons"
                         " in the Ipv6 address string\n",
                         addr);
        return -1;
    }        
    b[i] = value;
    /* Address is stored in network byte order(big endian) */
    for(i = 0; i < 8; i++) {
        p->ipv6[2*i] = b[i] >> 8;
        p->ipv6[2*i+1] = b[i] & 0xFF;
    }
    
    return 0;
}
