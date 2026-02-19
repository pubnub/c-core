#include "core/pubnub_ccore_pubsub.h"
#include "pubnub_internal.h"
#if PUBNUB_USE_LOGGER
#include "pbcc_logger_manager.h"
#endif // PUBNUB_USE_LOGGER
#include <string.h>
#include <stdlib.h>

static int json_kvp_builder(char* jsonbuilder, int pos, char* key, char* val)
{
    int dq_len  = strlen("\"");
    int co_len  = strlen(":");
    int key_len = strlen(key);
    int val_len = strlen(val);

    memcpy(jsonbuilder + pos, "\"", dq_len);
    pos += dq_len;
    memcpy(jsonbuilder + pos, key, key_len);
    pos += key_len;
    memcpy(jsonbuilder + pos, "\"", dq_len);
    pos += dq_len;
    memcpy(jsonbuilder + pos, ":", co_len);
    pos += co_len;
    memcpy(jsonbuilder + pos, val, val_len);
    pos += val_len;
    return pos;
}


void pbcc_adjust_state(
    struct pbcc_context* core,
    char const*          channel,
    char const*          channel_group,
    char const*          state)
{
    int ch_cnt = 0, cg_cnt = 0, tot_ch = 0, tot_cg = 0;
    if (channel) {
        tot_ch = 1;
        for (int i = 0; i < (int)strlen(channel); i++) {
            tot_ch = (channel[i] == ',') ? tot_ch + 1 : tot_ch;
        }
    }
    if (channel_group) {
        tot_cg = 1;
        for (int i = 0; i < (int)strlen(channel_group); i++) {
            tot_cg = (channel_group[i] == ',') ? tot_cg + 1 : tot_cg;
        }
    }

    int dq_len       = strlen("\"");
    int co_len       = strlen(":");
    int cm_len       = strlen(",");
    int ch_keys_len  = 0;
    int chg_keys_len = 0;

    if (tot_ch >= 1) {
        // commas length included in strlen(channel)
        // tot_ch * (dq_len * 2) - double-quotes around channel name
        // (tot_ch - 1) * (co_len + cm_len) - how many commas and colons
        // we need for channels key-value pairs
        ch_keys_len = strlen(channel) + tot_ch * (dq_len * 2 + co_len) +
                      ((tot_ch - 1) * cm_len);
    }

    if (tot_cg >= 1) {
        // commas length included in strlen(channel_group)
        // tot_cg * (dq_len * 2) - double-quotes around channel group name
        // (tot_cg - 1) * (co_len + cm_len) - how many commas and colons
        // we need for channel groups key-value pairs
        chg_keys_len = strlen(channel_group) + tot_cg * (dq_len * 2 + co_len) +
                       ((tot_cg - 1) * cm_len);
    }

    // 2 is our {} and 1 is our \0
    int buff_size =
        (tot_ch + tot_cg) * strlen(state) + ch_keys_len + chg_keys_len + 2 + 1;
    if (core->state != NULL && buff_size > strlen(core->state)) {
        core->state = (char*)realloc((char*)core->state, buff_size);
    }
    else if (core->state == NULL) {
        core->state = (char*)malloc(buff_size);
    }
    char* json_state = (char*)core->state;
    int   mem_len    = 0;
    if (json_state != NULL && core->state != NULL) {
        mem_len = strlen("{");
        memcpy(json_state, "{", mem_len);
        int cm_len = strlen(",");
        if (channel && strncmp(channel, (char*)",", 1) != 0) {
            char*  str_ch = (char*)channel;
            char*  ch_temp;
            size_t ch_len;
            bool   end = false;
            do {
                ch_temp = strchr(str_ch, ',');
                if (ch_cnt > 0 && json_state[mem_len - 1] != ',') {
                    memcpy(json_state + mem_len, ",", cm_len);
                    mem_len += cm_len;
                }
                if (ch_temp == NULL) {
                    end    = true;
                    ch_len = strlen(str_ch);
                }
                else {
                    ch_len = ch_temp - str_ch;
                }

                if (ch_len != 0) {
                    char* curr_ch = (char*)malloc(ch_len + 1);
                    memcpy(curr_ch, str_ch, ch_len);
                    curr_ch[ch_len] = '\0';

                    mem_len = json_kvp_builder(
                        json_state, mem_len, curr_ch, (char*)state);

                    free(curr_ch);
                }

                ch_cnt++;
                str_ch = ch_temp + 1;
            } while (false == end);
        }

        if (channel_group) {
            char* str_cg = (char*)channel_group;
            char* cg_temp;
            int   cg_len;
            bool  end = false;
            do {
                cg_temp = strchr(str_cg, ',');
                if ((ch_cnt > 0 || cg_cnt > 0) &&
                    json_state[mem_len - 1] != ',') {
                    memcpy(json_state + mem_len, ",", cm_len);
                    mem_len += cm_len;
                }
                if (cg_temp == NULL) {
                    end    = true;
                    cg_len = strlen(str_cg);
                }
                else {
                    cg_len = cg_temp - str_cg;
                }

                if (cg_len != 0) {
                    char* curr_cg = (char*)malloc(cg_len + 1);
                    memcpy(curr_cg, str_cg, cg_len);
                    curr_cg[cg_len] = '\0';

                    mem_len = json_kvp_builder(
                        json_state, mem_len, curr_cg, (char*)state);

                    free(curr_cg);
                }

                cg_cnt++;
                str_cg = cg_temp + 1;
            } while (false == end);
        }

        int cb_len     = strlen("}");
        int trim_comma = json_state[mem_len - 1] == ',' ? strlen(",") : 0;
        memcpy(json_state + mem_len - trim_comma, "}", cb_len);
        mem_len += cb_len - trim_comma;
        json_state[mem_len] = '\0';
        PBCC_LOG_DEBUG(
            core->logger_manager, "Formatted presence state: %s", json_state);
    }
}
