/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"

#if PUBNUB_SET_DNS_SERVERS
#include "core/pubnub_dns_servers.h"
#include "lib/pubnub_parse_ipv4_addr.h"
#else
#error PUBNUB_SET_DNS_SERVERS must be defined and set to 1 before compiling this file
#endif

#if PUBNUB_USE_IPV6
#include "lib/pubnub_parse_ipv6_addr.h"
#endif

#include "core/pubnub_assert.h"
#include "core/pubnub_log.h"

#include <string.h>
#include <sys/types.h>


pubnub_mutex_static_decl_and_init(m_lock);
static struct pubnub_ipv4_address m_primary_dns_server_ipv4 pubnub_guarded_by(m_lock);
static struct pubnub_ipv4_address m_secondary_dns_server_ipv4 pubnub_guarded_by(m_lock);
#if PUBNUB_USE_IPV6
static struct pubnub_ipv6_address m_primary_dns_server_ipv6 pubnub_guarded_by(m_lock);
static struct pubnub_ipv6_address m_secondary_dns_server_ipv6 pubnub_guarded_by(m_lock);
#endif /* PUBNUB_USE_IPV6 */


void pubnub_dns_servers_deinit(void)
{
    pubnub_mutex_destroy(m_lock);
}

int pubnub_dns_set_primary_server_ipv4(struct pubnub_ipv4_address ipv4_address)
{
    uint8_t* ipv4 = ipv4_address.ipv4;

    PUBNUB_LOG_TRACE("pubnub_dns_set_primary_server_ipv4(%u.%u.%u.%u).\n",
                     ipv4[0],
                     ipv4[1],
                     ipv4[2],
                     ipv4[3]);

    pubnub_mutex_init_static(m_lock);
    pubnub_mutex_lock(m_lock);
    memcpy(m_primary_dns_server_ipv4.ipv4,
           ipv4_address.ipv4,
           sizeof m_primary_dns_server_ipv4.ipv4);
    pubnub_mutex_unlock(m_lock);
    return 0;
}


int pubnub_dns_set_primary_server_ipv4_str(char const* ipv4_str)
{
    int ret;

    PUBNUB_ASSERT_OPT(ipv4_str != NULL);
    PUBNUB_LOG_TRACE("pubnub_dns_set_primary_server_ipv4_str(%s)\n", ipv4_str);

    pubnub_mutex_init_static(m_lock);
    pubnub_mutex_lock(m_lock);
    ret = pubnub_parse_ipv4_addr(ipv4_str, &m_primary_dns_server_ipv4);
    pubnub_mutex_unlock(m_lock);

    return ret;
}


int pubnub_dns_set_secondary_server_ipv4(struct pubnub_ipv4_address ipv4_address)
{
    uint8_t* ipv4 = ipv4_address.ipv4;

    PUBNUB_LOG_TRACE("pubnub_dns_set_secondary_server_ipv4(%u.%u.%u.%u)\n",
                     ipv4[0],
                     ipv4[1],
                     ipv4[2],
                     ipv4[3]);

    pubnub_mutex_init_static(m_lock);
    pubnub_mutex_lock(m_lock);
    memcpy(m_secondary_dns_server_ipv4.ipv4,
           ipv4_address.ipv4,
           sizeof m_secondary_dns_server_ipv4.ipv4);
    pubnub_mutex_unlock(m_lock);
    return 0;
}


int pubnub_dns_set_secondary_server_ipv4_str(char const* ipv4_str)
{
    int ret;

    PUBNUB_ASSERT_OPT(ipv4_str != NULL);
    PUBNUB_LOG_TRACE("pubnub_dns_set_secondary_server_ipv4_str(%s)\n", ipv4_str);

    pubnub_mutex_init_static(m_lock);
    pubnub_mutex_lock(m_lock);
    ret = pubnub_parse_ipv4_addr(ipv4_str, &m_secondary_dns_server_ipv4);
    pubnub_mutex_unlock(m_lock);

    return ret;
}


int pubnub_get_dns_primary_server_ipv4(struct pubnub_ipv4_address* o_ipv4)
{
    int            ret;
    const uint8_t* ipv4 = m_primary_dns_server_ipv4.ipv4;

    PUBNUB_ASSERT_OPT(o_ipv4 != NULL);

    pubnub_mutex_init_static(m_lock);
    pubnub_mutex_lock(m_lock);

    PUBNUB_LOG_TRACE("pubnub_dns_get_primary_server_ipv4(%u.%u.%u.%u).\n",
                     ipv4[0],
                     ipv4[1],
                     ipv4[2],
                     ipv4[3]);

    if (0 == m_primary_dns_server_ipv4.ipv4[0]) {
        ret = -1;
    }
    else {
        memcpy(o_ipv4->ipv4,
               m_primary_dns_server_ipv4.ipv4,
               sizeof m_primary_dns_server_ipv4.ipv4);
        ret = 0;
    }
    pubnub_mutex_unlock(m_lock);
    return ret;
}


int pubnub_get_dns_secondary_server_ipv4(struct pubnub_ipv4_address* o_ipv4)
{
    int            ret;
    const uint8_t* ipv4 = m_secondary_dns_server_ipv4.ipv4;

    PUBNUB_ASSERT_OPT(o_ipv4 != NULL);

    pubnub_mutex_init_static(m_lock);
    pubnub_mutex_lock(m_lock);

    PUBNUB_LOG_TRACE("pubnub_dns_get_secondary_server_ipv4(%u.%u.%u.%u).\n",
                     ipv4[0],
                     ipv4[1],
                     ipv4[2],
                     ipv4[3]);

    if (0 == m_secondary_dns_server_ipv4.ipv4[0]) {
        ret = -1;
    }
    else {
        memcpy(o_ipv4->ipv4,
               m_secondary_dns_server_ipv4.ipv4,
               sizeof m_secondary_dns_server_ipv4.ipv4);
        ret = 0;
    }
    pubnub_mutex_unlock(m_lock);
    return ret;
}

#if PUBNUB_USE_IPV6
int pubnub_dns_set_primary_server_ipv6(struct pubnub_ipv6_address ipv6_address)
{
    uint8_t* ipv6 = ipv6_address.ipv6;

    PUBNUB_LOG_TRACE("pubnub_dns_set_primary_server_ipv6(%u:%u:%u:%u:%u:%u:%u:%u).\n",
                     ipv6[0]*256 + ipv6[1],
                     ipv6[2]*256 + ipv6[3],
                     ipv6[4]*256 + ipv6[5],
                     ipv6[6]*256 + ipv6[7],
                     ipv6[8]*256 + ipv6[9],
                     ipv6[10]*256 + ipv6[11],
                     ipv6[12]*256 + ipv6[13],
                     ipv6[14]*256 + ipv6[15]);

    pubnub_mutex_init_static(m_lock);
    pubnub_mutex_lock(m_lock);
    memcpy(m_primary_dns_server_ipv6.ipv6,
           ipv6_address.ipv6,
           sizeof m_primary_dns_server_ipv6.ipv6);
    pubnub_mutex_unlock(m_lock);
    return 0;
}


int pubnub_dns_set_primary_server_ipv6_str(char const* ipv6_str)
{
    int ret;

    PUBNUB_ASSERT_OPT(ipv6_str != NULL);
    PUBNUB_LOG_TRACE("pubnub_dns_set_primary_server_ipv6_str(%s)\n", ipv6_str);

    pubnub_mutex_init_static(m_lock);
    pubnub_mutex_lock(m_lock);
    ret = pubnub_parse_ipv6_addr(ipv6_str, &m_primary_dns_server_ipv6);
    pubnub_mutex_unlock(m_lock);

    return ret;
}


int pubnub_dns_set_secondary_server_ipv6(struct pubnub_ipv6_address ipv6_address)
{
    uint8_t* ipv6 = ipv6_address.ipv6;

    PUBNUB_LOG_TRACE("pubnub_dns_set_secondary_server_ipv6(%u:%u:%u:%u:%u:%u:%u:%u).\n",
                     ipv6[0]*256 + ipv6[1],
                     ipv6[2]*256 + ipv6[3],
                     ipv6[4]*256 + ipv6[5],
                     ipv6[6]*256 + ipv6[7],
                     ipv6[8]*256 + ipv6[9],
                     ipv6[10]*256 + ipv6[11],
                     ipv6[12]*256 + ipv6[13],
                     ipv6[14]*256 + ipv6[15]);

    pubnub_mutex_init_static(m_lock);
    pubnub_mutex_lock(m_lock);
    memcpy(m_secondary_dns_server_ipv6.ipv6,
           ipv6_address.ipv6,
           sizeof m_secondary_dns_server_ipv6.ipv6);
    pubnub_mutex_unlock(m_lock);
    return 0;
}


int pubnub_dns_set_secondary_server_ipv6_str(char const* ipv6_str)
{
    int ret;

    PUBNUB_ASSERT_OPT(ipv6_str != NULL);
    PUBNUB_LOG_TRACE("pubnub_dns_set_secondary_server_ipv6_str(%s)\n", ipv6_str);

    pubnub_mutex_init_static(m_lock);
    pubnub_mutex_lock(m_lock);
    ret = pubnub_parse_ipv6_addr(ipv6_str, &m_secondary_dns_server_ipv6);
    pubnub_mutex_unlock(m_lock);

    return ret;
}


int pubnub_get_dns_primary_server_ipv6(struct pubnub_ipv6_address* o_ipv6)
{
    int            ret;
    const uint8_t* ipv6 = m_primary_dns_server_ipv6.ipv6;
    uint8_t        zero[sizeof m_primary_dns_server_ipv6.ipv6] = {0};

    PUBNUB_ASSERT_OPT(o_ipv6 != NULL);

    pubnub_mutex_init_static(m_lock);
    pubnub_mutex_lock(m_lock);

    PUBNUB_LOG_TRACE("pubnub_dns_get_primary_server_ipv6(%u:%u:%u:%u:%u:%u:%u:%u).\n",
                     ipv6[0]*256 + ipv6[1],
                     ipv6[2]*256 + ipv6[3],
                     ipv6[4]*256 + ipv6[5],
                     ipv6[6]*256 + ipv6[7],
                     ipv6[8]*256 + ipv6[9],
                     ipv6[10]*256 + ipv6[11],
                     ipv6[12]*256 + ipv6[13],
                     ipv6[14]*256 + ipv6[15]);

    if (memcmp(m_primary_dns_server_ipv6.ipv6, zero, sizeof m_primary_dns_server_ipv6.ipv6)
        == 0) {
        ret = -1;
    }
    else {
        memcpy(o_ipv6->ipv6,
               m_primary_dns_server_ipv6.ipv6,
               sizeof m_primary_dns_server_ipv6.ipv6);
        ret = 0;
    }
    pubnub_mutex_unlock(m_lock);
    return ret;
}


int pubnub_get_dns_secondary_server_ipv6(struct pubnub_ipv6_address* o_ipv6)
{
    int            ret;
    const uint8_t* ipv6 = m_secondary_dns_server_ipv6.ipv6;
    uint8_t        zero[sizeof m_secondary_dns_server_ipv6.ipv6] = {0};

    PUBNUB_ASSERT_OPT(o_ipv6 != NULL);

    pubnub_mutex_init_static(m_lock);
    pubnub_mutex_lock(m_lock);

    PUBNUB_LOG_TRACE("pubnub_dns_get_secondary_server_ipv6(%u:%u:%u:%u:%u:%u:%u:%u).\n",
                     ipv6[0]*256 + ipv6[1],
                     ipv6[2]*256 + ipv6[3],
                     ipv6[4]*256 + ipv6[5],
                     ipv6[6]*256 + ipv6[7],
                     ipv6[8]*256 + ipv6[9],
                     ipv6[10]*256 + ipv6[11],
                     ipv6[12]*256 + ipv6[13],
                     ipv6[14]*256 + ipv6[15]);

    if (memcmp(m_secondary_dns_server_ipv6.ipv6, zero, sizeof m_secondary_dns_server_ipv6.ipv6)
        == 0) {
        ret = -1;
    }
    else {
        memcpy(o_ipv6->ipv6,
               m_secondary_dns_server_ipv6.ipv6,
               sizeof m_secondary_dns_server_ipv6.ipv6);
        ret = 0;
    }
    pubnub_mutex_unlock(m_lock);
    return ret;
}
#endif /* PUBNUB_USE_IPV6 */
