/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub.hpp"

#include <iostream>
#include <exception>
#include <thread>
#include <vector>


/* Please note that this sample is the same whether you use the Pubnub
   C "sync" or "callback" interface during the build.

   Also, code that assumes C++ 11 features is protected by the
   standard "feature test" macro, hopefully your compiler has
   implemented it correctly.
 */

int main()
{
    try {
        std::string msg("Hello world from C++!");
        std::string cipher_key("enigma");

        std::vector<std::uint8_t> msg_vec(msg.begin(), msg.end());

        std::cout << "message to be encrypted: " << msg << std::endl;

        auto legacy_crypto_module = pubnub::crypto_module::legacy(cipher_key);
        auto legacy_encrypt_result = legacy_crypto_module.encrypt(msg_vec);

        std::cout << "legacy encrypted message: " 
            << std::string(legacy_encrypt_result.begin(), legacy_encrypt_result.end()) << std::endl;

        auto crypto_module = pubnub::crypto_module::aes_cbc(cipher_key);
        auto encrypt_result = crypto_module.encrypt(msg_vec);

        std::cout << "encrypted message: " 
            << std::string(encrypt_result.begin(), encrypt_result.end()) << std::endl;

        auto legacy_decrypt_result = legacy_crypto_module.decrypt(legacy_encrypt_result);

        std::cout << "legacy decrypted message: " 
            << std::string(legacy_decrypt_result.begin(), legacy_decrypt_result.end()) << std::endl;

        auto decrypt_result = crypto_module.decrypt(encrypt_result);

        std::cout << "decrypted message: " 
            << std::string(decrypt_result.begin(), decrypt_result.end()) << std::endl;
        
#ifdef PUBNUB_CRYPTO_API

        enum pubnub_res res;
        std::string chan("hello_world");
        pubnub::context pb("demo", "demo");
 
        /* Leave this commented out to use the default - which is
           blocking I/O on most platforms. Uncomment to use non-
           blocking I/O.
        */
        pb.set_blocking_io(pubnub::non_blocking);
        
        pb.set_user_id("zeka-peka-iz-jendeka");
        pb.set_auth("danaske");
        pb.set_transaction_timeout(
#if __cplusplus >= 201103L
            std::chrono::seconds(10)
#else
            10000
#endif
        );

        auto module = pubnub::crypto_module::aes_cbc(cipher_key);
        pb.set_crypto_module(module);

        if (PNR_OK == pb.subscribe(chan).await()) {
            std::cout << "Subscribed to channel " << chan << std::endl;
        }
        else {
            std::cout << "Subscribe failed!" << std::endl;
            return -1;
        }

        if (PNR_OK == pb.publish(chan, msg).await()) {
            std::cout << "Published message '" << msg << "' to channel " << chan << std::endl;
        }
        else {
            std::cout << "Publish failed!" << std::endl;
            return -1;
        }

        if (PNR_OK == pb.subscribe(chan).await()) {
            auto messages = pb.get_all();
            for (auto const& m : messages) {
                std::cout << "Received message: " << m << std::endl;
            }
        }
        else {
            std::cout << "Subscribe failed!" << std::endl;
            return -1;
        }

#endif /* PUBNUB_CRYPTO_API */
    }
    catch (std::exception& ex) {
        std::cout << "Caught exception: " << ex.what() << std::endl;
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
