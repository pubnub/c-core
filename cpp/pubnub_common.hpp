/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_COMMON_HPP
#define      INC_PUBNUB_COMMON_HPP

//extern "C" {
#include "pubnub_alloc.h"
#include "pubnub_coreapi.h"
#include "pubnub_generate_uuid.h"
#include "pubnub_blocking_io.h"
#include "pubnub_ssl.h"
//}


#include <string>
#include <vector>


namespace pubnub {

    enum pubv2_opt {
        store_in_history = 0x01,
        eat_after_reading = 0x02
    };
    inline pubv2_opt operator|(pubv2_opt l, pubv2_opt r) {
        return static_cast<pubv2_opt>(static_cast<int>(l) | static_cast<int>(r));
    }
    inline pubv2_opt operator&(pubv2_opt l, pubv2_opt r) {
        return static_cast<pubv2_opt>(static_cast<int>(l) & static_cast<int>(r));
    }
    enum blocking_io {
        blocking,
        non_blocking
    };
    enum ssl_opt {
        useSSL = 0x01,
        reduceSecurityOnError = 0x02,
        ignoreSecureConnectionRequirement = 0x04
    };
    inline ssl_opt operator|(ssl_opt l, ssl_opt r) {
        return static_cast<ssl_opt>(static_cast<int>(l) | static_cast<int>(r));
    }
    inline ssl_opt operator&(ssl_opt l, ssl_opt r) {
        return static_cast<ssl_opt>(static_cast<int>(l) & static_cast<int>(r));
    }

    class context {
    public:
        context(std::string pubkey, std::string subkey) :
            d_pubk(pubkey), d_ksub(subkey) {
            d_pb = pubnub_alloc();
            if (0 == d_pb) {
                throw std::bad_alloc();
            }
            pubnub_init(d_pb, d_pubk.c_str(), d_ksub.c_str());
        }
        
        void set_auth(std::string const &auth) {
            d_auth = auth;
            pubnub_set_auth(d_pb, auth.empty() ? NULL : d_auth.c_str());
        }
        std::string const &auth() const { return d_auth; }

        void set_uuid(std::string const &uuid) {
            d_uuid = uuid;
            pubnub_set_uuid(d_pb, uuid.empty() ? NULL : d_uuid.c_str());
        }
        int set_uuid_v4_random() {
            struct Pubnub_UUID uuid;
            if (0 != pubnub_generate_uuid_v4_random(&uuid)) {
                return -1;
            }
            set_uuid(pubnub_uuid_to_string(&uuid).uuid);
            return 0;
        }

        std::string const &uuid() const { return d_uuid; }

        std::string get() const {
            char const *msg = pubnub_get(d_pb);
            return (NULL == msg) ? "" : msg;
        }
        std::vector<std::string> get_all() const {
            std::vector<std::string> all;
            while (char const *msg = pubnub_get(d_pb)) {
                if (NULL == msg) {
                    break;
                }
                all.push_back(msg);
            }
            return all;
        }
        std::string get_channel() const {
            char const *chan = pubnub_get_channel(d_pb);
            return (NULL == chan) ? "" : chan;
        }
        std::vector<std::string> get_all_channels() const {
            std::vector<std::string> all;
            while (char const *msg = pubnub_get_channel(d_pb)) {
                if (NULL == msg) {
                    break;
                }
                all.push_back(msg);
            }
            return all;
        }
        
        void cancel() {
            pubnub_cancel(d_pb);
        }
        
        futres publish(std::string const &channel, std::string const &message) {
            return doit(pubnub_publish(d_pb, channel.c_str(), message.c_str()));
        }
        futres publishv2(std::string const &channel, std::string const &message,
                         pubv2_opt options) {
            return doit(pubnub_publishv2(d_pb, channel.c_str(), message.c_str(), options & store_in_history, options & eat_after_reading));
        }
        futres subscribe(std::string const &channel, std::string const &channel_group = "") {
            return doit(pubnub_subscribe(d_pb, channel.c_str(), channel_group.c_str()));
        }
        futres leave(std::string const &channel, std::string const &channel_group) {
            return doit(pubnub_leave(d_pb, channel.c_str(), channel_group.c_str()));
        }
        futres time() {
            return doit(pubnub_time(d_pb));
        }
        futres history(std::string const &channel, std::string const &channel_group = "", unsigned count = 100) {
            return doit(pubnub_history(d_pb, channel.c_str(), channel_group.c_str(), count));
        }
        futres historyv2(std::string const &channel, std::string const &channel_group = "", unsigned count = 100, bool include_token = false) {
            return doit(pubnub_historyv2(d_pb, channel.c_str(), channel_group.c_str(), count, include_token));
        }
        futres here_now(std::string const &channel, std::string const &channel_group = "") {
            return doit(pubnub_here_now(d_pb, channel.c_str(), channel_group.c_str()));
        }
        futres global_here_now() {
            return doit(pubnub_global_here_now(d_pb));
        }
        futres where_now(std::string const &uuid = "") {
            return doit(pubnub_where_now(d_pb, uuid.empty() ? NULL : uuid.c_str()));
        }
        futres set_state(std::string const &channel, std::string const &channel_group, std::string const &uuid, std::string const &state) {
            return doit(pubnub_set_state(d_pb, channel.c_str(), channel_group.c_str(), uuid.c_str(), state.c_str()));
        }
        futres state_get(std::string const &channel, std::string const &channel_group = "", std::string const &uuid = "") {
            return doit(pubnub_state_get(d_pb, channel.c_str(), channel_group.c_str(), uuid.empty() ? NULL : uuid.c_str()));
        }
        futres remove_channel_group(std::string const &channel_group) {
            return doit(pubnub_remove_channel_group(d_pb, channel_group.c_str()));
        }
        futres remove_channel_from_group(std::string const &channel, std::string const &channel_group) {
            return doit(pubnub_remove_channel_from_group(d_pb, channel.c_str(), channel_group.c_str()));
        }
        futres add_channel_to_group(std::string const &channel, std::string const &channel_group) {
            return doit(pubnub_add_channel_to_group(d_pb, channel.c_str(), channel_group.c_str()));
        }
        futres list_channel_group(std::string const &channel_group) {
            return doit(pubnub_list_channel_group(d_pb, channel_group.c_str()));
        }
        
        int last_http_code() const { 
            return pubnub_last_http_code(d_pb);
        }
        std::string last_publish_result() const { 
            return pubnub_last_publish_result(d_pb);
        }
        std::string last_time_token() const { 
            return pubnub_last_time_token(d_pb);
        }

        int set_blocking_io(blocking_io e) {
            if (blocking == e) {
                return pubnub_set_blocking_io(d_pb);
            }
            else {
                return pubnub_set_non_blocking_io(d_pb);
            }
        }
        void set_ssl_options(ssl_opt options) {
            pubnub_set_ssl_options(
                d_pb, 
                options & useSSL,
                options & reduceSecurityOnError,
                options & ignoreSecureConnectionRequirement
                );
        }
        
        ~context() {
            pubnub_free(d_pb);
        }

    private:
        // pubnub context is not copyable
        context(context const &);

        // internal helper function
        futres doit(pubnub_res e) {
            return futres(d_pb, *this, e);
        }

        /// The publish key
        std::string d_pubk;
        /// The subscribe key
        std::string d_ksub;
        /// The auth key
        std::string d_auth;
        /// The UUID
        std::string d_uuid;
        /// The (C) Pubnub context
        pubnub_t *d_pb;
  };

}

#endif // !defined INC_PUBNUB_COMMON_HPP


#if 0
#include <iostream>

#include <initializer_list>

using namespace std;

enum /*class*/ N {
    na = 0x01,
    nb = 0x02,
    nc = 0x04,
    /*nab = na | nb,
    nbc = nb | nc,
    nac = na | nc*/
};

/*constexpr*/ N operator|(N l, N r) {
    return static_cast<N>(static_cast<int>(l) | static_cast<int>(r));
}
N operator&(N l, N r) {
    return static_cast<N>(static_cast<int>(l) & static_cast<int>(r));
}

int f (N n) {
    return static_cast<int>(n);
}
class NO {
    public:
    NO(N n) : d_n(n) {}
    NO(char const *s, std::initializer_list<N> l) {
        d_n = static_cast<N>(0);
        for(auto n : l) {
            d_n = d_n | n;
        }
    }/*
    NO(N n, N nn) : d_n(n) {}*/
    N n() const { return d_n; }
    private:
    N d_n;
};
int g(NO n) {
    return n.n();
}
int main()
{
   cout << "Hello World: " << f(nb&(nb|nc)) << endl; 
   cout << "Hello World: " << g({"x", {na, nb}}) << endl; 
   cout << "Hello World: " << g(na) << endl; 
   
   //f(1);
   //f(na);
   return f(na & (na | nb));
}

#endif
