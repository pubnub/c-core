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

#if __cplusplus < 201103L
static int handle_msg(std::string msg, pubnub::context&, pubnub_res res) 
{
    if (PNR_OK != res) {
        std::cout << "Getting message failed with code: " << res;
        return -1;
    }
    std::cout << "Got message: \n" << msg << std::endl;
    
    return 0;
}
#endif


int main()
{
    try {
        std::string chan("hello_world");
        pubnub::context pb("demo", "demo");

        pb.set_user_id("zeka-peka-iz-jendeka");
        pb.set_transaction_timeout(
#if __cplusplus >= 201103L
            std::chrono::seconds(10)
#else
            10000
#endif
            );

        pubnub::subloop sublup(pb, chan);

//! [Fetch - pull interface]
        std::string msg;
        for (;;) {
            enum pubnub_res res = sublup.fetch(msg);
            if (res != PNR_OK) {
                std::cout << "Fetching message failed with code: " << res;
                break;
            }
            std::cout << "Got message: \n" << msg << std::endl;
        }
//! [Fetch - pull interface]

//! [Loop - push interface]
#if __cplusplus >= 201103L
        sublup.loop([](std::string msg, pubnub::context&, pubnub_res res) {
                if (PNR_OK != res) {
                    std::cout << "Getting message failed with code: " << res;
                    return -1;
                }
                std::cout << "Got message: \n" << msg << std::endl;

                return 0;
            });
#else
        sublup.loop(handle_msg);
#endif
//! [Loop - push interface]
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
    " subscribe loop demo over." << std::endl;

    return 0;
}
