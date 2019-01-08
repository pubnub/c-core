/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pnc_ops_sync.h"
#include "pnc_config.h"
#include "pnc_subscriptions.h"

#include "core/pubnub_helper.h"

#include <stdio.h>
#include <stdbool.h>
#include <string.h>


void pnc_ops_init(pubnub_t *pn, pubnub_t *pn_sub)
{
    // Nothing todo in sync version
}

void pnc_free(pubnub_t* p) {
    if (PN_CANCEL_STARTED == pubnub_cancel(p)) {
        enum pubnub_res pnru = pubnub_await(p);
        if (pnru != PNR_OK) {
            printf("Awaiting cancel failed: %d('%s')\n",
                   pnru,
                   pubnub_res_2_string(pnru));
        }
    }
    if (pubnub_free(p) != 0) {
        printf("Failed to free the Pubnub context\n");
    }
}

void pnc_ops_subscribe(pubnub_t *pn_sub)
{
    int i;
    enum pubnub_res res;
    char channels_string[PNC_SUBSCRIBE_CHANNELS_LIMIT * PNC_CHANNEL_NAME_SIZE];
    char groups_string[PNC_SUBSCRIBE_CHANNELS_LIMIT * PNC_CHANNEL_NAME_SIZE];

    puts("Subscribe will show the first message and return to select operation loop.");
    
    pnc_subscribe_list_channels(channels_string, sizeof channels_string);
    pnc_subscribe_list_channel_groups(groups_string, sizeof groups_string);
    
    
    for (i = 0; i < 2; i++) {
        puts("Subscribe loop...");
        
        if (strlen(channels_string) == 0 && strlen(groups_string) == 0) {
            puts("You need to add some channels or channel groups first. Ignoring");
            return;
        }
        else if (strlen(groups_string) == 0) {
            res = pubnub_subscribe(pn_sub, channels_string, NULL);
        } 
        else if (strlen(channels_string) == 0) {
            res = pubnub_subscribe(pn_sub, NULL, groups_string);
        } 
        else {
            res = pubnub_subscribe(pn_sub, channels_string, groups_string);
        }
        
        pnc_ops_parse_response("pubnub_subscribe()", res, pn_sub);
    }
}


void pnc_ops_unsubscribe(pubnub_t *pn)
{
    puts("Not needed in sync version");
}


void pnc_ops_parse_response(const char *method_name, enum pubnub_res res, pubnub_t *pn)
{
    const char *msg;
    
    if (res != PNR_STARTED) {
        printf("%s returned error %d('%s')\n", method_name, res, pubnub_res_2_string(res));
        return;
    }
    
    res = pubnub_await(pn);
    
    if (res == PNR_HTTP_ERROR) {
        printf("%s HTTP error / code: %d\n",
               method_name, pubnub_last_http_code(pn));
        return;
    }
    
    if (res == PNR_STARTED) {
        printf("%s returned unexpected: PNR_STARTED(%d)\n", method_name, res);
        return;
    }
    
    if (PNR_OK == res) {
        puts("\n****************************************");
        printf("Result for %s: success!\n", method_name);
        puts("****************************************");
        for (;;) {
            msg = pubnub_get(pn);
            if (NULL == msg) {
                break;
            }
            puts(msg);
        }
        puts("****************************************");
    }
    else {
        char const *desc = pubnub_last_publish_result(pn);
        printf("%s failed! error code %d('%s')\n", method_name, res, pubnub_res_2_string(res));
        if (desc[0] != '\0') {
            printf("Publish failed, with error: '%s' -> %d\n", desc, pubnub_parse_publish_result(desc));
        }
    }
}
