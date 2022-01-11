/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#define PUBNUB_USE_GRANT_TOKEN_API 1
#include "pubnub.hpp"

#include <iostream>
#include <exception>
#include <stdio.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include "core/pubnub_grant_token_api.h"

/* Please note that this sample is the same whether you use the Pubnub
   C "sync" or "callback" interface during the build.

   Also, code that assumes C++ 11 features is protected by the
   standard "feature test" macro, hopefully your compiler has
   implemented it correctly.
 */

int main()
{
    try {
        enum pubnub_res res;
        char* my_env_publish_key = getenv("PUBNUB_PUBLISH_KEY");
        char* my_env_subscribe_key = getenv("PUBNUB_SUBSCRIBE_KEY");
        char* my_env_secret_key = getenv("PUBNUB_SECRET_KEY");

        if (NULL == my_env_publish_key) { my_env_publish_key = (char*)"demo"; }
        if (NULL == my_env_subscribe_key) { my_env_subscribe_key = (char*)"demo"; }
        if (NULL == my_env_secret_key) { my_env_secret_key = (char*)"demo"; }

        printf("%s\n%s\n%s\n",my_env_publish_key,my_env_subscribe_key,my_env_secret_key);

        std::string chan("hello_world");
        pubnub::context gb(my_env_publish_key, my_env_subscribe_key);
        gb.set_secret_key(my_env_secret_key);
        gb.set_blocking_io(pubnub::non_blocking);
        if (0 != gb.set_uuid_v4_random()) {
            gb.set_uuid("pandu-iz-uuidgra");
        }
        else {
            std::cout << "Grant Generated UUID: " << gb.uuid() << std::endl;
        }
        std::cout << "Grant Token" << std::endl;
        struct pam_permission h_perm = { h_perm.read=true, h_perm.write=true };
        int perm_hello_world = pubnub_get_grant_bit_mask_value(h_perm);
        struct pam_permission cg_perm = {cg_perm.read=true, cg_perm.write=true, cg_perm.manage=true};
        int perm_channel_group = pubnub_get_grant_bit_mask_value(cg_perm);
        int ttl_minutes = 60;
        char perm_obj[2000];
        char* authorized_uuid = "my_authorized_uuid";
        sprintf(perm_obj,"{\"ttl\":%d, \"uuid\":\"%s\", \"permissions\":{\"resources\":{\"channels\":{ \"mych\":31, \"hello_world\":%d }, \"groups\":{ \"mycg\":31, \"channel-group\":%d }, \"users\":{ \"myuser\":31 }, \"spaces\":{ \"myspc\":31 }}, \"patterns\":{\"channels\":{ }, \"groups\":{ }, \"users\":{ \"^$\":1 }, \"spaces\":{ \"^$\":1 }},\"meta\":{ }}}", ttl_minutes, authorized_uuid, perm_hello_world, perm_channel_group);
        pubnub::futres futgres = gb.grant_token(perm_obj);
        res = futgres.await();
        std::string tkn = "";
        if (PNR_OK == res) {
            tkn = gb.get_grant_token();
            std::cout << "Grant Token done! token = " << tkn << std::endl;

            std::string cbor_data = gb.parse_token(tkn);
            std::cout << "pubnub_parse_token() = " << cbor_data << std::endl;
        }
        else {
            std::cout << "Grant Token failed with code: " << res << std::endl;
            return 0;
        }

        pubnub::context pb(my_env_publish_key, my_env_subscribe_key);
 
        /* Leave this commented out to use the default - which is
           blocking I/O on most platforms. Uncomment to use non-
           blocking I/O.
        */
        pb.set_blocking_io(pubnub::non_blocking);
        
        if (0 != pb.set_uuid_v4_random()) {
            pb.set_uuid("pandu-iz-uuidsam");
        }
        else {
            std::cout << "Generated UUID: " << pb.uuid() << std::endl;
        }
        pb.set_auth_token(tkn);

        pb.set_transaction_timeout(
#if __cplusplus >= 201103L
            std::chrono::seconds(10)
#else
            10000
#endif
            );

#if PUBNUB_CRYPTO_API
        std::cout << "Publishing encrypted" << std::endl;
        pubnub::futres futres1 = pb.publish_encrypted(chan, "\"Hello world from C++!\"", "KUKUMENEGRDNA");
#else
        std::cout << "Publishing" << std::endl;
        pubnub::futres futres1 = pb.publish(chan, "\"Hello world from C++!\"");
#endif
        res = futres1.await();
        if (PNR_OK == res) {
            std::cout << "Published! Response from Pubnub: " << pb.last_publish_result() << std::endl;
        }
        else if (PNR_PUBLISH_FAILED == res) {
            std::cout << "Published failed on Pubnub, description: " << pb.last_publish_result() << std::endl;
        }
        else {
            std::cout << "Publishing failed with code: " << res << std::endl;
        }

        std::cout << "Revoke token" << std::endl;
        res = gb.revoke_token(tkn).await();
        if (PNR_OK == res) {
            std::cout << "Access token permissions revoked! Response from Pubnub: " << gb.get_revoke_token_result() << std::endl;
        }
        else {
            std::cout << "revoke_token() failed with code: " << res << std::endl;
            return 0;
        }

#ifdef _WIN32
        Sleep(70000); //70 seconds
#else
        sleep(70); //70 seconds
#endif

#if PUBNUB_CRYPTO_API
        std::cout << "Publishing encrypted after revoke" << std::endl;
        pubnub::futres futres2 = pb.publish_encrypted(chan, "\"Hello world from C++!\"", "KUKUMENEGRDNA");
#else
        std::cout << "Publishing after revoke" << std::endl;
        pubnub::futres futres2 = pb.publish(chan, "\"Hello world from C++!\"");
#endif
        res = futres2.await();
        if (PNR_OK == res) {
            std::cout << "Unexpected publish success (request should fail)! Response from Pubnub: " << pb.last_publish_result() << std::endl;
        }
        else if (PNR_PUBLISH_FAILED == res) {
            std::cout << "Expected publish failure on Pubnub as expected, description: " << pb.last_publish_result() << std::endl;
        }
        else {
            std::cout << "Expected publish failure with code: " << res << std::endl;
        }
    }
    catch (std::exception &exc) {
        std::cout << "Caught exception: " << exc.what() << std::endl;
    }

std::cout << "Pubnub C++ " <<
#if defined(PUBNUB_CALLBACK_API)
    "callback" <<
#else
    "sync" <<
#endif
    " demo over." << std::endl;

    return 0;
}
