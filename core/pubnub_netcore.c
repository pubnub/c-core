/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"

#include "pubnub_ccore.h"
#include "pbpal.h"

#include <string.h>


static void outcome_detected(struct pubnub_ *pb, enum pubnub_res rslt)
{
    pbpal_close(pb);
    pb->core.last_result = rslt;
    pb->state = PBS_WAIT_CLOSE;
}


static void finish(struct pubnub_ *pb)
{
    enum pubnub_res pbres = PNR_OK;

    pb->core.http_reply[pb->core.http_buf_len] = '\0';
    DEBUG_PRINTF("finish('%s')\n", pb->core.http_reply);
    
    switch (pb->trans) {
    case PBTT_SUBSCRIBE:
        if (pbcc_parse_subscribe_response(&pb->core) != 0) {
            DEBUG_PRINTF("parse_subscribe failed\n");
            pbres = PNR_FORMAT_ERROR;
        }
        break;
    case PBTT_PUBLISH:
        pbres = pbcc_parse_publish_response(&pb->core);
        if (pbres != PNR_OK) {
            DEBUG_PRINTF("parse_publish failed\n");
        }
        break;
    case PBTT_TIME:
        if (pbcc_parse_time_response(&pb->core) != 0) {
            DEBUG_PRINTF("parse_time failed\n");
            pbres = PNR_FORMAT_ERROR;
        }
        break;
    case PBTT_HISTORY:
        if (pbcc_parse_history_response(&pb->core) != 0) {
            DEBUG_PRINTF("parse_history failed\n");
            pbres = PNR_FORMAT_ERROR;
        }
        break;
    case PBTT_HISTORYV2:
        if (pbcc_parse_historyv2_response(&pb->core) != 0) {
            DEBUG_PRINTF("parse_historyv2 failed\n");
            pbres = PNR_FORMAT_ERROR;
        }
        break;
    case PBTT_LEAVE:
    case PBTT_HERENOW:
    case PBTT_GLOBAL_HERENOW:
    case PBTT_WHERENOW:
    case PBTT_SET_STATE:
    case PBTT_STATE_GET:
        if (pbcc_parse_presence_response(&pb->core) != 0) {
            DEBUG_PRINTF("parse_presence failed\n");
            pbres = PNR_FORMAT_ERROR;
        }
        break;
    case PBTT_REMOVE_CHANNEL_GROUP:
    case PBTT_REMOVE_CHANNEL_FROM_GROUP:
    case PBTT_ADD_CHANNEL_TO_GROUP:
    case PBTT_LIST_CHANNEL_GROUP:
        pbres = pbcc_parse_channel_registry_response(&pb->core);
        if (pbres != PNR_OK) {
            DEBUG_PRINTF("parse_channel_registry failed\n");
            pbres = PNR_FORMAT_ERROR;
        }
        break;
    default:
        break;
    }

    if ((PNR_OK == pbres) && ((pb->core.http_code / 100) != 2)) {
        pbres = PNR_HTTP_ERROR;
    }

    outcome_detected(pb, pbres);
}


int pbnc_fsm(struct pubnub_ *pb)
{
    enum pubnub_res pbrslt;

    DEBUG_PRINTF("pbnc_fsm()\n");

next_state:
    WATCH(pb->state, "%d");
    switch (pb->state) {
    case PBS_NULL:
        break;
    case PBS_IDLE:
    case PBS_WAIT_DNS:
        pbrslt = pbpal_resolv_and_connect(pb);
        switch (pbrslt) {
        case PNR_IN_PROGRESS:
            pb->state = PBS_WAIT_DNS;
            break;
        case PNR_STARTED:
            pb->state = PBS_CONNECT;
            break;
        case PNR_OK:
            pb->state = PBS_CONNECT;
            goto next_state;
        default:
            pb->core.last_result = pbrslt;
            pbntf_trans_outcome(pb);
            break;
        }
        break;
    case PBS_CONNECT:
        if (pbpal_connected(pb)) {
            pbpal_send_literal_str(pb, "GET ");
            pb->state = PBS_TX_GET;
            goto next_state;
        }
        break;
    case PBS_TX_GET:
        if (pbpal_sent(pb)) {
            pb->state = PBS_TX_PATH;
            if (-1 == pbpal_send_str(pb, pb->core.http_buf)) {
                outcome_detected(pb, PNR_IO_ERROR);
                break;
            }
            goto next_state;
        }
        break;
    case PBS_TX_PATH:
        if (pbpal_sent(pb)) {
            pbpal_send_literal_str(pb, " HTTP/1.1\r\nHost: ");
            pb->state = PBS_TX_VER;
            goto next_state;
        }
        break;
    case PBS_TX_VER:
        if (pbpal_sent(pb)) {
            pb->state = PBS_TX_ORIGIN;
            if (-1 == pbpal_send_str(pb, PUBNUB_ORIGIN)) {
                outcome_detected(pb, PNR_IO_ERROR);
                break;
            }
            goto next_state;
        }
        break;
    case PBS_TX_ORIGIN:
        if (pbpal_sent(pb)) {
            pbpal_send_literal_str(pb, "\r\nUser-Agent: PubNub-C-core/0.1\r\nConnection: Keep-Alive\r\n\r\n");
            pb->state = PBS_TX_FIN_HEAD;
            goto next_state;
        }
        break;
    case PBS_TX_FIN_HEAD:
        DEBUG_PRINTF("PBS_TX_FIN_HEAD\n");
        if (pbpal_sent(pb)) {
            pbpal_start_read_line(pb);
            pb->state = PBS_RX_HTTP_VER;
            goto next_state;
        }
        break;
    case PBS_RX_HTTP_VER:
        DEBUG_PRINTF("PBS_RX_HTTP_VER\n");
        if (pbpal_line_read(pb)) {
            if (strncmp(pb->core.http_buf, "HTTP/1.", 7) != 0) {
                outcome_detected(pb, PNR_IO_ERROR);
                break;
            }
            pb->core.http_code = atoi(pb->core.http_buf + 9);
            WATCH(pb->core.http_code, "%d");
            WATCH(pbpal_read_len(pb), "%d");
            pb->core.http_content_len = 0;
            pb->core.http_chunked = false;
            pb->state = PBS_RX_HEADERS;
            goto next_state;
        }
        break;
    case PBS_RX_HEADERS:
        DEBUG_PRINTF("PBS_RX_HEADERS\n");
        pbpal_start_read_line(pb);
        pb->state = PBS_RX_HEADER_LINE;
        goto next_state;
    case PBS_RX_HEADER_LINE:
        DEBUG_PRINTF("PBS_RX_HEADER_LINE\n");
        if (pbpal_line_read(pb)) {
            char h_chunked[] = "Transfer-Encoding: chunked";
            char h_length[] = "Content-Length: ";
            DEBUG_PRINTF("header line was read: %.*s\n", pbpal_read_len(pb), pb->core.http_buf);
            WATCH(pbpal_read_len(pb), "%d");
            if (pbpal_read_len(pb) <= 2) {
                pb->core.http_buf_len = 0;
                pb->state = pb->core.http_chunked ? PBS_RX_CHUNK_LEN : PBS_RX_BODY;
                goto next_state;
            }
            if (strncmp(pb->core.http_buf, h_chunked, sizeof h_chunked - 1) == 0) {
                pb->core.http_chunked = true;
            }
            else if (strncmp(pb->core.http_buf, h_length, sizeof h_length - 1) == 0) {
                pb->core.http_content_len = atoi(pb->core.http_buf + sizeof h_length - 1);
                if (pb->core.http_content_len > PUBNUB_REPLY_MAXLEN) {
                    outcome_detected(pb, PNR_IO_ERROR);
                    break;
                }
            }
            pb->state = PBS_RX_HEADERS;
            goto next_state;
        }
        break;
    case PBS_RX_BODY:
        DEBUG_PRINTF("PBS_RX_BODY\n");
        if (pb->core.http_buf_len < pb->core.http_content_len) {
            pbpal_start_read(pb, pb->core.http_content_len - pb->core.http_buf_len);
            pb->state = PBS_RX_BODY_WAIT;
        }
        else {
            finish(pb);
        }
        goto next_state;
    case PBS_RX_BODY_WAIT:
        DEBUG_PRINTF("PBS_RX_BODY_WAIT\n");
        if (pbpal_read_over(pb)) {
            unsigned len = pbpal_read_len(pb);
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
        pbpal_start_read_line(pb);
        pb->state = PBS_RX_CHUNK_LEN_LINE;
        goto next_state;
    case PBS_RX_CHUNK_LEN_LINE:
        if (pbpal_line_read(pb)) {
            pb->core.http_content_len = strtoul(pb->core.http_buf, NULL, 16);
            DEBUG_PRINTF("About to read a chunk w/length: %d\n", pb->core.http_content_len);
            if (pb->core.http_content_len == 0) {
                finish(pb);
            }
            else if (pb->core.http_content_len > sizeof pb->core.http_buf) {
                outcome_detected(pb, PNR_IO_ERROR);
            }
            else if (pb->core.http_buf_len + pb->core.http_content_len > PUBNUB_REPLY_MAXLEN) {
                outcome_detected(pb, PNR_IO_ERROR);
            }
            else {
                pbpal_start_read(pb, pb->core.http_content_len + 2);
                pb->state = PBS_RX_BODY_CHUNK;
            }
            goto next_state;
        }
        break;
    case PBS_RX_BODY_CHUNK:
        if (pbpal_read_over(pb)) {
            memcpy(
                pb->core.http_reply + pb->core.http_buf_len, 
                pb->core.http_buf, 
                pb->core.http_content_len
                );
            pb->core.http_buf_len += pb->core.http_content_len;
            pb->state = PBS_RX_CHUNK_LEN;
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
        pbpal_close(pb);
        pb->state = PBS_WAIT_CANCEL_CLOSE;
        break;
    case PBS_WAIT_CANCEL_CLOSE:
        if (pbpal_closed(pb)) {
            pbpal_forget(pb);
            pb->core.msg_ofs = pb->core.msg_end = 0;
            pb->core.last_result = PNR_CANCELLED;
            pbntf_trans_outcome(pb);
        }
        break;
    }
    return 0;
}
