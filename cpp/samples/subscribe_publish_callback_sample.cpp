/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub.hpp"

#include <iostream>

/** This example, while using the C++ API which is "generic" will
    actually work only with the Pubnub C callback "back-end" (notification
    interface). 

    To do the same thing with the "sync back-end", the user should use
    the pubnub::futres::last_result() to check for the outcome of both
    operations that are started concurrently.
 */


int main()
{
    try {
        enum pubnub_res res;
        char const *chan = "hello_world";
        pubnub::context pb("demo", "demo");
        pubnub::context pb_2("demo", "demo");

        std::cout << "--------------------------" << std::endl <<
            "Subscribing..." << std::endl <<
            "--------------------------";
	
        /* First subscribe, to get the time token */
        res = pb.subscribe(chan).await();
        if (PNR_STARTED == res) {
            std::cout << "await() returned unexpected: PNR_STARTED" << std::endl;
            return -1;
        }
        if (PNR_OK == res) {
            std::cout << "Subscribed!" << std::endl;
        }
        else {
            std::cout << "Subscribing failed with code: " << (int)res << std::endl;;
        }

        /* The "real" subscribe, with the just acquired time token */
        pubnub::futres futres = pb.subscribe(chan);
	
        /* Don't do "full" await here, because we didn't publish
         * anything yet! */
        futres.start_await();
    
        std::cout << "--------------------------" << std::endl <<
            "Publishing..." << std::endl <<
            "--------------------------";

        /* Since the subscribe is ongoing in the `pb` context, we
           can't publish on it, so we use a different context to
           publish
        */
        pubnub::futres fr_2 = pb_2.publish(chan, "\"Hello world from subscribe-publish callback sample!\"");
        
        std::cout << "Await publish" << std::endl;
        res = fr_2.await();
        if (res == PNR_STARTED) {
            std::cout << "await() returned unexpected: PNR_STARTED" << std::endl;
            return -1;
        }
        if (PNR_OK == res) {
            std::cout << "Published! Response from Pubnub: " << pb_2.last_publish_result() << std::endl;
        }
        else if (PNR_PUBLISH_FAILED == res) {
            std::cout << "Published failed on Pubnub, description: " << pb_2.last_publish_result() << std::endl;
        }
        else {
            std::cout << "Publishing failed with code: " << (int)res << std::endl;
        }
	
        /* Now we await the subscribe on `pbp` */
        std::cout << "Await subscribe" << std::endl;;
        res = futres.end_await();
        if (PNR_OK == res) {
            std::cout << "Subscribed! Got messages:";
            std::string msg;
            do {
                msg = pb.get();
                std::cout << msg << std::endl;
            } while (!msg.empty());
        }
        else {
            std::cout << "Subscribing failed with code: " << (int)res << std::endl;
        }
    }
    catch (std::exception &exc) {
        std::cout << "Caught exception: " << exc.what() << std::endl;
    }
	
    /* We're done */
    std::cout << "Pubnub subscribe-publish callback demo over." << std::endl;

    return 0;
}
