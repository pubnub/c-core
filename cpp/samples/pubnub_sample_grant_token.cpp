/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#define PUBNUB_USE_GRANT_TOKEN_API 1
#include "pubnub.hpp"

#include <iostream>
#include <exception>
#include <stdio.h>
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
        gb.set_user_id("pandu-iz-uuidgra");

        std::cout << "Grant Token" << std::endl;
        struct pam_permission h_perm = { h_perm.read=true, h_perm.write=true };
        int perm_hello_world = pubnub_get_grant_bit_mask_value(h_perm);
        struct pam_permission cg_perm = {cg_perm.read=true, cg_perm.write=true, cg_perm.manage=true};
        int perm_channel_group = pubnub_get_grant_bit_mask_value(cg_perm);
        int ttl_minutes = 60;
        char perm_obj[2000];
        char* authorized_uuid = (char*)"my_authorized_uuid"; // TODO: @reviewers should we change uuid in perm_obj?
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
        
        pb.set_user_id("pandu-iz-uuidsam");
        pb.set_auth_token(tkn);

        pb.set_transaction_timeout(
#if __cplusplus >= 201103L
            std::chrono::seconds(10)
#else
            10000
#endif
            );

        std::cout << "First subscribe / connect" << std::endl;
        if (PNR_OK ==  pb.subscribe(chan).await()) {
            std::cout << "Subscribe/connected!" << std::endl;
        }
        else {
            std::cout << "Subscribe failed!" << std::endl;
        }

#if PUBNUB_CRYPTO_API
        std::cout << "Publishing encrypted" << std::endl;
        pubnub::futres futres = pb.publish_encrypted(chan, "\"Hello world from C++!\"", "KUKUMENEGRDNA");
#else
        std::cout << "Publishing" << std::endl;
        pubnub::futres futres = pb.publish(chan, "\"Hello world from C++!\"");
#endif
        res = futres.await();
        if (PNR_OK == res) {
            std::cout << "Published! Response from Pubnub: " << pb.last_publish_result() << std::endl;
        }
        else if (PNR_PUBLISH_FAILED == res) {
            std::cout << "Published failed on Pubnub, description: " << pb.last_publish_result() << std::endl;
        }
        else {
            std::cout << "Publishing failed with code: " << res << std::endl;
        }

        std::cout << "Subscribing" << std::endl;
        if (PNR_OK ==  pb.subscribe(chan).await()) {
            std::cout << "Subscribed! Got messages:" << std::endl;
            /// Lets illustrate getting all the message in a vector,
            /// and iterating over it
#if __cplusplus >= 201103L
#if PUBNUB_CRYPTO_API
            auto msg = pb.get_all_decrypted("KUKUMENEGRDNA");
#else
            auto msg = pb.get_all();
#endif
            for (auto it = msg.begin(); it != msg.end(); ++it) {
                std::cout << *it << std::endl;
            }
#else
#if PUBNUB_CRYPTO_API
            std::vector<std::string> msg = pb.get_all_decrypted("KUKUMENEGRDNA");
#else
            std::vector<std::string> msg = pb.get_all();
#endif
            for (std::vector<std::string>::iterator it = msg.begin(); it != msg.end(); ++it) {
                std::cout << *it << std::endl;
            }
#endif
        }
        else {
            std::cout << "Subscribe failed!" << std::endl;
        }

        std::cout << "Getting time" << std::endl;
        if (PNR_OK ==  pb.time().await()) {
            std::cout << "Gotten time " << pb.get() << "; last time token="<< pb.last_time_token() << std::endl;
        }
        else {
            std::cout << "Getting time failed!" << std::endl;
        }

        std::cout << "Getting history" << std::endl;
        res = pb.history(chan).await();
        if (PNR_OK ==  res) {
            std::cout << "Got history! Messages:" << std::endl;
            /// Lets illustrate getting all the message in a vector,
            /// and then accessing each vector index in a loop
#if __cplusplus >= 201103L
            auto msg = pb.get_all();
            /// a for-each loop for C++11
            for (auto &&m : msg) {
                std::cout << m << std::endl;
            }
#else
            std::vector<std::string> msg = pb.get_all();
            for (unsigned i = 0; i < msg.size(); ++i) {
                std::cout << msg.at(i) << std::endl;
            }
#endif
        }
        else {
            std::cout << "Getting history failed! error code: " << res << std::endl;
        }

        std::cout << "Getting history with `include_token`" << std::endl;
        if (PNR_OK ==  pb.history(chan, 10, true).await()) {
            std::cout << "Got history with time token! Messages:" << std::endl;
            /// Here we illustrate getting the messages one-by-one
            std::string msg;
            do {
                msg = pb.get();
                std::cout << msg << std::endl;
            } while (!msg.empty());
        }
        else {
            std::cout << "Getting history with time token failed!" << std::endl;
        }

        std::cout << "Getting here-now presence" << std::endl;
        if (PNR_OK ==  pb.here_now(chan).await()) {
            std::cout << "Got here-now presence: " << pb.get() << std::endl;
        }
        else {
            std::cout << "Getting here-now presence failed!" << std::endl;
        }

        /** Global here_now presence for "demo" subscribe key is _very_
            long, so we disable it. Enable to try out, or if you use
            a "real" subscribe key*/
#if 0
        std::cout << "Getting global here-now presence" << std::endl;
        if (PNR_OK ==  pb.global_here_now().await()) {
            std::cout << "Got global here-now presence: " << pb.get() << std::endl;
            }
        }
        else {
            std::cout << "Getting here-now presence failed!" << std::endl;
        }
#endif

        std::cout << "Getting where-now presence" << std::endl;
        if (PNR_OK ==  pb.where_now().await()) {
            std::cout << "Got where-now presence: " << pb.get() << std::endl;
        }
        else {
            std::cout << "Getting where-now presence failed!" << std::endl;
        }

        std::cout << "Setting state" << std::endl;
        if (PNR_OK ==  pb.set_state(chan, "", pb.user_id(), "{\"x\":5}").await()) {
            std::cout << "State was set: " << pb.get() << std::endl;
        }
        else {
            std::cout << "Setting state failed!" << std::endl;
        }

        std::cout << "Getting state" << std::endl;
        if (PNR_OK ==  pb.state_get(chan).await()) {
            std::cout << "State gotten: " << pb.get() << std::endl;
        }
        else {
            std::cout << "Getting state failed!" << std::endl;
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
