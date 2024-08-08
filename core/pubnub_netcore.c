/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"

#include "lib/pb_strncasecmp.h"

#include "core/pubnub_assert.h"
#include "core/pubnub_log.h"
#include "core/pubnub_ccore.h"
#include "core/pubnub_ccore_pubsub.h"
#include "core/pbpal.h"
#include "core/pubnub_version.h"
#include "core/pubnub_version_internal.h"
#include "core/pubnub_helper.h"
#if PUBNUB_USE_SUBSCRIBE_V2
#include "core/pbcc_subscribe_v2.h"
#endif
#if PUBNUB_USE_ADVANCED_HISTORY
#include "core/pbcc_advanced_history.h"
#endif
#if PUBNUB_USE_FETCH_HISTORY
#include "core/pbcc_fetch_history.h"
#endif
#if PUBNUB_USE_OBJECTS_API
#include "core/pbcc_objects_api.h"
#endif
#if PUBNUB_USE_ACTIONS_API
#include "core/pbcc_actions_api.h"
#endif
#if PUBNUB_USE_GRANT_TOKEN_API
#include "core/pbcc_grant_token_api.h"
#endif
#if PUBNUB_USE_REVOKE_TOKEN_API
#include "core/pbcc_revoke_token_api.h"
#endif
#include "core/pubnub_proxy_core.h"

#include <string.h>


#define WATCH_ENUM_RESOLV_N_CONNECT(X)                                        \
    do {                                                                      \
       enum pbpal_resolv_n_connect_result x_ = (X);                           \
       PUBNUB_LOG(PUBNUB_LOG_LEVEL_DEBUG,                                     \
                  __FILE__ "(%d) in %s: `" #X "` = %d (%s)\n",                \
                  __LINE__,                                                   \
                  __FUNCTION__,                                               \
                  x_,                                                         \
                  pbpal_resolv_n_connect_res_2_string(x_));                   \
    } while (0)

/** Each HTTP chunk has a trailining CRLF ("\r\n" in C-speak).  That's
    a little strange, but is in the "spirit" of HTTP.

    We need to read it from the network interface, but, we discard it.
 */
#define CHUNK_TRAIL_LENGTH 2


#if PUBNUB_RECEIVE_GZIP_RESPONSE
/* 'Accept-Encoding' header line */
#define ACCEPT_ENCODING "Accept-Encoding: gzip\r\n"
#define possible_gzip_response(pb)                                             \
    if ((pb)->data_compressed == compressionGZIP) {                            \
        pbres                 = pbgzip_decompress(pb);                         \
        (pb)->data_compressed = compressionNONE;                               \
        if (PNR_OK != pbres) {                                                 \
            outcome_detected((pb), pbres);                                     \
            return pbres;                                                      \
        }                                                                      \
    }
#else
#define ACCEPT_ENCODING ""
#define possible_gzip_response(pb)
#endif /* PUBNUB_RECEIVE_GZIP_RESPONSE */

bool HTTP_request_has_body(uint8_t method)
{
    switch(method) {
    case pubnubSendViaGET:
    case pubnubUseDELETE:
        return false;
    case pubnubSendViaPOST:
    case pubnubUsePATCH:
#if PUBNUB_USE_GZIP_COMPRESSION
    case pubnubSendViaPOSTwithGZIP:
    case pubnubUsePATCHwithGZIP:
#endif
        return true;
    default:
        PUBNUB_LOG_ERROR("Error: HTTP_message_has_body(method): unhandled method: %u\n",
                         method);
        return false;
    }
}


static char const* get_method_verb_string(uint8_t method)
{
    switch(method) {
    case pubnubSendViaGET:
        return "GET ";
    case pubnubSendViaPOST:
#if PUBNUB_USE_GZIP_COMPRESSION
    case pubnubSendViaPOSTwithGZIP:
#endif
        return "POST ";
    case pubnubUsePATCH:
#if PUBNUB_USE_GZIP_COMPRESSION
    case pubnubUsePATCHwithGZIP:
#endif
        return "PATCH ";
    case pubnubUseDELETE:
        return "DELETE ";
    default:
        PUBNUB_LOG_ERROR("Error: get_method_verb_string(method): unhandled method: %u\n",
                         method);
        return "UNKOWN ";
    }
}

static int send_fin_head(struct pubnub_* pb)
{
    char s[200];
    snprintf(s,
             sizeof s,
             "\r\nUser-Agent: %s%s",
             pubnub_uagent(),
             "\r\n" ACCEPT_ENCODING "\r\n");
    return pbpal_send_str(pb, s);
}


#define SEND_FIN_HEAD(pb)                                                      \
    if (0 > send_fin_head(pb)) {                                               \
        outcome_detected(pb, PNR_IO_ERROR);                                    \
        break;                                                                 \
    }


static bool should_keep_alive(struct pubnub_* pb, enum pubnub_res rslt)
{
    PUBNUB_LOG_DEBUG("should_keep_alive(pb=%p, rslt=%d('%s')) pb->flags.should_close = %d\n",
                     pb,
                     rslt,
                     pubnub_res_2_string(rslt),
                     (int)pb->flags.should_close);
    if (!pb->flags.should_close) {
#if PUBNUB_ADVANCED_KEEP_ALIVE
        time_t tt = time(NULL);
        PUBNUB_LOG_DEBUG("should_keep_alive(pb=%p): pb->keep_alive.count = %d, "
                         "pb->keep_alive.max = %d, "
                         "pb->keep_alive.t_connect = %ld, "
                         "pb->keep_alive.timeout = %ld, "
                         "time = %ld\n",
                         pb,
                         pb->keep_alive.count,
                         pb->keep_alive.max,
                         (long)pb->keep_alive.t_connect,
                         (long)pb->keep_alive.timeout,
                         (long)tt);
        if ((++pb->keep_alive.count >= pb->keep_alive.max)
            || ((tt - pb->keep_alive.t_connect) > pb->keep_alive.timeout)) {
            return false;
        }
#endif
        switch (rslt) {
        case PNR_ADDR_RESOLUTION_FAILED:
        case PNR_WAIT_CONNECT_TIMEOUT:
        case PNR_CONNECT_FAILED:
        case PNR_CONNECTION_TIMEOUT:
        case PNR_TIMEOUT:
        case PNR_ABORTED:
        case PNR_IO_ERROR:
        case PNR_CANCELLED:
        case PNR_STARTED:
        case PNR_INTERNAL_ERROR:
        case PNR_AUTHENTICATION_FAILED:
            return false;
        default:
            return true;
        }
    }
    return false;
}


static void close_connection(struct pubnub_* pb)
{
    if (pbpal_close(pb) <= 0) {
#if PUBNUB_NEED_RETRY_AFTER_CLOSE
        PUBNUB_LOG_TRACE("close_connection(): pb->flags.retry_after_close=%d\n",
                         pb->flags.retry_after_close);
        if (pb->flags.retry_after_close) {
            pb->state = PBS_RETRY;
            return;
        }
#endif
        pbpal_forget(pb);
        pbntf_trans_outcome(pb, PBS_IDLE);
    }
    else {
        pb->state = PBS_WAIT_CLOSE;
    }
}


static enum pubnub_state close_kept_alive_connection(struct pubnub_* pb)
{
    pb->flags.started_while_kept_alive = false;
    if (pbpal_close(pb) <= 0) {
#if PUBNUB_NEED_RETRY_AFTER_CLOSE
        PUBNUB_LOG_TRACE(
            "close_kept_alive_connection(): pb->flags.retry_after_close=%d\n",
            pb->flags.retry_after_close);
        if (pb->flags.retry_after_close) {
            return PBS_RETRY;
        }
#endif
        pbpal_forget(pb);
        return PBS_IDLE;
    }
    else {
        return PBS_KEEP_ALIVE_WAIT_CLOSE;
    }
}


static void outcome_detected(struct pubnub_* pb, enum pubnub_res rslt)
{
    pb->core.last_result = rslt;
    if (should_keep_alive(pb, rslt)) {
        /* We don't monitor the connection while in "keep-alive idle".
           This is easy on the CPU and we don't really care much, as
           we don't have a way to report to the user that the
           connection was lost.
         */
        PUBNUB_LOG_TRACE("outcome_detected(pb=%p): Keepin' it alive\n", pb);
        pbntf_lost_socket(pb);
        pbntf_trans_outcome(pb, PBS_KEEP_ALIVE_IDLE);
#if PUBNUB_NEED_RETRY_AFTER_CLOSE
        pb->flags.retry_after_close = false;
#endif
    }
    else {
        pb->flags.started_while_kept_alive = false;
        close_connection(pb);
    }
}


static enum pubnub_res dont_parse(struct pbcc_context* p)
{
    PUBNUB_UNUSED(p);
    PUBNUB_LOG_ERROR("Parsing not initialized for a type of transaction\n");
    return PNR_INTERNAL_ERROR;
}


static PFpbcc_parse_response_T m_aParseResponse[] = { dont_parse,
                                                      pbcc_parse_subscribe_response,
                                                      pbcc_parse_publish_response,
                                                      pbcc_parse_publish_response, /* PBTT_SIGNAL */
#if PUBNUB_ONLY_PUBSUB_API
                                                      dont_parse,
                                                      dont_parse,
                                                      dont_parse,
                                                      dont_parse,
                                                      dont_parse,
                                                      dont_parse,
                                                      dont_parse,
                                                      dont_parse,
                                                      dont_parse,
                                                      dont_parse,
                                                      dont_parse,
                                                      dont_parse,
                                                      dont_parse,
                                                      dont_parse,
                                                      dont_parse
#else
    pbcc_parse_presence_response, /* PBTT_LEAVE */
    pbcc_parse_time_response,
    pbcc_parse_history_response,
    pbcc_parse_presence_response, /* PBTT_HERENOW */
    pbcc_parse_presence_response, /* PBTT_GLOBAL_HERENOW */
    pbcc_parse_presence_response, /* PBTT_WHERENOW */
    pbcc_parse_presence_response, /* PBTT_SET_STATE */
    pbcc_parse_presence_response, /* PBTT_STATE_GET */
    pbcc_parse_channel_registry_response, /* PBTT_REMOVE_CHANNEL_GROUP */
    pbcc_parse_channel_registry_response, /* PBTT_REMOVE_CHANNEL_FROM_GROUP */
    pbcc_parse_channel_registry_response, /* PBTT_ADD_CHANNEL_TO_GROUP */
    pbcc_parse_channel_registry_response, /* PBTT_LIST_CHANNEL_GROUP */
    pbcc_parse_presence_response /* PBTT_HEARTBEAT */
#if PUBNUB_USE_SUBSCRIBE_V2
    , pbcc_parse_subscribe_v2_response /* PBTT_SUBSCRIBE_V2 */
#endif
#if PUBNUB_USE_ADVANCED_HISTORY
    , pbcc_parse_message_counts_response /* PBTT_MESSAGE_COUNTS */
    , pbcc_parse_delete_messages_response /* PBTT_DELETE_MESSAGES */
#endif
#if PUBNUB_USE_OBJECTS_API
    , pbcc_parse_objects_api_response /* PBTT_GET_USERS */
    , pbcc_parse_objects_api_response /* PBTT_CREATE_USER */
    , pbcc_parse_objects_api_response /* PBTT_GET_USER */
    , pbcc_parse_objects_api_response /* PBTT_DELETE_USER */
    , pbcc_parse_objects_api_response /* PBTT_GET_SPACES */
    , pbcc_parse_objects_api_response /* PBTT_CREATE_SPACE */
    , pbcc_parse_objects_api_response /* PBTT_GET_SPACE */
    , pbcc_parse_objects_api_response /* PBTT_DELETE_SPACE */
    , pbcc_parse_objects_api_response /* PBTT_GET_MEMBERSHIPS */
    , pbcc_parse_objects_api_response /* PBTT_UPDATE_MEMBERSHIPS */
    , pbcc_parse_objects_api_response /* PBTT_LEAVE_SPACES */
    , pbcc_parse_objects_api_response /* PBTT_GET_MEMBERS */
    , pbcc_parse_objects_api_response /* PBTT_ADD_MEMBERS */
    , pbcc_parse_objects_api_response /* PBTT_UPDATE_MEMBERS */
    , pbcc_parse_objects_api_response /* PBTT_REMOVE_MEMBERS */
#endif /* PUBNUB_USE_OBJECTS_API */
#if PUBNUB_USE_ACTIONS_API
    , pbcc_parse_actions_api_response /* PBTT_ADD_ACTION */
    , pbcc_parse_actions_api_response /* PBTT_REMOVE_ACTION */
    , pbcc_parse_actions_api_response /* PBTT_GET_ACTIONS */
    , pbcc_parse_history_with_actions_response /* PBTT_HISTORY_WITH_ACTIONS */
#endif /* PUBNUB_USE_OBJECTS_API */
#if PUBNUB_USE_GRANT_TOKEN_API
    , pbcc_parse_grant_token_api_response /* PBTT_GRANT_TOKEN */
#endif /* PUBNUB_USE_GRANT_TOKEN_API */
#if PUBNUB_USE_FETCH_HISTORY
    , pbcc_parse_fetch_history_response /* PBTT_FETCH_HISTORY */
#endif
#if PUBNUB_USE_REVOKE_TOKEN_API
    , pbcc_parse_revoke_token_response /* PBTT_REVOKE_TOKEN */
#endif /* PUBNUB_USE_REVOKE_TOKEN_API */
#endif /* PUBNUB_ONLY_PUBSUB_API */
};


PUBNUB_STATIC_ASSERT((sizeof m_aParseResponse / sizeof m_aParseResponse[0]) == PBTT_MAX,
                     bad_response_table_length);


static enum pubnub_res parse_pubnub_result(struct pubnub_* pb)
{
    enum pubnub_res pbres = m_aParseResponse[pb->trans](&pb->core);
    if (pbres != PNR_OK) {
        PUBNUB_LOG_WARNING("pb=%p parsing response for transaction type #%d "
                           "returned error %d\nResponse was: %s\n",
                           pb,
                           pb->trans,
                           pbres,
                           pb->core.http_reply);
    }
    else {
        pbauto_heartbeat_start_timer(pb);
    }
    
    return pbres;
}


static enum pubnub_res finish(struct pubnub_* pb)
{
    enum pubnub_res pbres;

#if PUBNUB_PROXY_API
    switch (pbproxy_handle_finish(pb)) {
    case pbproxyFinError:
        PUBNUB_LOG_TRACE("Proxy: Error, close connection\n");
        pb->flags.should_close = true;
        outcome_detected(pb, PNR_HTTP_ERROR);
        return PNR_HTTP_ERROR;
    case pbproxyFinRetry:
        PUBNUB_LOG_TRACE("Proxy: retry in current connection\n");
        pb->flags.retry_after_close = true;
        if (pb->flags.should_close) {
            close_connection(pb);
            return PNR_OK;
        }
        pb->state = PBS_CONNECTED;
        return PNR_OK;
    default:
        break;
    }
#endif
    /* Ensures existence of the reply buffer in case no:
       'Content-Length:', nor 'Transfer-Encoding: chunked'
       header line has been received
    */
    if (!pbcc_ensure_reply_buffer(&pb->core)) {
        outcome_detected(pb, PNR_REPLY_TOO_BIG);
        return PNR_REPLY_TOO_BIG;
    }
    possible_gzip_response(pb);
    pb->core.http_reply[pb->core.http_buf_len] = '\0';
    PUBNUB_LOG_TRACE("finish(pb=%p, '%s')\n", pb, pb->core.http_reply);
    pbres = parse_pubnub_result(pb);
    if ((PNR_OK == pbres) && ((pb->http_code / 100) != 2)) {
        pbres = PNR_HTTP_ERROR;
    }

    outcome_detected(pb, pbres);
    return pbres;
}


static char const* pbnc_state2str(enum pubnub_state e)
{
    switch (e) {
    case PBS_NULL:
        return "PBS_NULL";
#if PUBNUB_NEED_RETRY_AFTER_CLOSE
    case PBS_RETRY:
        return "PBS_RETRY";
#endif
    case PBS_IDLE:
        return "PBS_IDLE";
    case PBS_READY:
        return "PBS_READY";
    case PBS_WAIT_DNS_SEND:
        return "PBS_WAIT_DNS_SEND";
    case PBS_WAIT_DNS_RCV:
        return "PBS_WAIT_DNS_RCV";
    case PBS_WAIT_CONNECT:
        return "PBS_WAIT_CONNECT";
    case PBS_CONNECTED:
        return "PBS_CONNECTED";
#if PUBNUB_USE_SSL
    case PBS_WAIT_TLS_CONNECT:
        return "PBS_WAIT_TLS_CONNECT";
#endif
    case PBS_TX_GET:
        return "PBS_TX_GET";
    case PBS_TX_PATH:
        return "PBS_TX_PATH";
    case PBS_TX_SCHEME:
        return "PBS_TX_SCHEME";
    case PBS_TX_HOST:
        return "PBS_TX_HOST";
    case PBS_TX_PORT_NUM:
        return "PBS_TX_PORT_NUM";
    case PBS_TX_VER:
        return "PBS_TX_VER";
    case PBS_TX_EXTRA_HEADERS:
        return "PBS_TX_EXTRA_HEADERS";
    case PBS_TX_ORIGIN:
        return "PBS_TX_ORIGIN";
    case PBS_TX_FIN_HEAD:
        return "PBS_TX_FIN_HEAD";
    case PBS_TX_BODY:
        return "PBS_TX_BODY";
    case PBS_RX_HTTP_VER:
        return "PBS_RX_HTTP_VER";
    case PBS_RX_HEADERS:
        return "PBS_RX_HEADERS";
    case PBS_RX_HEADER_LINE:
        return "PBS_RX_HEADER_LINE";
    case PBS_RX_BODY:
        return "PBS_RX_BODY";
    case PBS_RX_BODY_WAIT:
        return "PBS_RX_BODY_WAIT";
    case PBS_RX_CHUNK_LEN:
        return "PBS_RX_CHUNK_LEN";
    case PBS_RX_CHUNK_LEN_LINE:
        return "PBS_RX_CHUNK_LEN_LINE";
    case PBS_RX_BODY_CHUNK:
        return "PBS_RX_BODY_CHUNK";
    case PBS_RX_BODY_CHUNK_WAIT:
        return "PBS_RX_BODY_CHUNK_WAIT";
    case PBS_WAIT_CLOSE:
        return "PBS_WAIT_CLOSE";
    case PBS_WAIT_CANCEL:
        return "PBS_WAIT_CANCEL";
    case PBS_WAIT_CANCEL_CLOSE:
        return "PBS_WAIT_CANCEL_CLOSE";
    case PBS_KEEP_ALIVE_IDLE:
        return "PBS_KEEP_ALIVE_IDLE";
    case PBS_KEEP_ALIVE_READY:
        return "PBS_KEEP_ALIVE_READY";
    case PBS_KEEP_ALIVE_WAIT_CLOSE:
        return "PBS_KEEP_ALIVE_WAIT_CLOSE";
    case PBS_WAIT_CANCEL_DNS:
        return "PBS_WAIT_CANCEL_DNS";
    case PBS_WAIT_CANCEL_KEEPALIVE:
        return "PBS_WAIT_CANCEL_KEEPALIVE";
    default:
        return "Unknown enum pubnub_state";
    }
}

static void initialize_fields_in_state_IDLE(struct pubnub_* pb)
{
#if PUBNUB_CHANGE_DNS_SERVERS
    pb->dns_check.dns_server_check = 0;
#endif
#if PUBNUB_NEED_RETRY_AFTER_CLOSE
    pb->flags.retry_after_close = false;
#else
    PUBNUB_UNUSED(pb);
#endif
#if PUBNUB_PROXY_API
    pb->proxy_tunnel_established = false;
    pb->proxy_saved_path_len     = 0;
    pb->proxy_authorization_sent = false;
    pb->auth_msg_count           = 0;
#endif
#if PUBNUB_USE_SSL
    pb->flags.trySSL = pb->options.useSSL;
#endif
}

int pbnc_fsm(struct pubnub_* pb)
{
    enum pubnub_res pbrslt;
    int             i;

    PUBNUB_LOG_TRACE("pbnc_fsm(pb=%p)\t", pb);

next_state:
    PUBNUB_LOG_TRACE("pb->state = %d (%s)\n", pb->state, pbnc_state2str(pb->state));
    switch (pb->state) {
    case PBS_NULL:
        break;
    case PBS_IDLE:
        initialize_fields_in_state_IDLE(pb);
        pb->state = PBS_READY;
        switch (pbntf_enqueue_for_processing(pb)) {
        case -1:
            pb->core.last_result = PNR_INTERNAL_ERROR;
            pbntf_trans_outcome(pb, PBS_IDLE);
            return 0;
        case 0:
            goto next_state;
        case +1:
            break;
        }
        break;
#if PUBNUB_NEED_RETRY_AFTER_CLOSE
    case PBS_RETRY:
        pb->flags.retry_after_close = false;
        pb->state                   = PBS_READY;
        goto next_state;
#endif
    case PBS_READY: {
        enum pbpal_resolv_n_connect_result rslv = pbpal_resolv_and_connect(pb);
        WATCH_ENUM_RESOLV_N_CONNECT(rslv);
        switch (rslv) {
        case pbpal_resolv_send_wouldblock:
            i = pbntf_got_socket(pb);
            if (i >= 0) {
                pbntf_start_wait_connect_timer(pb);
            }
            pb->state = PBS_WAIT_DNS_SEND;
            break;
        case pbpal_resolv_sent:
        case pbpal_resolv_rcv_wouldblock:
            i = pbntf_got_socket(pb);
            if (i >= 0) {
                pbntf_start_wait_connect_timer(pb);
            }
            pb->state = PBS_WAIT_DNS_RCV;
            pbntf_watch_in_events(pb);
            break;
        case pbpal_connect_wouldblock:
            i = pbntf_got_socket(pb);
            if (i >= 0) {
                pbntf_start_wait_connect_timer(pb);
            }
            pb->state = PBS_WAIT_CONNECT;
            break;
        case pbpal_connect_success:
            i = pbntf_got_socket(pb);
            pb->state = PBS_CONNECTED;
            break;
        default:
            pbpal_report_error_from_environment(pb, __FILE__, __LINE__);
            pb->core.last_result = PNR_ADDR_RESOLUTION_FAILED;
#if PUBNUB_ADNS_RETRY_AFTER_CLOSE
            if (pb->flags.retry_after_close) {
                close_connection(pb);
                goto next_state;
            }
#endif
            pbntf_trans_outcome(pb, PBS_IDLE);
            return 0;
        }
        if (0 == i) {
            goto next_state;
        }
        else if (i < 0) {
            pb->core.last_result = PNR_CONNECT_FAILED;
#if PUBNUB_ADNS_RETRY_AFTER_CLOSE
            if (pb->flags.retry_after_close) {
                close_connection(pb);
                goto next_state;
            }
#endif
            pbntf_trans_outcome(pb, PBS_IDLE);
        }
        break;
    }
    case PBS_WAIT_DNS_SEND: {
        enum pbpal_resolv_n_connect_result rslv = pbpal_resolv_and_connect(pb);
        WATCH_ENUM_RESOLV_N_CONNECT(rslv);
        switch (rslv) {
        case pbpal_resolv_send_wouldblock:
            break;
        case pbpal_resolv_sent:
        case pbpal_resolv_rcv_wouldblock:
            pb->state = PBS_WAIT_DNS_RCV;
            pbntf_watch_in_events(pb);
            break;
        case pbpal_connect_wouldblock:
            pbntf_start_wait_connect_timer(pb);
            pbntf_update_socket(pb);
            pb->state = PBS_WAIT_CONNECT;
            break;
        case pbpal_connect_success:
            pbntf_start_transaction_timer(pb);
            pbntf_update_socket(pb);
            pb->state = PBS_CONNECTED;
            goto next_state;
        default:
            pbpal_report_error_from_environment(pb, __FILE__, __LINE__);
            pbntf_update_socket(pb);
            outcome_detected(pb, PNR_ADDR_RESOLUTION_FAILED);
            break;
        }
        break;
    }
    case PBS_WAIT_DNS_RCV: {
        enum pbpal_resolv_n_connect_result rslv =
            pbpal_check_resolv_and_connect(pb);
        WATCH_ENUM_RESOLV_N_CONNECT(rslv);
        switch (rslv) {
        case pbpal_resolv_send_wouldblock:
        case pbpal_resolv_sent:
            outcome_detected(pb, PNR_INTERNAL_ERROR);
            break;
        case pbpal_resolv_rcv_wouldblock:
            break;
        case pbpal_connect_wouldblock:
            pbntf_start_wait_connect_timer(pb);
            pbntf_update_socket(pb);
            pb->state = PBS_WAIT_CONNECT;
            pbntf_watch_out_events(pb);
            break;
        case pbpal_connect_success:
            pbntf_start_transaction_timer(pb);
            pbntf_update_socket(pb);
            pb->state = PBS_CONNECTED;
            pbntf_watch_out_events(pb);
            goto next_state;
        default:
            pbpal_report_error_from_environment(pb, __FILE__, __LINE__);
            pbntf_update_socket(pb);
            outcome_detected(pb, PNR_ADDR_RESOLUTION_FAILED);
            break;
        }
        break;
    }
    case PBS_WAIT_CONNECT: {
        enum pbpal_resolv_n_connect_result rslv = pbpal_check_connect(pb);
        WATCH_ENUM_RESOLV_N_CONNECT(rslv);
        switch (rslv) {
        case pbpal_resolv_send_wouldblock:
        case pbpal_resolv_sent:
        case pbpal_resolv_rcv_wouldblock:
            outcome_detected(pb, PNR_INTERNAL_ERROR);
            break;
        case pbpal_connect_wouldblock:
            break;
        case pbpal_connect_success:
            pbntf_start_transaction_timer(pb);
            pb->state = PBS_CONNECTED;
            goto next_state;
        default:
            outcome_detected(pb, PNR_CONNECT_FAILED);
            break;
        }
        break;
    }
    case PBS_CONNECTED:
        pb->flags.should_close = !pb->options.use_http_keep_alive;
#if PUBNUB_NEED_RETRY_AFTER_CLOSE
        pb->flags.retry_after_close = false;
#endif
#if PUBNUB_ADVANCED_KEEP_ALIVE
        pb->keep_alive.t_connect = time(NULL);
        pb->keep_alive.count     = 0;
#endif
#if PUBNUB_PROXY_API
        if ((pbproxyHTTP_CONNECT == pb->proxy_type)
            && (!pb->proxy_tunnel_established)) {
            pb->state = PBS_TX_GET;
            i         = pbpal_send_literal_str(pb, "CONNECT ");
            if (i < 0) {
                outcome_detected(pb, PNR_IO_ERROR);
                break;
            }
            pb->state = PBS_TX_GET;
            goto next_state;
        }
#endif
#if PUBNUB_USE_SSL
        if ((NULL == pb->pal.ssl) && pb->flags.trySSL
#if PUBNUB_PROXY_API
            && (pbproxyHTTP_GET != pb->proxy_type)
#endif
        ) {
            enum pbpal_tls_result res;
            PUBNUB_ASSERT(pb->options.useSSL);
            res = pbpal_start_tls(pb);
            switch (res) {
            case pbtlsEstablished:
                break;
            case pbtlsStarted:
                pb->state = PBS_WAIT_TLS_CONNECT;
                return 0;
            case pbtlsFailed:
                if (pb->options.fallbackSSL) {
                    pb->flags.trySSL            = false;
                    pb->flags.retry_after_close = true;
                }
#if PUBNUB_USE_MULTIPLE_ADDRESSES
                else {
                    pb->flags.retry_after_close =
                        (++pb->spare_addresses.ipv4_index < pb->spare_addresses.n_ipv4)
#if PUBNUB_USE_IPV6
                        || (++pb->spare_addresses.ipv6_index
                            < pb->spare_addresses.n_ipv6)
#endif
                        ;
                }
#endif /* PUBNUB_USE_MULTIPLE_ADDRESSES */
                outcome_detected(pb, PNR_CONNECT_FAILED);
                return 0;
            default:
                PUBNUB_LOG_ERROR("Unexpected result: pbpal_start_tls()=%d\n", res);
                outcome_detected(pb, PNR_INTERNAL_ERROR);
                return 0;
            }
        }
#endif /* PUBNUB_USE_SSL */
        i = pbpal_send_str(pb, get_method_verb_string(pb->method));
        if (i < 0) {
            outcome_detected(pb, PNR_IO_ERROR);
            break;
        }
        pb->state = PBS_TX_GET;
        goto next_state;
#if PUBNUB_USE_SSL
    case PBS_WAIT_TLS_CONNECT: {
        enum pbpal_tls_result res = pbpal_check_tls(pb);
        switch (res) {
        case pbtlsEstablished:
            i = pbpal_send_str(pb, get_method_verb_string(pb->method));
            if (i < 0) {
                outcome_detected(pb, PNR_IO_ERROR);
                break;
            }
            pb->state = PBS_TX_GET;
            goto next_state;
        case pbtlsStarted:
            break;
        case pbtlsFailed:
            if (pb->options.fallbackSSL) {
                pb->flags.trySSL            = false;
                pb->flags.retry_after_close = true;
            }
#if PUBNUB_USE_MULTIPLE_ADDRESSES
            else {
                pb->flags.retry_after_close =
                    (++pb->spare_addresses.ipv4_index < pb->spare_addresses.n_ipv4)
#if PUBNUB_USE_IPV6
                    || (++pb->spare_addresses.ipv6_index < pb->spare_addresses.n_ipv6)
#endif
                    ;
            }
#endif /* PUBNUB_USE_MULTIPLE_ADDRESSES */
            outcome_detected(pb, PNR_CONNECT_FAILED);
            break;
        default:
            PUBNUB_LOG_ERROR("Unexpected result: pbpal_check_tls()=%d\n", res);
            outcome_detected(pb, PNR_INTERNAL_ERROR);
            break;
        }
        break;
    }
#endif /* PUBNUB_USE_SSL */
    case PBS_TX_GET:
        i = pbpal_send_status(pb);
        if (i <= 0) {
#if PUBNUB_PROXY_API
            switch (pb->proxy_type) {
            case pbproxyHTTP_GET: {
                char const* http = "http://";
                pb->state        = PBS_TX_SCHEME;
                if (i < 0) {
                    outcome_detected(pb, PNR_IO_ERROR);
                    break;
                }
                PUBNUB_ASSERT_OPT(pb->core.http_buf_len < PUBNUB_BUF_MAXLEN);
                if (0 == pb->proxy_saved_path_len) {
                    memcpy(pb->proxy_saved_path,
                           pb->core.http_buf,
                           pb->core.http_buf_len + 1);
                    pb->proxy_saved_path_len = pb->core.http_buf_len;
                }
                else {
                    PUBNUB_ASSERT_OPT(pb->proxy_saved_path_len < PUBNUB_BUF_MAXLEN);
                    memmove(pb->core.http_buf,
                            pb->proxy_saved_path,
                            pb->proxy_saved_path_len + 1);
                    pb->core.http_buf_len = pb->proxy_saved_path_len;
                }
#if PUBNUB_USE_SSL
                if (pb->flags.trySSL) {
                    PUBNUB_ASSERT(pb->options.useSSL);
                    http = "https://";
                }
#endif
                if (0 > pbpal_send_str(pb, http)) {
                    outcome_detected(pb, PNR_IO_ERROR);
                }
                break;
            }
            case pbproxyHTTP_CONNECT:
                pb->state = PBS_TX_SCHEME;
                if (i < 0) {
                    outcome_detected(pb, PNR_IO_ERROR);
                    break;
                }
                if (!pb->proxy_tunnel_established) {
                    PUBNUB_ASSERT_OPT(pb->core.http_buf_len < PUBNUB_BUF_MAXLEN);
                    if (0 == pb->proxy_saved_path_len) {
                        memcpy(pb->proxy_saved_path,
                               pb->core.http_buf,
                               pb->core.http_buf_len + 1);
                        pb->proxy_saved_path_len = pb->core.http_buf_len;
                    }
                }
                else if (pb->proxy_saved_path_len > 0) {
                    PUBNUB_ASSERT_OPT(pb->proxy_saved_path_len < PUBNUB_BUF_MAXLEN);
                    memmove(pb->core.http_buf,
                            pb->proxy_saved_path,
                            pb->proxy_saved_path_len + 1);
                    pb->core.http_buf_len    = pb->proxy_saved_path_len;
                    pb->proxy_saved_path_len = 0;
                }
                break;
            case pbproxyNONE:
                pb->state = PBS_TX_PATH;
                if ((i < 0) || (-1 == pbpal_send_str(pb, pb->core.http_buf))) {
                    outcome_detected(pb, PNR_IO_ERROR);
                }
                break;
            default:
                outcome_detected(pb, PNR_INTERNAL_ERROR);
                break;
            }
#else
            pb->state = PBS_TX_PATH;
            if ((i < 0) || (-1 == pbpal_send_str(pb, pb->core.http_buf))) {
                outcome_detected(pb, PNR_IO_ERROR);
                break;
            }
#endif /* PUBNUB_PROXY_API */
            goto next_state;
        }
        break;
#if PUBNUB_PROXY_API
    case PBS_TX_SCHEME:
        i = pbpal_send_status(pb);
        if (i <= 0) {
            if ((pb->proxy_type == pbproxyHTTP_CONNECT)
                && pb->proxy_tunnel_established) {
                pb->state = PBS_TX_HOST;
            }
            else {
                char const* o = PUBNUB_ORIGIN_SETTABLE ? pb->origin : PUBNUB_ORIGIN;
                pb->state     = PBS_TX_HOST;
                if ((i < 0) || (-1 == pbpal_send_str(pb, o))) {
                    outcome_detected(pb, PNR_IO_ERROR);
                    break;
                }
            }
            goto next_state;
        }
        break;
    case PBS_TX_HOST:
        i = pbpal_send_status(pb);
        if (i < 0) {
            outcome_detected(pb, PNR_IO_ERROR);
        }
        else if (0 == i) {
            if ((pb->proxy_type == pbproxyHTTP_CONNECT)
                && !pb->proxy_tunnel_established) {
                char const* port_toward_origin = ":80";
#if PUBNUB_USE_SSL
                if (pb->flags.trySSL) {
                    PUBNUB_ASSERT(pb->options.useSSL);
                    port_toward_origin = ":443";
                }
#endif
                if (0 > pbpal_send_str(pb, port_toward_origin)) {
                    outcome_detected(pb, PNR_IO_ERROR);
                    break;
                }
                pb->state = PBS_TX_PORT_NUM;
            }
            else {
                pb->state = PBS_TX_PATH;
                if (-1 == pbpal_send_str(pb, pb->core.http_buf)) {
                    outcome_detected(pb, PNR_IO_ERROR);
                    break;
                }
            }
            goto next_state;
        }
        break;
    case PBS_TX_PORT_NUM:
#endif /* PUBNUB_PROXY_API */
    case PBS_TX_PATH:
        i = pbpal_send_status(pb);
        if (i < 0) {
            outcome_detected(pb, PNR_IO_ERROR);
        }
        else if (0 == i) {
            if (0 > pbpal_send_literal_str(pb, " HTTP/1.1\r\nHost: ")) {
                outcome_detected(pb, PNR_IO_ERROR);
                break;
            }
            pb->state = PBS_TX_VER;
            goto next_state;
        }
        break;
    case PBS_TX_VER:
        i = pbpal_send_status(pb);
        if (i <= 0) {
            char const* o = PUBNUB_ORIGIN_SETTABLE ? pb->origin : PUBNUB_ORIGIN;
            pb->state     = PBS_TX_ORIGIN;
            if ((i < 0) || (-1 == pbpal_send_str(pb, o))) {
                outcome_detected(pb, PNR_IO_ERROR);
                break;
            }
            goto next_state;
        }
        break;
    case PBS_TX_ORIGIN:
        i = pbpal_send_status(pb);
        if (i < 0) {
            outcome_detected(pb, PNR_IO_ERROR);
        }
        else if (0 == i) {
#if PUBNUB_PROXY_API
            if (!pb->proxy_tunnel_established) {
                char hedr[1024] = "\r\n";
                if (0 == pbproxy_http_header_to_send(pb, hedr + 2, sizeof hedr - 2)) {
                    PUBNUB_LOG_TRACE("Sending HTTP proxy header: '%s'\n", hedr);
                    pb->state = PBS_TX_EXTRA_HEADERS;
                    if (-1 == pbpal_send_str(pb, hedr)) {
                        outcome_detected(pb, PNR_IO_ERROR);
                        break;
                    }
                    goto next_state;
                }
            }
#endif
            if (HTTP_request_has_body(pb->method)
#if PUBNUB_PROXY_API
                && (pb->proxy_tunnel_established || (pbproxyNONE == pb->proxy_type))
#endif
            ) {
                char hedr[128] = "\r\n";
                pbcc_via_post_headers(
                    &(pb->core), hedr + 2, sizeof hedr - 2);
                PUBNUB_LOG_TRACE(
                    "Sending HTTP 'via POST, or PATCH' headers: '%s'\n", hedr);
                pb->state = PBS_TX_EXTRA_HEADERS;
                if (-1 == pbpal_send_str(pb, hedr)) {
                    outcome_detected(pb, PNR_IO_ERROR);
                    break;
                }
                goto next_state;
            }
            SEND_FIN_HEAD(pb);
            pb->state = PBS_TX_FIN_HEAD;
            goto next_state;
        }
        break;
    case PBS_TX_EXTRA_HEADERS:
        i = pbpal_send_status(pb);
        if (i < 0) {
            outcome_detected(pb, PNR_IO_ERROR);
        }
        else if (0 == i) {
            SEND_FIN_HEAD(pb);
            pb->state = PBS_TX_FIN_HEAD;
            goto next_state;
        }
        break;
    case PBS_TX_FIN_HEAD:
        i = pbpal_send_status(pb);
        if (i < 0) {
            outcome_detected(pb, PNR_IO_ERROR);
        }
        else if (0 == i) {
            if (HTTP_request_has_body(pb->method)
#if PUBNUB_PROXY_API
                && (pb->proxy_tunnel_established || (pbproxyNONE == pb->proxy_type))
#endif
            ) {
                const char* message = pb->core.message_to_send;
#if PUBNUB_USE_GZIP_COMPRESSION
                size_t len = (pb->core.gzip_msg_len != 0) ? pb->core.gzip_msg_len
                                                          : strlen(message);
#else
                size_t len = strlen(message);
#endif
                pb->state = PBS_TX_BODY;
                if (-1 == pbpal_send(pb, message, len)) {
                    outcome_detected(pb, PNR_IO_ERROR);
                    break;
                }
            }
            else {
                pbpal_start_read_line(pb);
                pb->state = PBS_RX_HTTP_VER;
                pbntf_watch_in_events(pb);
            }
            goto next_state;
        }
        break;
    case PBS_TX_BODY:
        i = pbpal_send_status(pb);
        if (i < 0) {
            outcome_detected(pb, PNR_IO_ERROR);
        }
        else if (0 == i) {
            pbpal_start_read_line(pb);
            pb->state = PBS_RX_HTTP_VER;
            pbntf_watch_in_events(pb);
            goto next_state;
        }
        break;
    case PBS_RX_HTTP_VER:
        pbauto_heartbeat_transaction_ongoing(pb);
        pbrslt = pbpal_line_read_status(pb);
        PUBNUB_LOG_TRACE("pb=%p PBS_RX_HTTP_VER: pbrslt=%d\n", pb, pbrslt);
        switch (pbrslt) {
        case PNR_IN_PROGRESS:
            break;
        case PNR_OK:
            if (strncmp(pb->core.http_buf, "HTTP/1.", 7) != 0) {
                PUBNUB_LOG_ERROR("pb=%p bad HTTP response version: %.*s\n",
                                 pb,
                                 pbpal_read_len(pb),
                                 pb->core.http_buf);
                outcome_detected(pb, PNR_IO_ERROR);
                break;
            }
            pb->http_code = atoi(pb->core.http_buf + 9);
            WATCH_USHORT(pb->http_code);
            pb->core.http_content_len = 0;
            pb->http_chunked          = false;
            pb->state                 = PBS_RX_HEADERS;
            goto next_state;
        case PNR_CONNECTION_TIMEOUT:
        case PNR_TIMEOUT:
        case PNR_IO_ERROR:
            if (pb->flags.started_while_kept_alive) {
                pb->state = close_kept_alive_connection(pb);
                goto next_state;
            }
            /*FALLTHRU*/
        default:
            PUBNUB_LOG_ERROR("pb=%p in PBS_RX_HTTP_VER: failure inducing "
                             "pbpal_line_read_status %d\n",
                             pb,
                             pbrslt);
            outcome_detected(pb, pbrslt);
            break;
        }
        break;
    case PBS_RX_HEADERS:
        pbpal_start_read_line(pb);
        pb->state = PBS_RX_HEADER_LINE;
        goto next_state;
    case PBS_RX_HEADER_LINE:
        pbrslt = pbpal_line_read_status(pb);
        PUBNUB_LOG_TRACE("pb=%p PBS_RX_HEADER_LINE: pbrslt=%d\n", pb, pbrslt);
        switch (pbrslt) {
        case PNR_IN_PROGRESS:
            break;
        case PNR_OK: {
            /* We know that Pubnub will always use keep-alive unless
               we ask for `close`, so, we don't check for the
               `Connection` header.
            */
            char h_chunked[] = "Transfer-Encoding: chunked";
            char h_length[]  = "Content-Length: ";
            char h_close[]   = "Connection: close";
#if PUBNUB_RECEIVE_GZIP_RESPONSE
            char h_encoding[] = "Content-Encoding: gzip";
#endif
            int read_len = pbpal_read_len(pb);
            PUBNUB_LOG_TRACE("pb=%p header line was read: '%.*s'\n",
                             pb,
                             read_len,
                             pb->core.http_buf);
            WATCH_INT(read_len);
            if (read_len <= 2) {
                pb->core.http_buf_len = 0;
                if (!pb->http_chunked) {
                    if (0 == pb->core.http_content_len) {
#if PUBNUB_PROXY_API
                        WATCH_ENUM(pb->proxy_type);
                        WATCH_INT(pb->proxy_tunnel_established);
                        if ((pb->proxy_type == pbproxyHTTP_CONNECT)
                            && !pb->proxy_tunnel_established) {
                            if (PNR_OK != finish(pb)) {
                                break;
                            }
                            goto next_state;
                        }
#endif
                        outcome_detected(pb, PNR_IO_ERROR);
                        break;
                    }
                    pb->state = PBS_RX_BODY;
                }
                else {
                    pb->state = PBS_RX_CHUNK_LEN;
                }
                goto next_state;
            }
            if (pb_strncasecmp(pb->core.http_buf, h_chunked, sizeof h_chunked - 1) == 0) {
                pb->http_chunked = true;
            }
            else if (pb_strncasecmp(pb->core.http_buf, h_length, sizeof h_length - 1) == 0) {
                size_t len = atoi(pb->core.http_buf + sizeof h_length - 1);
                if (0 != pbcc_realloc_reply_buffer(&pb->core, len)) {
                    outcome_detected(pb, PNR_REPLY_TOO_BIG);
                    break;
                }
                pb->core.http_content_len = len;
            }
            else if (pb_strncasecmp(pb->core.http_buf, h_close, sizeof h_close - 1) == 0) {
                pb->flags.should_close = true;
            }
#if PUBNUB_RECEIVE_GZIP_RESPONSE
            else if (pb_strncasecmp(pb->core.http_buf, h_encoding, sizeof h_encoding - 1)
                     == 0) {
                pb->data_compressed = compressionGZIP;
            }
#endif
            else if (pbproxy_handle_http_header(pb, pb->core.http_buf) != 0) {
                outcome_detected(pb, PNR_AUTHENTICATION_FAILED);
                return 0;
            }
            pb->state = PBS_RX_HEADERS;
            goto next_state;
        }
        case PNR_TX_BUFF_TOO_SMALL:
            /** We could copy the "line so far" to the reply buffer
                and then process the line from the reply buffer when
                it's finished. This would be much more complex, yet
                most lines will not be that long and, if the reply
                buffer is static, it, too, might not be large enough.
            */
            pb->state = PBS_RX_HEADERS;
            goto next_state;
        default:
            outcome_detected(pb, pbrslt);
            break;
        }
        break;
    case PBS_RX_BODY:
        if (pb->core.http_buf_len < pb->core.http_content_len) {
            pbpal_start_read(pb, pb->core.http_content_len - pb->core.http_buf_len);
            pb->state = PBS_RX_BODY_WAIT;
            goto next_state;
        }
        else {
            finish(pb);
#if PUBNUB_PROXY_API
            if (pb->flags.retry_after_close) {
                goto next_state;
            }
#endif
        }
        break;
    case PBS_RX_BODY_WAIT:
        pbrslt = pbpal_read_status(pb);
        PUBNUB_LOG_TRACE("pb=%p PBS_RX_BODY_WAIT: pbrslt=%d\n", pb, pbrslt);
        switch (pbrslt) {
        case PNR_IN_PROGRESS:
            break;
        case PNR_OK: {
            unsigned len = pbpal_read_len(pb);
            WATCH_UINT(len);
            WATCH_SIZE_T(pb->core.http_buf_len);
            PUBNUB_ASSERT_OPT(pb->core.http_buf_len + len
                              <= pb->core.http_content_len);
            memcpy(pb->core.http_reply + pb->core.http_buf_len, pb->core.http_buf, len);
            pb->core.http_buf_len += len;
            pb->state = PBS_RX_BODY;
            goto next_state;
        }
        default:
            outcome_detected(pb, pbrslt);
            break;
        }
        break;
    case PBS_RX_CHUNK_LEN:
        pbpal_start_read_line(pb);
        pb->state = PBS_RX_CHUNK_LEN_LINE;
        goto next_state;
    case PBS_RX_CHUNK_LEN_LINE:
        pbrslt = pbpal_line_read_status(pb);
        PUBNUB_LOG_TRACE("pb=%p PBS_RX_CHUNK_LEN_LINE: pbrslt=%d\n", pb, pbrslt);
        switch (pbrslt) {
        case PNR_IN_PROGRESS:
            break;
        case PNR_OK: {
            unsigned chunk_length = strtoul(pb->core.http_buf, NULL, 16);

            PUBNUB_LOG_TRACE("About to read a chunk w/length: %u\n", chunk_length);
            if (chunk_length == 0) {
                finish(pb);
#if PUBNUB_PROXY_API
                if (pb->flags.retry_after_close) {
                    goto next_state;
                }
#endif
            }
            else if (0
                     != pbcc_realloc_reply_buffer(
                            &pb->core, pb->core.http_buf_len + chunk_length)) {
                outcome_detected(pb, PNR_REPLY_TOO_BIG);
            }
            else {
                pb->core.http_content_len = chunk_length + CHUNK_TRAIL_LENGTH;
                pb->state                 = PBS_RX_BODY_CHUNK;
                goto next_state;
            }
            break;
        }
        default:
            outcome_detected(pb, pbrslt);
            break;
        }
        break;
    case PBS_RX_BODY_CHUNK:
        if (pb->core.http_content_len > 0) {
            pbpal_start_read(pb, pb->core.http_content_len);
            pb->state = PBS_RX_BODY_CHUNK_WAIT;
        }
        else {
            pb->state = PBS_RX_CHUNK_LEN;
        }
        goto next_state;
    case PBS_RX_BODY_CHUNK_WAIT:
        pbrslt = pbpal_read_status(pb);
        PUBNUB_LOG_TRACE("pb=%p PBS_RX_BODY_CHUNK_WAIT: pbrslt=%d\n", pb, pbrslt);
        switch (pbrslt) {
        case PNR_IN_PROGRESS:
            break;
        case PNR_OK: {
            unsigned len = pbpal_read_len(pb);

            PUBNUB_ASSERT_OPT(pb->core.http_content_len >= len);
            PUBNUB_ASSERT_OPT(len > 0);

            if (pb->core.http_content_len > CHUNK_TRAIL_LENGTH) {
                unsigned to_copy = pb->core.http_content_len - CHUNK_TRAIL_LENGTH;
                if (len < to_copy) {
                    to_copy = len;
                }
                memcpy(pb->core.http_reply + pb->core.http_buf_len,
                       pb->core.http_buf,
                       to_copy);
                pb->core.http_buf_len += to_copy;
            }
            pb->core.http_content_len -= len;
            pb->state = PBS_RX_BODY_CHUNK;
            goto next_state;
        }
        default:
            outcome_detected(pb, pbrslt);
            break;
        }
        break;
    case PBS_WAIT_CLOSE:
        if (pbpal_closed(pb)) {
#if PUBNUB_NEED_RETRY_AFTER_CLOSE
            if (pb->flags.retry_after_close) {
                pb->state = PBS_RETRY;
                goto next_state;
            }
#endif
            pbpal_forget(pb);
            pbntf_trans_outcome(pb, PBS_IDLE);
        }
        break;
    case PBS_WAIT_CANCEL:
        pb->state = PBS_WAIT_CANCEL_CLOSE;
        if (pbpal_close(pb) <= 0) {
            goto next_state;
        }
        break;
    case PBS_WAIT_CANCEL_CLOSE:
        if (pbpal_closed(pb)) {
#if PUBNUB_NEED_RETRY_AFTER_CLOSE
            if (pb->flags.retry_after_close) {
                pb->state = PBS_RETRY;
                goto next_state;
            }
#endif
            pbpal_forget(pb);
            pb->core.msg_ofs = pb->core.msg_end = 0;
            pbntf_trans_outcome(pb, PBS_IDLE);
        }
        break;
    case PBS_WAIT_CANCEL_DNS:
#if PUBNUB_NEED_RETRY_AFTER_CLOSE
        pb->flags.retry_after_close = true;
#endif
        close_connection(pb);
        goto next_state;
    case PBS_KEEP_ALIVE_IDLE:
#if PUBNUB_PROXY_API
        pb->proxy_saved_path_len     = 0;
        pb->proxy_authorization_sent = false;
        pb->auth_msg_count           = 0;
#endif
        pb->state                          = PBS_KEEP_ALIVE_READY;
        pb->flags.started_while_kept_alive = true;
        switch (pbntf_enqueue_for_processing(pb)) {
        case -1:
            pb->core.last_result = PNR_INTERNAL_ERROR;
            pbntf_trans_outcome(pb, PBS_IDLE);
            return 0;
        case 0:
            goto next_state;
        case +1:
            break;
        }
        break;
    case PBS_KEEP_ALIVE_READY:
        i = pbntf_got_socket(pb);
        if (i < 0) {
            pb->core.last_result = PNR_CONNECT_FAILED;
            pbntf_trans_outcome(pb, PBS_IDLE);
            break;
        }
        pb->state = PBS_TX_GET;
        i = pbpal_send_str(pb, get_method_verb_string(pb->method));
        if (i < 0) {
            pb->state = close_kept_alive_connection(pb);
        }
        goto next_state;
    case PBS_KEEP_ALIVE_WAIT_CLOSE:
        if (pbpal_closed(pb)) {
#if PUBNUB_NEED_RETRY_AFTER_CLOSE
            if (pb->flags.retry_after_close) {
                pb->state = PBS_RETRY;
                goto next_state;
            }
#endif
            pbpal_forget(pb);
            pb->state = PBS_IDLE;
            goto next_state;
        }
        break;
    case PBS_WAIT_CANCEL_KEEPALIVE:
        pb->state = close_kept_alive_connection(pb);
        goto next_state;
    default:
        PUBNUB_LOG_ERROR("pbnc_fsm(pb=%p): unhandled state: %s\n",
                         pb,
                         pbnc_state2str(pb->state));
        break;
    }
#if PUBNUB_ADNS_RETRY_AFTER_CLOSE
    if (PBS_RETRY == pb->state) {
        goto next_state;
    }
#endif
    return 0;
}


void pbnc_stop(struct pubnub_* pbp, enum pubnub_res outcome_to_report)
{
    PUBNUB_LOG_TRACE("pbnc_stop(pb=%p, %s) pb->state = %d (%s)\n",
                     pbp,
                     pubnub_res_2_string(outcome_to_report),
                     pbp->state,
                     pbnc_state2str(pbp->state));
    pbp->core.last_result = outcome_to_report;
    switch (pbp->state) {
    case PBS_WAIT_CANCEL:
    case PBS_WAIT_CANCEL_CLOSE:
        break;
    case PBS_WAIT_DNS_SEND:
    case PBS_WAIT_DNS_RCV:
    case PBS_WAIT_CONNECT:
#if defined(PUBNUB_CALLBACK_API)
        if (PNR_TIMEOUT == outcome_to_report) {
            if ((pbp->state != PBS_WAIT_CONNECT) &&
#if PUBNUB_CHANGE_DNS_SERVERS
                (pbpal_dns_rotate_server(pbp) == 0)
#else
                (pbp->flags.sent_queries < PUBNUB_MAX_DNS_QUERIES)
#endif /* PUBNUB_CHANGE_DNS_SERVERS */
            ) {
                pbp->state = PBS_WAIT_CANCEL_DNS;
            }
            else {
                pbp->core.last_result = (PBS_WAIT_CONNECT == pbp->state)
                                        ? PNR_WAIT_CONNECT_TIMEOUT
                                        : PNR_ADDR_RESOLUTION_FAILED;
                pbp->state = PBS_WAIT_CANCEL;
            }
        }
        else {
            pbp->state = PBS_WAIT_CANCEL;
        }
        pbntf_requeue_for_processing(pbp);
#else
        pbp->state = PBS_WAIT_CANCEL;
        pbnc_fsm(pbp);
#endif /* defined(PUBNUB_CALLBACK_API) */
        break;
    case PBS_NULL:
        PUBNUB_LOG_ERROR("pbnc_stop(pbp=%p) got called in NULL state\n", pbp);
        break;
    case PBS_IDLE:
        pbp->trans = PBTT_NONE;
        break;
    case PBS_KEEP_ALIVE_IDLE:
        pbp->trans = PBTT_NONE;
        /*FALLTHRU*/
    case PBS_RX_HTTP_VER:
        /* Transaction generating PNR_TIMEOUT outcome at any point can not end
           up in PBS_KEEP_ALIVE_IDLE so previous *FALLTHROUHGH* is safe */
        if ((PNR_TIMEOUT == outcome_to_report)
            && (pbp->flags.started_while_kept_alive)) {
            pbp->state = PBS_WAIT_CANCEL_KEEPALIVE;
            pbntf_requeue_for_processing(pbp);
            break;
        }
        /*FALLTHRU*/
    default:
        pbp->state = PBS_WAIT_CANCEL;
#if defined(PUBNUB_CALLBACK_API)
        pbntf_requeue_for_processing(pbp);
#else
        pbnc_fsm(pbp);
#endif
        break;
    }
}


bool pbnc_can_start_transaction(struct pubnub_ const* pbp)
{
    switch (pbp->state) {
    case PBS_IDLE:
    case PBS_KEEP_ALIVE_IDLE:
        return true;
    default:
        return false;
    }
}
