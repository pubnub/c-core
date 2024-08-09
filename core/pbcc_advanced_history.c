/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"

#if PUBNUB_USE_ADVANCED_HISTORY
#include "pubnub_memory_block.h"
#include "pubnub_server_limits.h"
#include "pubnub_advanced_history.h"
#include "pubnub_version.h"
#include "pubnub_json_parse.h"
#include "pubnub_url_encode.h"

#include "pubnub_assert.h"
#include "pubnub_helper.h"
#include "pubnub_log.h"
#else
#error this module can only be used if PUBNUB_USE_ADVANCED_HISTORY is defined and set to 1
#endif


#define PUBNUB_MIN_TIMETOKEN_LEN 4


enum pubnub_res pbcc_parse_message_counts_response(struct pbcc_context* p)
{
    enum pbjson_object_name_parse_result jpresult;
    struct pbjson_elem                   el;
    struct pbjson_elem                   found;
    char*                                reply = p->http_reply;
    int                                  replylen = p->http_buf_len;

    if ((reply[0] != '{') || (reply[replylen - 1] != '}')) {
        PUBNUB_LOG_ERROR("Error: pbcc_parse_message_counts_response(pbcc=%p) - "
                         "Response is not json object: response='%.*s'\n",
                         p,
                         replylen,
                         reply);
        return PNR_FORMAT_ERROR;
    }
    el.start = reply;
    el.end   = reply + replylen;
    if (pbjson_value_for_field_found(&el, "status", "403")){
        PUBNUB_LOG_ERROR("Error: pbcc_parse_message_counts_response(pbcc=%p) - "
                         "Access Denied: response='%.*s'\n",
                         p,
                         replylen,
                         reply);
        return PNR_ACCESS_DENIED;
    }
    jpresult = pbjson_get_object_value(&el, "error", &found);
    if (jonmpOK == jpresult) {
        if (pbjson_elem_equals_string(&found, "false")) {
            /* If found, object's 'end' field points to the first character
               behind its end */
            jpresult = pbjson_get_object_value(&el, "channels", &found);
            if (jonmpOK == jpresult) {
                p->msg_ofs = (found.start + 1) - reply;
                p->msg_end = (found.end - 1) - reply;
            }
            else {
                PUBNUB_LOG_ERROR("Error: pbcc_parse_message_counts_response(pbcc=%p) - "
                                 "No 'channels' array found, error=%d\n",
                                 p,
                                 jpresult);
                return PNR_FORMAT_ERROR;
            }
        }
        else {
            return PNR_ERROR_ON_SERVER;        
        }
    }
    else {
        PUBNUB_LOG_ERROR("Error: pbcc_parse_message_counts_response(pbcc=%p) - "
                         "'error' atribute not found in the response. error=%d\n"
                         "response='%.*s'\n",
                         p,
                         jpresult,
                         replylen,
                         reply);
        return PNR_FORMAT_ERROR;
    }
    p->chan_ofs = p->chan_end = 0;

    return PNR_OK;
}


int pbcc_get_error_message(struct pbcc_context* p, pubnub_chamebl_t* o_msg)
{
    enum pbjson_object_name_parse_result jpresult;
    struct pbjson_elem                   el;
    struct pbjson_elem                   found;
    int                                  rslt;

    el.start    = p->http_reply;
    el.end      = p->http_reply + p->http_buf_len;
    /* If found, object's 'end' field points to the first character behind its end(quotation marks) */
    jpresult = pbjson_get_object_value(&el, "error_message", &found);
    if (jonmpOK == jpresult) {
        o_msg->size = found.end - 2 - found.start;
        /* found.start, in this case, points to the quotation mark at the string beginning */
        o_msg->ptr = (char*)found.start + 1;
        rslt = 0;
    }
    else {
        PUBNUB_LOG_ERROR("Error: pubnub_get_error_message(pbcc=%p) - "
                         "No error message found. error=%d\n"
                         "response='%.*s'\n",
                         p,
                         jpresult,
                         (int)p->http_buf_len,
                         p->http_reply);
        rslt = -1;
    }
    return rslt;
}


int pbcc_get_chan_msg_counts_size(struct pbcc_context* p)
{
    char const* next;
    char const* end;
    int number_of_key_value_pairs = 0;

    end = p->http_reply + p->msg_end;
    next = p->http_reply + p->msg_ofs;
    if (next >= end) {
        return 0;
    }
    for (;next < end; next++) {
        if (',' == *next) {
            ++number_of_key_value_pairs;
        }
    }

    return ++number_of_key_value_pairs; 
}


int pbcc_get_chan_msg_counts(struct pbcc_context* p, 
                             size_t* io_count, 
                             struct pubnub_chan_msg_count* chan_msg_counters)
{
    char const*  ch_start;
    char const*  end;
    size_t count = 0;

    ch_start = p->http_reply + p->msg_ofs;
    end = p->http_reply + p->msg_end + 1;
    if (ch_start >= end - 1) {
        *io_count = 0;
        return 0;
    }
    ch_start = pbjson_skip_whitespace(ch_start, end);
    while ((ch_start < end) && (count < *io_count)) {
        char const* ch_end = pbjson_find_end_element(ch_start, end);
        
        chan_msg_counters[count].channel.size = ch_end - ch_start - 1;
        chan_msg_counters[count].channel.ptr = (char*)(ch_start + 1),
        ch_start = pbjson_skip_whitespace(ch_end + 1, end);
        if (*ch_start != ':') {
            PUBNUB_LOG_DEBUG("Note: pubnub_get_chan_msg_counts(pbcc=%p) - "
                             "colon missing after channel name=%*.s\n"
                             "characters after channel name='%s'\n",
                             p,
                             (int)chan_msg_counters[count].channel.size,
                             chan_msg_counters[count].channel.ptr,
                             ch_start);
        }
        else if (sscanf(++ch_start,
                        "%lu",
                        (unsigned long*)&(chan_msg_counters[count].message_count)) != 1) {
            PUBNUB_LOG_DEBUG("Note: pubnub_get_chan_msg_counts(pbcc=%p) - "
                             "failed to read the message count for channel='%*.s'\n"
                             "got these characters instead='%s'\n",
                             p,
                             (int)chan_msg_counters[count].channel.size,
                             chan_msg_counters[count].channel.ptr,                             
                             ch_start - 1);
        }
        else {
            ++count;
        }
        while((*ch_start != ',') && (ch_start < end)) {
            ch_start++;
        }
        if (ch_start == end) {
            break;
        }
        ch_start = pbjson_skip_whitespace(ch_start + 1, end);
    }
    if ((count == *io_count) && (ch_start != end)) {
        PUBNUB_LOG_DEBUG("Note: pubnub_get_chan_msg_counts(pbcc=%p) - "
                         "more than expected message counters,\n"
                         "unhandled part of the response='%s'\n",
                         p,
                         ch_start);
    }
    else {
        *io_count = count;
    }
    p->msg_ofs = p->msg_end;
    
    return 0;
}


static int initialize_msg_counters(char const* channel, int* o_count)
{
    int         n = 0;
    char const* next;
    
    for (next = channel; ' ' == *next; next++) continue;
    o_count[n++] = -(next - channel) - 1;    
    for (next = strchr(next, ','); next != NULL; next = strchr(next + 1, ','), n++) {
        for (++next; ' ' == *next; next++) continue ;
        /* Saving negative channel offsets(-1) in the array of counters.
           That is, if channel name from the 'channel' list is not found in the answer
           corresponding array member stays negative, while when channel name(from the
           'channel' list) is found in the response this value is used for locating
           channel name within the list od channels before it is changed to its
           corresponding message counter value(which could, also, be zero).
         */
        o_count[n] = -(next - channel) - 1;
    }
    return n;
}


int pbcc_get_message_counts(struct pbcc_context* p, char const* channel, int* o_count)
{
    char const* ch_start;
    char const* end;
    int         n = initialize_msg_counters(channel, o_count);
    int         counts = 0;

    ch_start = p->http_reply + p->msg_ofs;
    end = p->http_reply + p->msg_end + 1;
    if (ch_start >= end - 1) {
        /* Response message carries no counters */
        return 0;
    }
    ch_start = pbjson_skip_whitespace(ch_start, end);
    while((ch_start < end) && (counts < n)) {
        /* index in the array of message counters */
        int         i = 0;
        /* channel name length */
        unsigned    len;
        char const* ptr_ch = NULL;
        char const* ch_end;
        ch_end = (char*)pbjson_find_end_element(ch_start++, end);
        len = ch_end - ch_start;
        for (i = 0; i < n ; i++) {
            if (o_count[i] < 0) {
                ptr_ch = channel - o_count[i] - 1;
                /* Comparing channel name found in response message with name from 'channel' list. */
                if ((memcmp(ptr_ch, ch_start, len) == 0) &&
                    ((' ' == *(ptr_ch + len)) || (',' == *(ptr_ch + len)) || ('\0' == *(ptr_ch + len)))) {
                    break;
                }
            }
        }
        if (i == n) {
            PUBNUB_LOG_DEBUG("Note: pubnub_get_msg_counts(pbcc=%p) - "
                             "channel not present in the query list 'channel',\n"
                             "unhandled channel from the response='%*.s'\n",
                             p,
                             len,
                             ch_start);
        }
        ch_start = pbjson_skip_whitespace(ch_end + 1, end);
        if (*ch_start != ':') {
            PUBNUB_LOG_DEBUG("Note: pubnub_get_msg_counts(pbcc=%p) - "
                             "colon missing after channel name='%*.s'\n"
                             "characters after channel name='%s'",
                             p,
                             len,
                             ptr_ch,
                             ch_start);
        }    /* Saving message count value in the array provided */
        else if (sscanf(++ch_start, "%u", (unsigned*)(o_count + i)) != 1) {
            PUBNUB_LOG_DEBUG("Note: pubnub_get_msg_counts(pbcc=%p) - "
                             "failed to read the message count for channel='%*.s'\n"
                             "got these characters instead='%s'\n",
                             p,
                             len,
                             ptr_ch,
                             ch_start - 1);
        }
        else {
            ++counts;
        }
        while((*ch_start != ',') && (ch_start < end)) {
            ch_start++;
        }
        if (ch_start == end) {
            break;
        }
        ch_start = pbjson_skip_whitespace(ch_start + 1, end);
    }
    if ((counts == n) && (ch_start != end)) {
        PUBNUB_LOG_DEBUG("Note: pubnub_get_msg_counts(pbcc=%p) - "
                         "more than expected message counters,\n"
                         "unhandled part of the response='%s'\n",
                         p,
                         ch_start);
    }
    p->msg_ofs = p->msg_end;
    return 0;
}


static enum pubnub_parameter_error check_timetoken(struct pbcc_context* p,
                                                   char const* timetoken)
{
    size_t len = strlen(timetoken);
    if (len < PUBNUB_MIN_TIMETOKEN_LEN) {
        PUBNUB_LOG_ERROR("Error: message_counts_prep(pbcc=%p) - "
                         "Timetoken shorter than minimal permited.\n"
                         "PUBNUB_MIN_TIMETOKEN_LEN=%d\n"
                         "timetoken='%s'\n",
                         p,
                         PUBNUB_MIN_TIMETOKEN_LEN,
                         timetoken);
        return pnarg_INVALID_TIMETOKEN;
    }
    else if (strspn(timetoken, OK_SPAN_CHARACTERS) != len) {
        PUBNUB_LOG_ERROR("Error: message_counts_prep(pbcc=%p) - "
                         "Invalid timetoken.(timetoken='%s')\n",
                         p,
                         timetoken);
        return pnarg_INVALID_TIMETOKEN;
    }
    return pnarg_PARAMS_OK;
}


static enum pubnub_parameter_error check_channels(struct pbcc_context* p, char const* channels)
{
    size_t len;
    char const* previous_channel = channels;
    char const* next;
    for (next = strchr(previous_channel, ','); next != NULL;) {
        if ((next - previous_channel > PUBNUB_MAX_CHANNEL_NAME_LENGTH) ||
            (next - previous_channel < 1)) {
            PUBNUB_LOG_ERROR("Error: message_counts_prep(pbcc=%p) - "
                             "Channel name within the channel list too long(or missing):\n"
                             "PUBNUB_MAX_CHANNEL_NAME_LENGTH=%d\n"
                             "channel='%.*s'\n",
                             p,
                             PUBNUB_MAX_CHANNEL_NAME_LENGTH,
                             (int)(next - previous_channel),
                             previous_channel);
            return pnarg_INVALID_CHANNEL;
        }
        previous_channel = next + 1;
        next = strchr(previous_channel, ',');
    }
    len = strlen(previous_channel);
    if ((len > PUBNUB_MAX_CHANNEL_NAME_LENGTH) || (len < 1)) {
        PUBNUB_LOG_ERROR("Error: message_counts_prep(pbcc=%p) - "
                         "Channel name at the end of the channel list is too long(or missing):\n"
                         "PUBNUB_MAX_CHANNEL_NAME_LENGTH=%d\n"
                         "channel='%s'\n",
                         p,
                         PUBNUB_MAX_CHANNEL_NAME_LENGTH,
                         previous_channel);
        return pnarg_INVALID_CHANNEL;
    }
    return pnarg_PARAMS_OK;
}


static enum pubnub_parameter_error check_channel_timetokens(struct pbcc_context* p,
                                                            char const* channel,
                                                            char const* channel_timetokens)
{
    size_t len;
    char const* previous_channel = channel;
    char const* previous_timetoken = channel_timetokens;
    char const* next = strchr(previous_channel, ',');
    char const* next_within_timetokens = strchr(previous_timetoken, ',');
    while (next != NULL) {
        if (NULL == next_within_timetokens) {                
            PUBNUB_LOG_ERROR("Error: message_counts_prep(pbcc=%p) - Number of channels and "
                             "number of channel_timetokens don't match: "
                             "More channels than channel_timetokens.\n"
                             "channel='%s'\n"
                             "channel_timetokens='%s'\n",
                             p,
                             channel,
                             channel_timetokens);
            return pnarg_CHANNEL_TIMETOKEN_COUNT_MISMATCH;
        }
        if ((next - previous_channel > PUBNUB_MAX_CHANNEL_NAME_LENGTH) ||
            (next - previous_channel < 1)) {
            PUBNUB_LOG_ERROR("Error: message_counts_prep(pbcc=%p) - "
                             "Channel name within the channel list too long(or missing):\n"
                             "PUBNUB_MAX_CHANNEL_NAME_LENGTH=%d\n"
                             "channel='%.*s'\n",
                             p,
                             PUBNUB_MAX_CHANNEL_NAME_LENGTH,
                             (int)(next - previous_channel),
                             previous_channel);
            return pnarg_INVALID_CHANNEL;
        }
        if (next_within_timetokens - previous_timetoken < PUBNUB_MIN_TIMETOKEN_LEN) {
            PUBNUB_LOG_ERROR("Error: message_counts_prep(pbcc=%p) - "
                             "Timetoken within the list shorter than minimal permited.\n"
                             "PUBNUB_MIN_TIMETOKEN_LEN=%d\n"
                             "timetoken='%.*s'\n",
                             p,
                             PUBNUB_MIN_TIMETOKEN_LEN,
                             (int)(next_within_timetokens - previous_timetoken),
                             previous_timetoken);
            return pnarg_INVALID_TIMETOKEN;
        }
        previous_channel = next + 1;
        previous_timetoken = next_within_timetokens + 1;
        next = strchr(previous_channel, ',');
        next_within_timetokens = strchr(previous_timetoken, ',');
    }
    if (next_within_timetokens != NULL) {                
        PUBNUB_LOG_ERROR("Error: message_counts_prep(pbcc=%p) - Number of channels and "
                         "number of channel_timetokens don't match: "
                         "More channel_timetokens than channels.\n"
                         "channel='%s'\n"
                         "channel_timetokens='%s'\n",
                         p,
                         channel,
                         channel_timetokens);
        return pnarg_CHANNEL_TIMETOKEN_COUNT_MISMATCH;
    }
    len = strlen(previous_channel);
    if ((len > PUBNUB_MAX_CHANNEL_NAME_LENGTH) || (len < 1)) {
        PUBNUB_LOG_ERROR("Error: message_counts_prep(pbcc=%p) - "
                         "Channel name at the end of the channel list is too long(or missing):\n"
                         "PUBNUB_MAX_CHANNEL_NAME_LENGTH=%d\n"
                         "channel='%s'\n",
                         p,
                         PUBNUB_MAX_CHANNEL_NAME_LENGTH,
                         previous_channel);
        return pnarg_INVALID_CHANNEL;
    }
    if (strlen(previous_timetoken) < PUBNUB_MIN_TIMETOKEN_LEN) {
        PUBNUB_LOG_ERROR("Error: message_counts_prep(pbcc=%p) - "
                         "Timetoken at the end of the list is shorter than minimal permited.\n"
                         "PUBNUB_MIN_TIMETOKEN_LEN=%d\n"
                         "timetoken='%s'\n",
                         p,
                         PUBNUB_MIN_TIMETOKEN_LEN,
                         previous_timetoken);
        return pnarg_INVALID_TIMETOKEN;
    }
    if (strspn(channel_timetokens, OK_SPAN_CHARACTERS) != strlen(channel_timetokens)) {
        PUBNUB_LOG_ERROR("Error: message_counts_prep(pbcc=%p) - Invalid channel_timetokens."
                         "(channel_timetokens='%s')\n",
                         p,
                         channel_timetokens);
        return pnarg_INVALID_TIMETOKEN;
    }
    return pnarg_PARAMS_OK;
}

static enum pubnub_parameter_error check_parameters(struct pbcc_context* p,
                                                    char const* channel,
                                                    char const* timetoken,
                                                    char const* channel_timetokens)
{
    if (timetoken != NULL) {
        if (check_timetoken(p, timetoken) != pnarg_PARAMS_OK) {
            return pnarg_INVALID_TIMETOKEN;
        }
        else if (check_channels(p, channel) != pnarg_PARAMS_OK) {
            return pnarg_INVALID_CHANNEL;
        }
    }
    if (channel_timetokens != NULL) {
        enum pubnub_parameter_error rslt = check_channel_timetokens(p,
                                                                    channel,
                                                                    channel_timetokens);
        if (rslt != pnarg_PARAMS_OK) {
            return rslt;
        }
    }

    return pnarg_PARAMS_OK;
}


enum pubnub_res pbcc_message_counts_prep(
                                         enum pubnub_trans pt,
                                         struct pbcc_context* p,
                                         char const*          channel,
                                         char const*          timetoken,
                                         char const*          channel_timetokens)
{
    char const* const uname = pubnub_uname();
    char const*       user_id  = pbcc_user_id_get(p);
    enum pubnub_res   rslt = PNR_OK;

    PUBNUB_ASSERT_OPT(user_id != NULL);
    
    if (check_parameters(p, channel, timetoken, channel_timetokens) != pnarg_PARAMS_OK) {
        return PNR_INVALID_PARAMETERS;
    }
    if (p->msg_ofs < p->msg_end) {
        return PNR_RX_BUFF_NOT_EMPTY;
    }

    p->http_content_len = 0;
    p->msg_ofs = p->msg_end = 0;

    p->http_buf_len = snprintf(p->http_buf,
                               sizeof p->http_buf,
                               "/v3/history/sub-key/%s/message-counts/",
                               p->subscribe_key);
    APPEND_URL_ENCODED_M(p, channel);

    URL_PARAMS_INIT(qparam, PUBNUB_MAX_URL_PARAMS);
    if (uname) { ADD_URL_PARAM(qparam, pnsdk, uname); }
    if (user_id) { ADD_URL_PARAM(qparam, uuid, user_id); }
#if PUBNUB_CRYPTO_API
    if (p->secret_key == NULL) { ADD_URL_AUTH_PARAM(p, qparam, auth); }
    ADD_TS_TO_URL_PARAM();
#else
    ADD_URL_AUTH_PARAM(p, qparam, auth);
#endif
    if (timetoken) { ADD_URL_PARAM(qparam, timetoken, timetoken); }
    if (channel_timetokens) { ADD_URL_PARAM(qparam, channelsTimetoken, channel_timetokens); }

#if PUBNUB_CRYPTO_API
  SORT_URL_PARAMETERS(qparam);
#endif
  ENCODE_URL_PARAMS_TRANS(pt, p, qparam);
#if PUBNUB_CRYPTO_API
    if (p->secret_key != NULL) {
        rslt = pbcc_sign_url(p, "", pubnubSendViaGET, true);
        if (rslt != PNR_OK) {
            PUBNUB_LOG_ERROR("pbcc_sign_url failed. pb=%p, message=%s, method=%d", p, "", pubnubSendViaGET);
        }
    }
#endif

    PUBNUB_LOG_DEBUG("pbcc_message_counts_prep. REQUEST =%s\n", p->http_buf);
    return (rslt != PNR_OK) ? rslt : PNR_STARTED;
}

enum pubnub_res pbcc_delete_messages_prep(struct pbcc_context* pb,
                                          char const*          channel,
                                          char const*          start,
                                          char const*          end)
{
    char const* const uname = pubnub_uname();
    char const* user_id = pbcc_user_id_get(pb);
#if PUBNUB_CRYPTO_API
    enum pubnub_res rslt = PNR_OK;
#endif

    PUBNUB_ASSERT_OPT(NULL != user_id);

    pb->msg_ofs = pb->msg_end = 0;
    pb->http_content_len = 0;

    pb->http_buf_len =
        snprintf(pb->http_buf,
                 sizeof pb->http_buf,
                 "/v3/history/sub-key/%s/channel/",
                 pb->subscribe_key);
    APPEND_URL_ENCODED_M(pb, channel);

    URL_PARAMS_INIT(qparam, PUBNUB_MAX_URL_PARAMS);
    if (uname) { ADD_URL_PARAM(qparam, pnsdk, uname); }
    ADD_URL_PARAM(qparam, uuid, user_id);
#if PUBNUB_CRYPTO_API
    if (pb->secret_key == NULL) { ADD_URL_AUTH_PARAM(pb, qparam, auth); }
    ADD_TS_TO_URL_PARAM();
#else
    ADD_URL_AUTH_PARAM(pb, qparam, auth);
#endif
    if (start) { ADD_URL_PARAM(qparam, start, start); }
    if (end) { ADD_URL_PARAM(qparam, end, end); }

#if PUBNUB_CRYPTO_API
    SORT_URL_PARAMETERS(qparam);
#endif
    ENCODE_URL_PARAMETERS(pb, qparam);
#if PUBNUB_CRYPTO_API
    if (pb->secret_key != NULL) {
        rslt = pbcc_sign_url(pb, "", pubnubSendViaGET, true);
    }
#endif

    PUBNUB_LOG_DEBUG("pbcc_delete_messages_prep. REQUEST =%s\n", pb->http_buf);
#if PUBNUB_CRYPTO_API
    return (rslt != PNR_OK) ? rslt : PNR_STARTED;
#else
    return PNR_STARTED;
#endif
}

pubnub_chamebl_t pbcc_get_delete_messages_response(struct pbcc_context* pb)
{
    pubnub_chamebl_t resp;
    char const*      reply     = pb->http_reply;
    int              reply_len = pb->http_buf_len;

    if (PNR_OK != pb->last_result) {
        PUBNUB_LOG_ERROR("pbcc_get_delete_messages_response(pb=%p) can be "
                         "called only if previous transactin "
                         "PBTT_DELETE_MESSAGES(%d) is finished successfully. "
                         "Transaction result was: %d('%s')\n",
                         pb,
                         PBTT_DELETE_MESSAGES,
                         pb->last_result,
                         pubnub_res_2_string(pb->last_result));
        resp.ptr  = NULL;
        resp.size = 0;
        return resp;
    }

    resp.ptr  = (char*)reply;
    resp.size = reply_len;
    return resp;
}

enum pubnub_res pbcc_parse_delete_messages_response(struct pbcc_context* pb)
{
    enum pbjson_object_name_parse_result jpresult;
    struct pbjson_elem                   el;
    struct pbjson_elem                   found;
    char*                                reply     = pb->http_reply;
    int                                  reply_len = pb->http_buf_len;

    if ((reply[0] != '{') || (reply[reply_len - 1] != '}')) {
        PUBNUB_LOG_ERROR(
            "Error: pbcc_parse_delete_messages_response(pbcc=%p) - "
            "Response is not json object: response='%.*s'\n",
            pb,
            reply_len,
            reply);
        return PNR_FORMAT_ERROR;
    }
    el.start = reply;
    el.end   = reply + reply_len;
    if (pbjson_value_for_field_found(&el, "status", "403")) {
        PUBNUB_LOG_ERROR(
            "Error: pbcc_parse_delete_messages_response(pbcc=%p) - "
            "Access Denied: response='%.*s'\n",
            pb,
            reply_len,
            reply);
        return PNR_ACCESS_DENIED;
    }
    jpresult = pbjson_get_object_value(&el, "error", &found);
    if (jonmpOK == jpresult) {
        if (pbjson_elem_equals_string(&found, "true")) {
            return PNR_ERROR_ON_SERVER;
        }
    }
    else {
        PUBNUB_LOG_ERROR(
            "Error: pbcc_parse_delete_messages_response(pbcc=%p) - "
            "'error' atribute not found in the response. error=%d\n"
            "response='%.*s'\n",
            pb,
            jpresult,
            reply_len,
            reply);
        return PNR_FORMAT_ERROR;
    }
    pb->chan_ofs = pb->chan_end = 0;

    return PNR_OK;
}