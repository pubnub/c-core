/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub.hpp"


#include <iostream>

#include <ctime>
#include <cstdlib>


/** This example, while using the C++ API which is "generic" will
    surely work only with the Pubnub C sync "back-end" (notification
    interface). Whether it works with the "callback" interface depends
    on a particular implementation of that interface.
 */

int main()
{
    try {
        /* This is a widely use channel, something should happen there
           from time to time
        */
        std::string chan = "hello_world";
        pubnub::context pb("demo", "demo");

        srand((unsigned)time(NULL));

        /* This is essential, as otherwise waiting for incoming data will
           block! Since we're doing this, be sure to not enable verbose
           debugging, as you won't see a thing except endless lines of
           some tracing.
        */
        pb.set_blocking_io(pubnub::non_blocking);

        std::cout << "--------------------------" << std::endl <<
            "Subscribe loop starting..." << std::endl <<
            "--------------------------" << std::endl;

        for (;;) {
            time_t t = time(NULL);
            bool stop = false;
            pubnub::futres futres = pb.subscribe(chan);

            /* Don't await here, 'cause it will loop until done */
            while (!stop) {
                pubnub_res res = futres.last_result();
                if (res == PNR_STARTED) {
                    /* Here we simulate the "get out of subscribe loop"
                       external signal with a random number. Basically,
                       this has a 4% chance of stopping the wait every
                       second.
                    */
                    if (time(NULL) != t) {
                        stop = (rand() % 25) == 3;
                        t = time(NULL);
                    }
                }
                else {
                    if (PNR_OK == res) {
                        std::cout << "Subscribed! Got messages:" << std::endl;
                        std::string msg;
                        do {
                            msg = pb.get();
                            std::cout << msg << std::endl;
                        } while (!msg.empty());
                    }
                    else {
                        std::cout << "Subscribing failed with code: " << res << std::endl;
                    }
                    break;
                }
            }
            if (stop) {
                std::cout << "---------------------------" << std::endl <<
                    "Cancelling the Subscribe..." << std::endl <<
                    "---------------------------" << std::endl;
                pb.cancel();
                /* Now it's OK to await, since we don't have anything else
                   to do
                */
                futres.await();
                break;
            }
        }
    }
    catch (std::exception &exc) {
        std::cout << "Caught exception: " << exc.what() << std::endl;
    }

    std::cout << "Pubnub cancel-subscribe sync demo over." << std::endl;

    return 0;
}
