#include "pubnub_internal.h"
#include "core/pubnub_dns_servers.h"
#if !defined(PUBNUB_SET_DNS_SERVERS)
#error Must define PUBNUB_SET_DNS_SERVERS before compiling this file
#endif
#include "lib/pubnub_parse_ipv4_addr.h"

#include "core/pubnub_assert.h"
#include "core/pubnub_log.h"

#include <string.h>
#include <sys/types.h>


static struct pubnub_ipv4_address m_primary_dns_server pubnub_guarded_by(m_lock);
static struct pubnub_ipv4_address m_secondary_dns_server pubnub_guarded_by(m_lock);
pubnub_mutex_static_decl_and_init(m_lock);


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
    memcpy(m_primary_dns_server.ipv4,
           ipv4_address.ipv4,
           sizeof m_primary_dns_server.ipv4);
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
    ret = pubnub_parse_ipv4_addr(ipv4_str, &m_primary_dns_server);
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
    memcpy(m_secondary_dns_server.ipv4,
           ipv4_address.ipv4,
           sizeof m_secondary_dns_server.ipv4);
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
    ret = pubnub_parse_ipv4_addr(ipv4_str, &m_secondary_dns_server);
    pubnub_mutex_unlock(m_lock);

    return ret;
}


int pubnub_get_dns_primary_server_ipv4(struct pubnub_ipv4_address* o_ipv4)
{
    int            ret;
    const uint8_t* ipv4 = m_primary_dns_server.ipv4;
    uint8_t        zero[sizeof m_primary_dns_server.ipv4];

    PUBNUB_ASSERT_OPT(o_ipv4 != NULL);

    pubnub_mutex_init_static(m_lock);
    pubnub_mutex_lock(m_lock);

    PUBNUB_LOG_TRACE("pubnub_dns_get_primary_server_ipv4(%u.%u.%u.%u).\n",
                     ipv4[0],
                     ipv4[1],
                     ipv4[2],
                     ipv4[3]);

    memset(zero, 0, sizeof zero);
    if (memcmp(m_primary_dns_server.ipv4, zero, sizeof m_primary_dns_server.ipv4)
        == 0) {
        ret = -1;
    }
    else {
        memcpy(o_ipv4->ipv4,
               m_primary_dns_server.ipv4,
               sizeof m_primary_dns_server.ipv4);
        ret = 0;
    }
    pubnub_mutex_unlock(m_lock);
    return ret;
}


int pubnub_get_dns_secondary_server_ipv4(struct pubnub_ipv4_address* o_ipv4)
{
    int            ret;
    const uint8_t* ipv4 = m_secondary_dns_server.ipv4;
    uint8_t        zero[sizeof m_secondary_dns_server.ipv4];

    PUBNUB_ASSERT_OPT(o_ipv4 != NULL);

    pubnub_mutex_init_static(m_lock);
    pubnub_mutex_lock(m_lock);

    PUBNUB_LOG_TRACE("pubnub_dns_get_secondary_server_ipv4(%u.%u.%u.%u).\n",
                     ipv4[0],
                     ipv4[1],
                     ipv4[2],
                     ipv4[3]);

    memset(zero, 0, sizeof zero);
    if (memcmp(m_secondary_dns_server.ipv4, zero, sizeof m_secondary_dns_server.ipv4)
        == 0) {
        ret = -1;
    }
    else {
        memcpy(o_ipv4->ipv4,
               m_secondary_dns_server.ipv4,
               sizeof m_secondary_dns_server.ipv4);
        ret = 0;
    }
    pubnub_mutex_unlock(m_lock);
    return ret;
}
