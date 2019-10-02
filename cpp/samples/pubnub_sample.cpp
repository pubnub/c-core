/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub.hpp"

#include <iostream>
#include <exception>


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
        std::string chan("hello_world");
        pubnub::context pb("demo", "demo");
 
        /* Leave this commented out to use the default - which is
           blocking I/O on most platforms. Uncomment to use non-
           blocking I/O.
        */
        pb.set_blocking_io(pubnub::non_blocking);
        
        if (0 != pb.set_uuid_v4_random()) {
            pb.set_uuid("zeka-peka-iz-jendeka");
        }
        else {
            std::cout << "Generated UUID: " << pb.uuid() << std::endl;
        }
        pb.set_auth("danaske");

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
        if (PNR_OK ==  pb.set_state(chan, "", pb.uuid(), "{\"x\":5}").await()) {
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
