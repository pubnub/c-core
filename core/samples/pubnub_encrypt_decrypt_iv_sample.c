/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_sync.h"

#include "core/pubnub_helper.h"
#include "core/pubnub_timers.h"
#include "core/pubnub_crypto.h"
#include "core/pubnub_json_parse.h"

#include <stdio.h>
#include <time.h>
#include <string.h>

static void generate_user_id(pubnub_t* pbp)
{ 
    char const*                      user_id_default = "zeka-peka-iz-jendeka";
    struct Pubnub_UUID               uuid;
    static struct Pubnub_UUID_String str_uuid;

    if (0 != pubnub_generate_uuid_v4_random(&uuid)) {
        pubnub_set_user_id(pbp, user_id_default);
    }
    else {
        str_uuid = pubnub_uuid_to_string(&uuid);
        pubnub_set_user_id(pbp, str_uuid.uuid);
        printf("Generated UUID: %s\n", str_uuid.uuid);
    }
}


static void sync_sample_free(pubnub_t* p)
{
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


int main()
{
    time_t          t0;
    char const*     msg;
    enum pubnub_res res;
    char const*     chan = "hello_world";
    char const*     cipher_key = "enigma";

    /*  pubnub_encrypt   */
    char noserializemsg_encrypted_str[256];
    size_t noserializemsg_size = 256;
    char noserialize_msg[7] = "yay!";
    printf("Message to be encrypted = %s\n",noserialize_msg);
    pubnub_bymebl_t no_serialize_data = { (uint8_t*)noserialize_msg, (size_t)strlen(noserialize_msg) };
    if (0 != pubnub_encrypt(cipher_key, no_serialize_data, noserializemsg_encrypted_str, &noserializemsg_size)) {
        printf("pubnub_encrypt failed! %s %d", __FILE__, __LINE__);
        return -1;
    }
    
    printf("base64 message = %s (size = %d) \n", noserializemsg_encrypted_str, (int)noserializemsg_size);
    printf("\n ***************************************************** \n");

    /*  pubnub_decrypt   */
    pubnub_bymebl_t noserializemsg_deencrypted = { NULL, 2000 };
    noserializemsg_deencrypted.ptr = (uint8_t*)malloc(2000);
    if (0 != pubnub_decrypt(cipher_key, noserializemsg_encrypted_str, &noserializemsg_deencrypted)){
        printf("error pubnub_decrypt %s %d\n", __FILE__, __LINE__);
    }
    if (NULL == (&noserializemsg_deencrypted)->ptr){
        printf("Decrypt failed for = %s\n", noserializemsg_encrypted_str);    
    }
    else{
        printf("Decrypt of previous encrypted message = %s (size = %d)\n", (&noserializemsg_deencrypted)->ptr, (int)(&noserializemsg_deencrypted)->size);
    }
    printf("\n ***************************************************** \n");

    /*  pubnub_encrypt_buffered   */
    char msg_encrypted_str[256];
    size_t msg_size = 256;
    char msg1[5] = "yay!";
    pubnub_bymebl_t msg_data = { (uint8_t*)msg1, (size_t)strlen(msg1) };
    pubnub_bymebl_t buff_data = { (uint8_t*)malloc(2000), 2000};
    if (0 != pubnub_encrypt_buffered(cipher_key, msg_data, msg_encrypted_str, &msg_size, buff_data)) {
        printf("pubnub_encrypt_buffered failed! %s %d", __FILE__, __LINE__);
        return -1;
    }

    printf("base64 message = %s (size = %d) \n", msg_encrypted_str, (int)msg_size);
    printf("\n ***************************************************** \n");

    /*  pubnub_decrypt_buffered   */
    pubnub_bymebl_t msg_deencrypted = { (uint8_t*)malloc(2000), 2000 };
    pubnub_bymebl_t msg_buff = { (uint8_t*)malloc(2000), 2000 };
    if (0 != pubnub_decrypt_buffered(cipher_key, msg_encrypted_str, &msg_deencrypted, &msg_buff)) {
        printf("error pubnub_decrypt %s %d\n", __FILE__, __LINE__);
    }
    if (NULL == (&msg_deencrypted)->ptr) {
        printf("Decrypt failed for = %s\n", msg_encrypted_str);
    }
    else {
        printf("Decrypt of previous encrypted message = %s (size = %d)\n", (&msg_deencrypted)->ptr, (int)(&msg_deencrypted)->size);
    }
    printf("\n ***************************************************** \n");

    no_serialize_data.ptr = NULL;
    (&noserializemsg_deencrypted)->ptr = NULL;
    noserializemsg_encrypted_str[0] = '\0';

    printf("\n");


    pubnub_t*       pbp  = pubnub_alloc();

    if (NULL == pbp) {
        printf("Failed to allocate Pubnub context!\n");
        return -1;
    }
    pubnub_init(pbp, "demo", "demo");

    pubnub_set_transaction_timeout(pbp, PUBNUB_DEFAULT_SUBSCRIBE_TIMEOUT);

    /* Leave this commented out to use the default - which is
       blocking I/O on most platforms. Uncomment to use non-
       blocking I/O.
    */
    pubnub_set_non_blocking_io(pbp);

    generate_user_id(pbp);
 
    pubnub_set_auth(pbp, "my-uuid-pandu");


    puts("Subscribing...");
    time(&t0);
    res = pubnub_subscribe(pbp, chan, NULL);
    if (PNR_STARTED == res) {
        res = pubnub_await(pbp);
    }
    printf("Subscribe/connect lasted %lf seconds.\n", difftime(time(NULL), t0));
    if (PNR_OK == res) {
        puts("Subscribed!");
    }
    else {
        printf("Subscribing failed with code: %d('%s')\n",
               res,
               pubnub_res_2_string(res));
    }


    puts("Publishing...");
    time(&t0);
    res = pubnub_publish_encrypted(pbp, chan, "\"yay!\"", cipher_key);
    if (PNR_STARTED == res) {
        res = pubnub_await(pbp);
    }
    printf("Publish lasted %lf seconds.\n", difftime(time(NULL), t0));
    if (PNR_OK == res) {
        printf("Published! Response from Pubnub: %s\n",
               pubnub_last_publish_result(pbp));
    }
    else if (PNR_PUBLISH_FAILED == res) {
        printf("Published failed on Pubnub, description: %s\n",
               pubnub_last_publish_result(pbp));
    }
    else {
        printf("Publishing failed with code: %d('%s')\n",
               res,
               pubnub_res_2_string(res));
    }


    time(&t0);
    res = pubnub_subscribe(pbp, chan, NULL);
    if (PNR_STARTED == res) {
        res = pubnub_await(pbp);
    }
    printf("Subscribe lasted %lf seconds.\n", difftime(time(NULL), t0));
    if (PNR_OK == res) {
        puts("Subscribed! Got messages:");
        for (;;) {
            pubnub_bymebl_t msg = pubnub_get_decrypted_alloc(pbp, cipher_key);
            if (msg.ptr != NULL){
                puts((char *)msg.ptr);
                if (0 == strcmp((char *)msg.ptr,"\"yay!\"")){
                    puts("Received published message. Hurray!");
                    break;
                }
            }
        }
    }
    else {
        printf("Subscribing failed with code: %d('%s')\n",
               res,
               pubnub_res_2_string(res));
    }


    puts("Getting history with include_token...");
    res = pubnub_history(pbp, chan, 1, true);
    if (PNR_STARTED == res) {
        res = pubnub_await(pbp);
    }
    if (PNR_OK == res) {
        puts("Got history! Elements:");
        msg = pubnub_get(pbp);
        if (NULL != msg) {
            puts(msg);
            struct pbjson_elem el;
            struct pbjson_elem found;
            el.start = msg+1;
            el.end = msg + strlen(msg);
            if (jonmpOK == pbjson_get_object_value(&el, "message", &found)){
                char msg_val[256];
                snprintf(msg_val, found.end - found.start-1,"%s", (char*)found.start+1);
                pubnub_bymebl_t msg = pubnub_decrypt_alloc(cipher_key, msg_val);
                if (msg.ptr != NULL){
                    if (0 == strcmp((char *)msg.ptr,"\"yay!\"")){
                        printf("Decryption gave %s in History. Hurray!\n", msg.ptr);
                    }
                }
            }
        }
    }
    else {
        printf("Getting history v2 with include_token failed with code: "
               "%d('%s')\n",
               res,
               pubnub_res_2_string(res));
    }
    
    /* We're done, but, keep-alive might be on, so we need to cancel,
     * then free */
    sync_sample_free(pbp);

    puts("Pubnub encrypt decrypt with IV demo over.");

    return 0;
}
