/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"

#include "pubnub_assert.h"
#include "pubnub_log.h"
#include "pubnub_ccore.h"
#include "pubnub_ccore_pubsub.h"
#include "pbpal.h"

#include "pubnub_proxy_core.h"

#include <string.h>


static void outcome_detected(struct pubnub_ *pb, enum pubnub_res rslt)
{
    pb->core.last_result = rslt;
    if (pbpal_close(pb) <= 0) {
#if PUBNUB_PROXY_API
        PUBNUB_LOG_TRACE("outcome_detected(): pb->retry_after_close=%d\n", pb->retry_after_close);
        if (pb->retry_after_close) {
            pb->state = PBS_IDLE;
        }
        else {
            pbpal_forget(pb);
            pbntf_trans_outcome(pb);
        }
#else
        pbpal_forget(pb);
        pbntf_trans_outcome(pb);
#endif
    }
    else {
        pb->state = PBS_WAIT_CLOSE;
    }
}


static enum pubnub_res dont_parse(struct pbcc_context *p)
{
    PUBNUB_UNUSED(p);
    PUBNUB_LOG_ERROR("Parsing not initialized for a type of transaction\n");
    return PNR_INTERNAL_ERROR;
}


static PFpbcc_parse_response_T m_aParseResponse[] = {
    dont_parse,
    pbcc_parse_subscribe_response,
    pbcc_parse_publish_response,
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
#endif
};


PUBNUB_STATIC_ASSERT((sizeof m_aParseResponse / sizeof m_aParseResponse[0]) == PBTT_MAX, bad_response_table_length);


static enum pubnub_res parse_pubnub_result(struct pubnub_ *pb)
{
    enum pubnub_res pbres;

    pbres = m_aParseResponse[pb->trans](&pb->core);
    if (pbres != PNR_OK) {
        PUBNUB_LOG_WARNING("pb=%p parsing response for transaction type #%d failed\n", pb, pb->trans);
    }

    return pbres;
}


static void finish(struct pubnub_ *pb)
{
    enum pubnub_res pbres;

#if PUBNUB_PROXY_API
    switch (pbproxy_handle_finish(pb)) {
    case pbproxyFinError:
        PUBNUB_LOG_TRACE("Proxy: Error, close connection\n");
        outcome_detected(pb, PNR_HTTP_ERROR);
        return;
    case pbproxyFinRetryConnected:
        PUBNUB_LOG_TRACE("Proxy: retry in current connection\n");
        pb->state = PBS_CONNECTED;
        PUBNUB_ASSERT_OPT(pb->proxy_saved_path_len < PUBNUB_BUF_MAXLEN);
        memmove(pb->core.http_buf, pb->proxy_saved_path, pb->proxy_saved_path_len + 1);
        pb->core.http_buf_len = pb->proxy_saved_path_len;
        return;
    case pbproxyFinRetryReconnect:
        PUBNUB_LOG_TRACE("Proxy: Will retry after close\n");
        PUBNUB_ASSERT_OPT(pb->proxy_saved_path_len < PUBNUB_BUF_MAXLEN);
        memmove(pb->core.http_buf, pb->proxy_saved_path, pb->proxy_saved_path_len + 1);
        pb->core.http_buf_len = pb->proxy_saved_path_len;
        pb->retry_after_close = true;
        break;
    default:
        break;
    }
#endif

    pb->core.http_reply[pb->core.http_buf_len] = '\0';
    PUBNUB_LOG_TRACE("finish(pb=%p, '%s')\n", pb, pb->core.http_reply);

    pbres = parse_pubnub_result(pb);
    if ((PNR_OK == pbres) && ((pb->http_code / 100) != 2)) {
        pbres = PNR_HTTP_ERROR;
    }
    
    outcome_detected(pb, pbres);
}


static char const* pbnc_state2str(enum pubnub_state e)
{
    switch (e) {
    case PBS_NULL: return "PBS_NULL";
    case PBS_IDLE: return "PBS_IDLE";
    case PBS_READY: return "PBS_READY";
    case PBS_WAIT_DNS_SEND: return "PBS_WAIT_DNS_SEND";
    case PBS_WAIT_DNS_RCV: return "PBS_WAIT_DNS_RCV";
    case PBS_WAIT_CONNECT: return "PBS_WAIT_CONNECT";
    case PBS_CONNECTED: return "PBS_CONNECTED";
    case PBS_TX_GET: return "PBS_TX_GET";
    case PBS_TX_PATH: return "PBS_TX_PATH";
    case PBS_TX_SCHEME: return "PBS_TX_SCHEME";
    case PBS_TX_HOST: return "PBS_TX_HOST";
    case PBS_TX_PORT_NUM: return "PBS_TX_PORT_NUM";
    case PBS_TX_VER: return "PBS_TX_VER";
    case PBS_TX_PROXY_AUTHORIZATION: return "PBS_TX_PROXY_AUTHORIZATION";
    case PBS_TX_ORIGIN: return "PBS_TX_ORIGIN";
    case PBS_TX_FIN_HEAD: return "PBS_TX_FIN_HEAD";
    case PBS_RX_HTTP_VER: return "PBS_RX_HTTP_VER";
    case PBS_RX_HEADERS: return "PBS_RX_HEADERS";
    case PBS_RX_HEADER_LINE: return "PBS_RX_HEADER_LINE";
    case PBS_RX_BODY: return "PBS_RX_BODY";
    case PBS_RX_BODY_WAIT: return "PBS_RX_BODY_WAIT";
    case PBS_RX_CHUNK_LEN: return "PBS_RX_CHUNK_LEN";
    case PBS_RX_CHUNK_LEN_LINE: return "PBS_RX_CHUNK_LEN_LINE";
    case PBS_RX_BODY_CHUNK: return "PBS_RX_BODY_CHUNK";
    case PBS_RX_BODY_CHUNK_WAIT: return "PBS_RX_BODY_CHUNK_WAIT";
    case PBS_WAIT_CLOSE: return "PBS_WAIT_CLOSE";
    case PBS_WAIT_CANCEL: return "PBS_WAIT_CANCEL";
    case PBS_WAIT_CANCEL_CLOSE: return "PBS_WAIT_CANCEL_CLOSE";
    default: return "Unknown enum pubnub_state";
    }
}


int pbnc_fsm(struct pubnub_ *pb)
{
    enum pubnub_res pbrslt;
    int i;

    PUBNUB_LOG_TRACE("pbnc_fsm(pb=%p)\t", pb);

next_state:
    PUBNUB_LOG_TRACE("pb->state = %d (%s)\n", pb->state, pbnc_state2str(pb->state));
    switch (pb->state) {
    case PBS_NULL:
        break;
    case PBS_IDLE:
#if PUBNUB_PROXY_API
        pb->retry_after_close = false;
        pb->proxy_tunnel_established = false;
#endif
        pb->state = PBS_READY;
        switch (pbntf_enqueue_for_processing(pb)) {
        case -1:
            pb->core.last_result = PNR_INTERNAL_ERROR;
            pbntf_trans_outcome(pb);
            return 0;
        case 0:
            goto next_state;
        case +1:
            break;
        }
        break;
    case PBS_READY:
    {
        enum pbpal_resolv_n_connect_result rslv = pbpal_resolv_and_connect(pb);
        WATCH_ENUM(rslv);
        switch (rslv) {
        case pbpal_resolv_send_wouldblock:
            pb->state = PBS_WAIT_DNS_SEND;
            break;
        case pbpal_resolv_sent:
        case pbpal_resolv_rcv_wouldblock:
            pb->state = PBS_WAIT_DNS_RCV;
            pbntf_watch_in_events(pb);
            break;
        case pbpal_connect_wouldblock:
            pb->state = PBS_WAIT_CONNECT;
            break;
        case pbpal_connect_success:
            pb->state = PBS_CONNECTED;
            break;
        default:
            pb->core.last_result = PNR_ADDR_RESOLUTION_FAILED;
            pbntf_trans_outcome(pb);
            return 0;
        }
        i = pbntf_got_socket(pb, pb->pal.socket);
        if (0 == i) {
            goto next_state;
        }
        else if (i < 0) {
            pb->core.last_result = PNR_CONNECT_FAILED;
            pbntf_trans_outcome(pb);
        }
        break;
    }
    case PBS_WAIT_DNS_SEND:
    {
        enum pbpal_resolv_n_connect_result rslv = pbpal_check_resolv_and_connect(pb);
        WATCH_ENUM(rslv);
        switch (rslv) {
        case pbpal_resolv_send_wouldblock:
            break;
        case pbpal_resolv_sent:
        case pbpal_resolv_rcv_wouldblock:
            pbntf_update_socket(pb, pb->pal.socket);
            pb->state = PBS_WAIT_DNS_RCV;
            pbntf_watch_in_events(pb);
            break;
        case pbpal_connect_wouldblock:
            pbntf_update_socket(pb, pb->pal.socket);
            pb->state = PBS_WAIT_CONNECT;
            break;
        case pbpal_connect_success:
            pb->state = PBS_CONNECTED;
            goto next_state;
        default:
            outcome_detected(pb, PNR_ADDR_RESOLUTION_FAILED);
            break;
        }
        break;
    }
    case PBS_WAIT_DNS_RCV:
    {
        enum pbpal_resolv_n_connect_result rslv = pbpal_check_resolv_and_connect(pb);
        WATCH_ENUM(rslv);
        switch (rslv) {
        case pbpal_resolv_send_wouldblock:
        case pbpal_resolv_sent:
            outcome_detected(pb, PNR_INTERNAL_ERROR);
            break;
        case pbpal_resolv_rcv_wouldblock:
            break;
        case pbpal_connect_wouldblock:
            pbntf_update_socket(pb, pb->pal.socket);
            pb->state = PBS_WAIT_CONNECT;
            pbntf_watch_out_events(pb);
            break;
        case pbpal_connect_success:
            pb->state = PBS_CONNECTED;
            pbntf_watch_out_events(pb);
            goto next_state;
        default:
            outcome_detected(pb, PNR_ADDR_RESOLUTION_FAILED);
            break;
        }
        break;
    }
    case PBS_WAIT_CONNECT:
    {
        enum pbpal_resolv_n_connect_result rslv = pbpal_check_connect(pb);
        WATCH_ENUM(rslv);
        switch (rslv) {
        case pbpal_resolv_send_wouldblock:
        case pbpal_resolv_sent:
        case pbpal_resolv_rcv_wouldblock:
            pb->core.last_result = PNR_INTERNAL_ERROR;
            pbntf_trans_outcome(pb);
            break;
        case pbpal_connect_wouldblock:
            break;
        case pbpal_connect_success:
            pb->state = PBS_CONNECTED;
            goto next_state;
        default:
            outcome_detected(pb, PNR_CONNECT_FAILED);
            break;
        }
        break;
    }
    case PBS_CONNECTED:
#if PUBNUB_PROXY_API
        if ((pb->proxy_type == pbproxyHTTP_CONNECT) && (!pb->proxy_tunnel_established)) {
            pbpal_send_literal_str(pb, "CONNECT ");
        }
        else {
            pbpal_send_literal_str(pb, "GET ");
        }
#else
        pbpal_send_literal_str(pb, "GET ");
#endif
        pb->state = PBS_TX_GET;
        goto next_state;
    case PBS_TX_GET:
        i = pbpal_send_status(pb);
        if (i <= 0) {
#if PUBNUB_PROXY_API
            switch (pb->proxy_type) {
            case pbproxyHTTP_GET:
                pb->state = PBS_TX_SCHEME;
                if (i < 0) {
                    outcome_detected(pb, PNR_IO_ERROR);
                    break;
                }
                PUBNUB_ASSERT_OPT(pb->core.http_buf_len < PUBNUB_BUF_MAXLEN);
                memcpy(pb->proxy_saved_path, pb->core.http_buf, pb->core.http_buf_len + 1);
                pb->proxy_saved_path_len = pb->core.http_buf_len;
                pbpal_send_literal_str(pb, "http://");
                break;
            case pbproxyHTTP_CONNECT:
                pb->state = PBS_TX_SCHEME;
                if (i < 0) {
                    outcome_detected(pb, PNR_IO_ERROR);
                    break;
                }
                if (!pb->proxy_tunnel_established) {
                    PUBNUB_ASSERT_OPT(pb->core.http_buf_len < PUBNUB_BUF_MAXLEN);
                    memcpy(pb->proxy_saved_path, pb->core.http_buf, pb->core.http_buf_len + 1);
                    pb->proxy_saved_path_len = pb->core.http_buf_len;
                }
                else {
                    PUBNUB_ASSERT_OPT(pb->proxy_saved_path_len < PUBNUB_BUF_MAXLEN);
                    memmove(pb->core.http_buf, pb->proxy_saved_path, pb->proxy_saved_path_len + 1);
                    pb->core.http_buf_len = pb->proxy_saved_path_len;
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
            if ((pb->proxy_type == pbproxyHTTP_CONNECT) && pb->proxy_tunnel_established) {
                pb->state = PBS_TX_HOST;
            }
            else {
                char const* o = PUBNUB_ORIGIN_SETTABLE ? pb->origin : PUBNUB_ORIGIN;
                pb->state = PBS_TX_HOST;
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
            if ((pb->proxy_type == pbproxyHTTP_CONNECT) && !pb->proxy_tunnel_established) {
                pbpal_send_literal_str(pb, ":80");
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
            pbpal_send_literal_str(pb, " HTTP/1.1\r\nHost: ");
            pb->state = PBS_TX_VER;
            goto next_state;
        }
        break;
    case PBS_TX_VER:
        i = pbpal_send_status(pb);
        if (i <= 0) {
            char const* o = PUBNUB_ORIGIN_SETTABLE ? pb->origin : PUBNUB_ORIGIN;
            pb->state = PBS_TX_ORIGIN;
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
            char header_to_send[1024] = "\r\n";
            if (0 == pbproxy_http_header_to_send(pb, header_to_send+2, sizeof header_to_send-2)) {
                PUBNUB_LOG_TRACE("Sending HTTP proxy header: '%s'\n", header_to_send);
                pb->state = PBS_TX_PROXY_AUTHORIZATION;
                if (-1 == pbpal_send_str(pb, header_to_send)) {
                    outcome_detected(pb, PNR_IO_ERROR);
                    break;
                }
            }
            else {
                pbpal_send_literal_str(pb, "\r\nUser-Agent: PubNub-C-core/2.2\r\nConnection: Keep-Alive\r\n\r\n");
                pb->state = PBS_TX_FIN_HEAD;
            }
#else
            pbpal_send_literal_str(pb, "\r\nUser-Agent: PubNub-C-core/2.2\r\nConnection: Keep-Alive\r\n\r\n");
            pb->state = PBS_TX_FIN_HEAD;
#endif
            goto next_state;
        }
        break;
    case PBS_TX_PROXY_AUTHORIZATION:
        i = pbpal_send_status(pb);
        if (i < 0) {
            outcome_detected(pb, PNR_IO_ERROR);
        }
        else if (0 == i) {
            pbpal_send_literal_str(pb, "\r\nUser-Agent: PubNub-C-core/2.2\r\nConnection: Keep-Alive\r\n\r\n");
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
            pbpal_start_read_line(pb);
            pb->state = PBS_RX_HTTP_VER;
            pbntf_watch_in_events(pb);
            goto next_state;
        }
        break;
    case PBS_RX_HTTP_VER:
        pbrslt = pbpal_line_read_status(pb);
        PUBNUB_LOG_TRACE("pb=%p PBS_RX_HTTP_VER: pbrslt=%d\n", pb, pbrslt);
        switch (pbrslt) {
        case PNR_IN_PROGRESS:
            break;
        case PNR_OK:
            if (strncmp(pb->core.http_buf, "HTTP/1.", 7) != 0) {
                PUBNUB_LOG_ERROR("pb=%p bad HTTP response version: %.*s\n", pb, pbpal_read_len(pb), pb->core.http_buf);
                outcome_detected(pb, PNR_IO_ERROR);
                break;
            }
            pb->http_code = atoi(pb->core.http_buf + 9);
            WATCH_USHORT(pb->http_code);
            pb->core.http_content_len = 0;
            pb->http_chunked = false;
            pb->state = PBS_RX_HEADERS;
            goto next_state;
        default:
            PUBNUB_LOG_ERROR("pb=%p in PBS_RX_HTTP_VER: failure inducing pbpal_line_read_status %d\n", pb, pbrslt);
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
        case PNR_OK:
        {
            char h_chunked[] = "Transfer-Encoding: chunked";
            char h_length[] = "Content-Length: ";
            int read_len = pbpal_read_len(pb);
            PUBNUB_LOG_TRACE("pb=%p header line was read: '%.*s'\n", pb, read_len, pb->core.http_buf);
            WATCH_INT(read_len);
            if (read_len <= 2) {
                pb->core.http_buf_len = 0;
                if (!pb->http_chunked) {
                    if (0 == pb->core.http_content_len) {
#if PUBNUB_PROXY_API
                        WATCH_ENUM(pb->proxy_type);
                        WATCH_INT(pb->proxy_tunnel_established);
                        if ((pb->proxy_type == pbproxyHTTP_CONNECT) && !pb->proxy_tunnel_established) {
                            finish(pb);
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
            if (strncmp(pb->core.http_buf, h_chunked, sizeof h_chunked - 1) == 0) {
                pb->http_chunked = true;
            }
            else if (strncmp(pb->core.http_buf, h_length, sizeof h_length - 1) == 0) {
                size_t len = atoi(pb->core.http_buf + sizeof h_length - 1);
                if (0 != pbcc_realloc_reply_buffer(&pb->core, len)) {
                    outcome_detected(pb, PNR_REPLY_TOO_BIG);
                    break;
                }
                pb->core.http_content_len = len;
            }
            else {
                pbproxy_handle_http_header(pb, pb->core.http_buf);
            }
            pb->state = PBS_RX_HEADERS;
            goto next_state;
        }
        case PNR_TX_BUFF_TOO_SMALL:
            /** We could try to copy the "line so far" to the reply
                buffer and then do some processing there, but, it
                could become very complex, as we could have a few more
                of these until we finally find a newline. So, for now,
                just try to recover by skipping this line.
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
            if (pb->retry_after_close) {
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
        case PNR_OK:
        {
            unsigned len = pbpal_read_len(pb);
            WATCH_UINT(len);
            WATCH_UINT(pb->core.http_buf_len);
            PUBNUB_ASSERT_OPT(pb->core.http_buf_len + len <= pb->core.http_content_len);
            memcpy(
                pb->core.http_reply + pb->core.http_buf_len,
                pb->core.http_buf,
                len
                );
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
        case PNR_OK:
        {
            unsigned chunk_length = strtoul(pb->core.http_buf, NULL, 16);

            PUBNUB_LOG_TRACE("About to read a chunk w/length: %d\n", chunk_length);
            if (chunk_length == 0) {
                finish(pb);
#if PUBNUB_PROXY_API
                if (pb->retry_after_close) {
                    goto next_state;
                }
#endif
            }
            else if (chunk_length > sizeof pb->core.http_buf) {
                outcome_detected(pb, PNR_IO_ERROR);
            }
            else if (0 != pbcc_realloc_reply_buffer(&pb->core, pb->core.http_buf_len + chunk_length)) {
                outcome_detected(pb, PNR_REPLY_TOO_BIG);
            }
            else {
                pb->core.http_content_len = chunk_length + 2;
                pb->state = PBS_RX_BODY_CHUNK;
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
        case PNR_OK:
        {
            unsigned len = pbpal_read_len(pb);

            PUBNUB_ASSERT_OPT(pb->core.http_content_len >= len);
            PUBNUB_ASSERT_OPT(len > 0);

            if (pb->core.http_content_len > 2) {
                unsigned to_copy = pb->core.http_content_len - 2;
                if (len < to_copy) {
                    to_copy = len;
                }
                memcpy(
                    pb->core.http_reply + pb->core.http_buf_len,
                    pb->core.http_buf,
                    to_copy
                    );
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
#if PUBNUB_PROXY_API
            if (pb->retry_after_close) {
                pb->state = PBS_IDLE;
            }
            else {
                pbpal_forget(pb);
                pbntf_trans_outcome(pb);
            }
#else
            pbpal_forget(pb);
            pbntf_trans_outcome(pb);
#endif
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
#if PUBNUB_PROXY_API
            if (pb->retry_after_close) {
                pb->state = PBS_IDLE;
            }
            else {
                pbpal_forget(pb);
                pb->core.msg_ofs = pb->core.msg_end = 0;
                pbntf_trans_outcome(pb);
            }
#else
            pbpal_forget(pb);
            pb->core.msg_ofs = pb->core.msg_end = 0;
            pbntf_trans_outcome(pb);
#endif
        }
        break;
    }
    return 0;
}


void pbnc_stop(struct pubnub_ *pb, enum pubnub_res outcome_to_report)
{
    pb->core.last_result = outcome_to_report;
    switch (pb->state) {
    case PBS_WAIT_CANCEL:
    case PBS_WAIT_CANCEL_CLOSE:
        break;
    case PBS_IDLE:
    case PBS_NULL:
        pbntf_trans_outcome(pb);
        break;
    default:
        pb->state = PBS_WAIT_CANCEL;
        pbntf_requeue_for_processing(pb);
        break;
    }
}
