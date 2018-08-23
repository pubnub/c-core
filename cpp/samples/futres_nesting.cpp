/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub.hpp"

#include <iostream>
#include <exception>

#include "pubnub_mutex.hpp"


/* Please note that this sample is the same whether you use the Pubnub
   C "sync" or "callback" interface during the build.

   Also, code that assumes C++ 11 features is protected by the
   standard "feature test" macro, hopefully your compiler has
   implemented it correctly.

   Here we demonstrate the `then` method of the `pubnub::futres`.
   It is an idiom well-known to the node.js users, and is also known
   as "callback hell".
 */

const std::string chan("hello_world");

static pubnub_mutex_t m_done_m;
static bool m_done;





#if __cplusplus >= 201103L

/// Here's how to use it in c++11, with lambdas
static void cpp11(pubnub::context &ipb)
{
    std::cout << "Publishing" << std::endl;
    ipb.publish(chan, "\"Hello world from C++!\"").then([=](pubnub::context &pb, pubnub_res res) {
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
        pb.subscribe(chan).then([=](pubnub::context &pb, pubnub_res res) {
            if (PNR_OK ==  res) {
                std::cout << "Subscribed!" << std::endl;
            }
            else {
                std::cout << "Subscribe failed!" << std::endl;
            }
            pb.subscribe(chan).then([=](pubnub::context &pb, pubnub_res res) {
                if (PNR_OK ==  res) {
                    std::cout << "Subscribed! Got messages:" << std::endl;
                    auto msg = pb.get_all();
                    for (auto &&x: msg) {
                        std::cout << x << std::endl;
                    }
                }
                else {
                    std::cout << "Subscribe failed!" << std::endl;
                }
                pb.time().then([=](pubnub::context &pb, pubnub_res res) {
                    if (PNR_OK == res) {
                        std::cout << "Gotten time " << pb.get() << "; last time token="<< pb.last_time_token() << std::endl;
                    }
                    else {
                        std::cout << "Getting time failed!" << std::endl;
                    }
                    std::cout << "Getting history" << std::endl;
                    pb.history(chan).then([=](pubnub::context &pb, pubnub_res res) {
                        if (PNR_OK == res) {
                            std::cout << "Got history! Messages:" << std::endl;
                            auto msg = pb.get_all();
                            for (auto &&m : msg) {
                                std::cout << m << std::endl;
                            }
                        }
                        else {
                            std::cout << "Getting history failed!" << std::endl;
                        }
                        // We could go on, but I've had enough of the callback
                        // hell - lambda/node.js style, too
                        pubnub::lock_guard lck(m_done_m);
                        m_done = true;
                        });
                    });
                });
            });
        });
}

#else

static void on_time(pubnub::context &pb, pubnub_res res)
{
    if (PNR_OK ==  res) {
        std::cout << "Gotten time " << pb.get() << "; last time token="<< pb.last_time_token() << std::endl;
    }
    else {
        std::cout << "Getting time failed!" << std::endl;
    }
    // We could go on here, but I've had enough of the "callback hell" :)
    pubnub::lock_guard lck(m_done_m);
    m_done = true;
}


static void on_subscribe(pubnub::context &pb, pubnub_res res)
{
    if (PNR_OK ==  res) {
        std::cout << "Subscribed! Got messages:" << std::endl;
        std::vector<std::string> msg = pb.get_all();
        for (std::vector<std::string>::iterator it = msg.begin(); it != msg.end(); ++it) {
            std::cout << *it << std::endl;
        }
    }
    else {
        std::cout << "Subscribe failed!" << std::endl;
    }
    pb.time().then(on_time);
}


static void on_first_subscribe(pubnub::context &pb, pubnub_res res)
{
    if (PNR_OK ==  res) {
        std::cout << "Subscribed!" << std::endl;
    }
    else {
        std::cout << "Subscribe failed!" << std::endl;
    }
    pb.subscribe(chan).then(on_subscribe);
}


static void on_published(pubnub::context &pb, pubnub_res res)
{
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
    pb.subscribe(chan).then(on_first_subscribe);
}

/// Here's how to use it in C++98 (without lambdas).  We shall
/// demonstrate only the functions, not the functional objects, the
/// only difference being that you could always pass the same
/// functional object and keep some state in it, if you need to.
static void cpp98(pubnub::context &pb)
{
    std::cout << "Publishing" << std::endl;
    pb.publish(chan, "\"Hello world from C++!\"").then(on_published);
}

#endif

int main()
{
    try {
        pubnub::context pb("demo", "demo");
 
        /* Leave this commented out to use the default - which is
           blocking I/O on most platforms. Uncomment to use non-
           blocking I/O.
        */
//       pb.set_blocking_io(pubnub::non_blocking);

#if __cplusplus >= 201103L
        m_done = false;
        cpp11(pb);
        for (;;) {
            pubnub::lock_guard lck(m_done_m);
            if (m_done) {
                break;
            }
        }
#else
        pubnub_mutex_init(m_done_m);
        cpp98(pb);
        for (;;) {
            bool done;
            pubnub_mutex_lock(m_done_m);
            done = m_done;
            pubnub_mutex_unlock(m_done_m);
            if (done) {
                break;
            }
        }
#endif
    }
    catch (std::exception &exc) {
        std::cout << "Caught exception: " << exc.what() << std::endl;
    }

std::cout << "Pubnub C++ futres nesting for " <<
#if defined(PUBNUB_CALLBACK_API)
    "callback" <<
#else
    "sync" <<
#endif
    " demo over." << std::endl;

    return 0;
}
