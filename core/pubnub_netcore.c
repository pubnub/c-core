/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"

#include "pubnub_assert.h"
#include "pubnub_log.h"
#include "pubnub_ccore.h"
#include "pbpal.h"

#include <string.h>


static void outcome_detected(struct pubnub_ *pb, enum pubnub_res rslt)
{
    pb->core.last_result = rslt;
    if (pbpal_close(pb) <= 0) {
        pbpal_forget(pb);
        pbntf_trans_outcome(pb);
    }
    else {
        pb->state = PBS_WAIT_CLOSE;
    }
}


static enum pubnub_res parse_pubnub_result(struct pubnub_ *pb)
{
    enum pubnub_res pbres = PNR_OK;
    switch (pb->trans) {
    case PBTT_SUBSCRIBE:
        if (pbcc_parse_subscribe_response(&pb->core) != 0) {
            PUBNUB_LOG_WARNING("parse_subscribe failed\n");
            pbres = PNR_FORMAT_ERROR;
        }
        break;
    case PBTT_PUBLISH:
        pbres = pbcc_parse_publish_response(&pb->core);
        if (pbres != PNR_OK) {
            PUBNUB_LOG_WARNING("parse_publish failed\n");
        }
        break;
    case PBTT_TIME:
        if (pbcc_parse_time_response(&pb->core) != 0) {
            PUBNUB_LOG_WARNING("parse_time failed\n");
            pbres = PNR_FORMAT_ERROR;
        }
        break;
    case PBTT_HISTORY:
        if (pbcc_parse_history_response(&pb->core) != 0) {
            PUBNUB_LOG_WARNING("parse_history failed\n");
            pbres = PNR_FORMAT_ERROR;
        }
        break;
    case PBTT_LEAVE:
    case PBTT_HERENOW:
    case PBTT_GLOBAL_HERENOW:
    case PBTT_WHERENOW:
    case PBTT_SET_STATE:
    case PBTT_STATE_GET:
    case PBTT_HEARTBEAT:
        if (pbcc_parse_presence_response(&pb->core) != 0) {
            PUBNUB_LOG_WARNING("parse_presence failed\n");
            pbres = PNR_FORMAT_ERROR;
        }
        break;
    case PBTT_REMOVE_CHANNEL_GROUP:
    case PBTT_REMOVE_CHANNEL_FROM_GROUP:
    case PBTT_ADD_CHANNEL_TO_GROUP:
    case PBTT_LIST_CHANNEL_GROUP:
        pbres = pbcc_parse_channel_registry_response(&pb->core);
        if (pbres != PNR_OK) {
            PUBNUB_LOG_WARNING("parse_channel_registry failed\n");
        }
        break;
    default:
        break;
    }

    return pbres;
}


static void finish(struct pubnub_ *pb)
{
    enum pubnub_res pbres;

#if PUBNUB_PROXY_API
    if (pb->proxy_type == pbproxyHTTP_CONNECT) {
        if (!pb->proxy_tunnel_established) {
            if ((pb->http_code / 100) != 2) {
                outcome_detected(pb, PNR_HTTP_ERROR);
            }
            else {
                pb->proxy_tunnel_established = true;
                pb->state = PBS_CONNECTED;
            }

            return;
        }
        else {
            pb->proxy_tunnel_established = false;
        }
    }
#endif

    pb->core.http_reply[pb->core.http_buf_len] = '\0';
    PUBNUB_LOG_TRACE("finish('%s')\n", pb->core.http_reply);

    pbres = parse_pubnub_result(pb);
    if ((PNR_OK == pbres) && ((pb->http_code / 100) != 2)) {
        pbres = PNR_HTTP_ERROR;
    }
    
    outcome_detected(pb, pbres);
}


int pbnc_fsm(struct pubnub_ *pb)
{
    enum pubnub_res pbrslt;
    int i;

    PUBNUB_LOG_TRACE("pbnc_fsm()\t");

next_state:
    WATCH_ENUM(pb->state);
    switch (pb->state) {
    case PBS_NULL:
        break;
    case PBS_IDLE:
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
                pbpal_send_literal_str(pb, "http://");
                break;
            case pbproxyHTTP_CONNECT:
                pb->state = PBS_TX_SCHEME;
                if (i < 0) {
                    outcome_detected(pb, PNR_IO_ERROR);
                    break;
                }
                if (!pb->proxy_tunnel_established) {
                    strcpy(pb->proxy_saved_path, pb->core.http_buf);
                }
                else {
                    strcpy(pb->core.http_buf, pb->proxy_saved_path);
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
        if (i <= 0) {
            if ((pb->proxy_type == pbproxyHTTP_CONNECT) && !pb->proxy_tunnel_established) {
                char port_num[20];
                snprintf(port_num, sizeof port_num, ":%d", 80);
                pbpal_send_str(pb, port_num);
                pb->state = PBS_TX_PORT_NUM;
                goto next_state;
            }
            else {
                pb->state = PBS_TX_PATH;
                if ((i < 0) || (-1 == pbpal_send_str(pb, pb->core.http_buf))) {
                    outcome_detected(pb, PNR_IO_ERROR);
                    break;
                }
            }
            goto next_state;
        }
        break;
    case PBS_TX_PORT_NUM:
        i = pbpal_send_status(pb);
        if (i <= 0) {
            pb->state = PBS_TX_PATH;
            if (i < 0) {
                outcome_detected(pb, PNR_IO_ERROR);
                break;
            }
            goto next_state;
        }
        break;
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
            pbpal_send_literal_str(pb, "\r\nUser-Agent: PubNub-C-core/2.1\r\nConnection: Keep-Alive\r\n\r\n");
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
        switch (pbrslt) {
        case PNR_IN_PROGRESS:
            break;
        case PNR_OK:
            if (strncmp(pb->core.http_buf, "HTTP/1.", 7) != 0) {
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
            outcome_detected(pb, pbrslt);
            break;
        }
        break;
    case PBS_RX_HEADERS:
        PUBNUB_LOG_TRACE("PBS_RX_HEADERS\n");
        pbpal_start_read_line(pb);
        pb->state = PBS_RX_HEADER_LINE;
        goto next_state;
    case PBS_RX_HEADER_LINE:
        PUBNUB_LOG_TRACE("PBS_RX_HEADER_LINE\n");
        pbrslt = pbpal_line_read_status(pb);
        switch (pbrslt) {
        case PNR_IN_PROGRESS:
            break;
        case PNR_OK:
        {
            char h_chunked[] = "Transfer-Encoding: chunked";
            char h_length[] = "Content-Length: ";
            int read_len = pbpal_read_len(pb);
            PUBNUB_LOG_TRACE("header line was read: %.*s\n", read_len, pb->core.http_buf);
            WATCH_INT(read_len);
            if (read_len <= 2) {
                pb->core.http_buf_len = 0;
                if (!pb->http_chunked) {
                    if (0 == pb->core.http_content_len) {
#if PUBNUB_PROXY_API
                        if ((pb->proxy_type == pbproxyHTTP_CONNECT) && !pb->proxy_tunnel_established) {
                            finish(pb);
                            break;
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
            pb->state = PBS_RX_HEADERS;
            goto next_state;
        }
        default:
            outcome_detected(pb, pbrslt);
            break;
        }
        break;
    case PBS_RX_BODY:
        PUBNUB_LOG_TRACE("PBS_RX_BODY\n");
        if (pb->core.http_buf_len < pb->core.http_content_len) {
            pbpal_start_read(pb, pb->core.http_content_len - pb->core.http_buf_len);
            pb->state = PBS_RX_BODY_WAIT;
            goto next_state;
        }
        else {
            finish(pb);
        }
        break;
    case PBS_RX_BODY_WAIT:
        PUBNUB_LOG_TRACE("PBS_RX_BODY_WAIT\n");
        if (pbpal_read_over(pb)) {
            unsigned len = pbpal_read_len(pb);
            WATCH_UINT(len);
            WATCH_UINT(pb->core.http_buf_len);
            memcpy(
                pb->core.http_reply + pb->core.http_buf_len,
                pb->core.http_buf,
                len
                );
            pb->core.http_buf_len += len;
            pb->state = PBS_RX_BODY;
            goto next_state;
        }
        break;
    case PBS_RX_CHUNK_LEN:
        PUBNUB_LOG_TRACE("PBS_RX_CHUNK_LEN\n");
        pbpal_start_read_line(pb);
        pb->state = PBS_RX_CHUNK_LEN_LINE;
        goto next_state;
    case PBS_RX_CHUNK_LEN_LINE:
        pbrslt = pbpal_line_read_status(pb);
        PUBNUB_LOG_TRACE("PBS_RX_CHUNK_LEN_LINE: pbrslt=%d\n", pbrslt);
        switch (pbrslt) {
        case PNR_IN_PROGRESS:
            break;
        case PNR_OK:
        {
            unsigned chunk_length = strtoul(pb->core.http_buf, NULL, 16);

            PUBNUB_LOG_TRACE("About to read a chunk w/length: %d\n", chunk_length);
            if (chunk_length == 0) {
                finish(pb);
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
        PUBNUB_LOG_TRACE("PBS_RX_BODY_CHUNK\n");
        if (pb->core.http_content_len > 0) {
            pbpal_start_read(pb, pb->core.http_content_len);
            pb->state = PBS_RX_BODY_CHUNK_WAIT;
        }
        else {
            pb->state = PBS_RX_CHUNK_LEN;
        }
        goto next_state;
    case PBS_RX_BODY_CHUNK_WAIT:
        PUBNUB_LOG_TRACE("PBS_RX_BODY_CHUNK_WAIT\n");
        if (pbpal_read_over(pb)) {
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
        break;
    case PBS_WAIT_CLOSE:
        if (pbpal_closed(pb)) {
            pbpal_forget(pb);
            pbntf_trans_outcome(pb);
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
            pbpal_forget(pb);
            pb->core.msg_ofs = pb->core.msg_end = 0;
            pbntf_trans_outcome(pb);
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
