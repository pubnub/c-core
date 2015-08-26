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

    /** An algorithm to join (contatenate) the elements of the given
        range [first, last) to a string-like object, separating
        elements with the @p separator. 

        It is implied that the range is from a container of elements
        that have the same type as the @p separator, or of types
        that can be `added` to the type of the @p separator.

        The string-like type only has to support the `+=` operator,
        be default constructible and copy or move constructible.

        @param first Input iterator of the start of the range
        @param last Input iterator of the end of the range (one past the
        last element)
        @param separator The string-like object to put between elements
        in the result.
        @return A string-like object containing all the elements from
        the range [first, last], separated with @p separator. If range
        is empty, a default-constructed element of the type of the
        @p separator (i.e. an empty string)
     */
    template <typename I, typename S>
    S join(I first, I last, S const &separator) {
        S result;
        if (first == last) {
            return result;
        }
        I prev = first;
        I next = first;
        for (++next; next != last; ++next) {
            result += *prev;
            result += separator;
            prev = next;
        }
        result += *prev;
        return result;
    }

    /** Pass a container for @p container, well do the join from
        container.begin() to container.end(). A helper function to use
        you know that you want to join all the elements of a
        container.
     */
    template <typename C, typename S>
    S join(C container, S const &separator) {
        return join(container.begin(), container.end(), separator);
    }

	static const std::string comma = ",";
	
    /** Options for Publish v2. These are designed to be used as
     * "bit-masks", for which purpose there are overloaded `&` and `|`
     * (bit-and and bit-or) operators.
     */
    enum pubv2_opt {
        /// If not set, message will not be stored in history of the channel
        store_in_history = 0x01,
        /// If set, message will not be stored for delayed or repeated
        /// retrieval or display
        eat_after_reading = 0x02
    };
    inline pubv2_opt operator|(pubv2_opt l, pubv2_opt r) {
        return static_cast<pubv2_opt>(static_cast<int>(l) | static_cast<int>(r));
    }
    inline pubv2_opt operator&(pubv2_opt l, pubv2_opt r) {
        return static_cast<pubv2_opt>(static_cast<int>(l) & static_cast<int>(r));
    }

    /// Posible settings for (usage of) blocking I/O
    enum blocking_io {
        /// Use blocking I/O
        blocking,
        /// Use non-blocking I/O
        non_blocking
    };

    /** Options for SSL/TLS transport. These are designed to be used as
     * "bit-masks", for which purpose there are overloaded `&` and `|`
     * (bit-and and bit-or) operators.
     */
    enum ssl_opt {
        /// Should the PubNub client establish the connection to
        /// PubNub using SSL? (default: YES)
        useSSL = 0x01,
        /// When SSL is enabled, should PubNub client ignore all SSL
        /// certificate-handshake issues and still continue in SSL
        /// mode if it experiences issues handshaking across local
        /// proxies, firewalls, etc? (default: YES)
        reduceSecurityOnError = 0x02,
        /// When SSL is enabled, should the client fallback to a
        /// non-SSL connection if it experiences issues handshaking
        /// across local proxies, firewalls, etc? (default: YES)
        ignoreSecureConnectionRequirement = 0x04
    };
    inline ssl_opt operator|(ssl_opt l, ssl_opt r) {
        return static_cast<ssl_opt>(static_cast<int>(l) | static_cast<int>(r));
    }
    inline ssl_opt operator&(ssl_opt l, ssl_opt r) {
        return static_cast<ssl_opt>(static_cast<int>(l) & static_cast<int>(r));
    }

    /** The C++ Pubnub context. It is a wrapper of the Pubnub C context,
     * not a "native" C++ implementation.
     *
     * One of its main design decisions is to make it safe,
     * sacrificing some performance (and memory footprint). The
     * rationale is that you can use the Pubnub C client "as-is" from
     * C++ if you're going for really high performance and low
     * footprint.
     *
     * The other main design decision is to use a "std::future<>" like
     * interface for getting the outcome of a Pubnub transaction and
     * that is the same regardless of the interface used for the
     * Pubnub C library that we wrap. IOW, it is the same for both the
     * "sync" and "callback" interfaces. Thus, C++ client code is
     * always the same and the user selects during build the Pubnub C
     * "backend" to use.
     *
     * Also, here we document the C++ aspects of certain methods and
     * their general meaning. For details, refer to the Pubnub C
     * documentation, as this is just a wrapper, all of it holds.
     */
    class context {
    public:
        /** Create the context, that will use @p pubkey as its
            publish key and @subkey as its subscribe key for its
            lifetime. These cannot be changed, to use another
            pair, create a new context.
            @see pubnub_alloc, pubnub_init
        */
        context(std::string pubkey, std::string subkey) :
            d_pubk(pubkey), d_ksub(subkey) {
            d_pb = pubnub_alloc();
            if (0 == d_pb) {
                throw std::bad_alloc();
            }
            pubnub_init(d_pb, d_pubk.c_str(), d_ksub.c_str());
        }
        
        /** Sets the `auth` key to @p auth. If @p auth is an
            empty string, `auth` will not be used.
            @see pubnub_set_auth
         */
        void set_auth(std::string const &auth) {
            d_auth = auth;
            pubnub_set_auth(d_pb, auth.empty() ? NULL : d_auth.c_str());
        }
        /// Returns the current `auth` key for this context
        std::string const &auth() const { return d_auth; }

        /** Sets the UUID to @p uuid. If @p uuid is an empty string,
            UUID will not be used.
            @see pubnub_set_uuid
         */
        void set_uuid(std::string const &uuid) {
            d_uuid = uuid;
            pubnub_set_uuid(d_pb, uuid.empty() ? NULL : d_uuid.c_str());
        }
        /// Set the UUID to a random-generated one
        /// @see pubnub_generate_uuid_v4_random
        int set_uuid_v4_random() {
            struct Pubnub_UUID uuid;
            if (0 != pubnub_generate_uuid_v4_random(&uuid)) {
                return -1;
            }
            set_uuid(pubnub_uuid_to_string(&uuid).uuid);
            return 0;
        }
        /// Returns the current UUID
        std::string const &uuid() const { return d_uuid; }

        /// Returns the next message from the context. If there are
        /// none, returns an empty string.
        /// @see pubnub_get
        std::string get() const {
            char const *msg = pubnub_get(d_pb);
            return (NULL == msg) ? "" : msg;
        }
        /// Returns a vector of all messages from the context.
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
        /// Returns the next channel string from the context.
        /// If there are none, returns an empty string
        /// @see pubnub_get_channel
        std::string get_channel() const {
            char const *chan = pubnub_get_channel(d_pb);
            return (NULL == chan) ? "" : chan;
        }
        /// Returns a vector of all channel strings from the context
        /// @see pubnub_get_channel
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
        
        /// Cancels the transaction, if any is ongoing. If none is
        /// ongoing, it is ignored.
        /// @see pubnub_cancel
        void cancel() {
            pubnub_cancel(d_pb);
        }
        
        /// Publishes a @p message on the @p channel. The @p channel
        /// can have many channels separated by a comma
        /// @see pubnub_publish
        futres publish(std::string const &channel, std::string const &message) {
            return doit(pubnub_publish(d_pb, channel.c_str(), message.c_str()));
        }

        /// Pass a vector of channels in the @p channel and we will
        /// put commas between them. A helper function.
        futres publish(std::vector<std::string> const &channel, std::string const &message) {
            return publish(join(channel, comma), message);
        }

        /// Publishes a @p message on the @p channel using v2, with the
        /// options set in @p options.
        /// @see pubnub_publishv2
        futres publishv2(std::string const &channel, std::string const &message,
                         pubv2_opt options) {
            return doit(pubnub_publishv2(d_pb, channel.c_str(), message.c_str(), (options & store_in_history) != 0, (options & eat_after_reading) != 0));
        }

        /// Pass a vector of channels in the @p channel and we will
        /// put commas between them. A helper function.
        futres publishv2(std::vector<std::string> const &channel, std::string const &message, pubv2_opt options) {
            return publishv2(join(channel, comma), message, options);
        }
        
        /// Subscribes to @p channel and/or @p channel_group
        /// @see pubnub_subscribe
        futres subscribe(std::string const &channel, std::string const &channel_group = "") {
            return doit(pubnub_subscribe(d_pb, channel.c_str(), channel_group.c_str()));
        }

        /// Pass a vector of channels in the @p channel and a vector
        /// of channel groups for @p channel_group and we will put
        /// commas between them. A helper function.
        futres subscribe(std::vector<std::string> const &channel, std::vector<std::string> const &channel_group) {
            return subscribe(join(channel, comma), join(channel_group, comma));
        }

        /// Leaves a @p channel and/or @p channel_group
        /// @see pubnub_leave
        futres leave(std::string const &channel, std::string const &channel_group) {
            return doit(pubnub_leave(d_pb, channel.c_str(), channel_group.c_str()));
        }

        /// Pass a vector of channels in the @p channel and a vector
        /// of channel groups for @p channel_group and we will put
        /// commas between them. A helper function.
        futres leave(std::vector<std::string> const &channel, std::vector<std::string> const &channel_group) {
            return leave(join(channel, comma), join(channel_group, comma));
        }

        /// Starts a "get time" transaction
        /// @see pubnub_time
        futres time() {
            return doit(pubnub_time(d_pb));
        }

        /// Starts a transaction to get message history for @p channel
        /// and/or @pchannel_group, with the limit of max @p count
        /// messages to retrieve
        /// @see pubnub_history
        futres history(std::string const &channel, std::string const &channel_group = "", unsigned count = 100) {
            return doit(pubnub_history(d_pb, channel.c_str(), channel_group.c_str(), count));
        }

        /// Pass a vector of channels in the @p channel and a vector
        /// of channel groups for @p channel_group and we will put
        /// commas between them. A helper function.
        futres history(std::vector<std::string> const &channel, std::vector<std::string> const &channel_group, unsigned count = 100) {
            return history(join(channel, comma), join(channel_group, comma), count);
        }

        /// Starts a transaction to get message history for @p channel
        /// and/or @pchannel_group, with the limit of max @p count
        /// messages to retrieve, and optionally @p include_token to get
        /// a time token for each message. Uses the v2 protocol.
        /// @see pubnub_historyv2
        futres historyv2(std::string const &channel, std::string const &channel_group = "", unsigned count = 100, bool include_token = false) {
            return doit(pubnub_historyv2(d_pb, channel.c_str(), channel_group.c_str(), count, include_token));
        }

        /// Pass a vector of channels in the @p channel and a vector
        /// of channel groups for @p channel_group and we will put
        /// commas between them. A helper function.
        futres historyv2(std::vector<std::string> const &channel, std::vector<std::string> const &channel_group, unsigned count = 100, bool include_token = false) {
            return historyv2(join(channel, comma), join(channel_group, comma), count, include_token);
        }

        /// Starts a transaction to get a list of currently present
        /// UUIDs on a @p channel and/or @p channel_group
        /// @see pubnub_here_now
        futres here_now(std::string const &channel, std::string const &channel_group = "") {
            return doit(pubnub_here_now(d_pb, channel.c_str(), channel_group.c_str()));
        }

        /// Pass a vector of channels in the @p channel and a vector
        /// of channel groups for @p channel_group and we will put
        /// commas between them. A helper function.
        futres here_now(std::vector<std::string> const &channel, std::vector<std::string> const &channel_group) {
            return here_now(join(channel, comma), join(channel_group, comma));
        }

        /// Starts a transaction to get a list of currently present
        /// UUIDs on all channels
        /// @see pubnub_global_here_now
        futres global_here_now() {
            return doit(pubnub_global_here_now(d_pb));
        }
        
        /// Starts a transaction to get a list of channels the @p uuid
        /// is currently present on. If @p uuid is not given (or is an
        /// empty string) it will 
        /// @see pubnub_where_now
        futres where_now(std::string const &uuid = "") {
            return doit(pubnub_where_now(d_pb, uuid.empty() ? NULL : uuid.c_str()));
        }

        /// Starts a transaction to set the @p state JSON object for the
        /// given @p channel and/or @pchannel_group of the given @p uuid
        /// @see pubnub_set_state
        futres set_state(std::string const &channel, std::string const &channel_group, std::string const &uuid, std::string const &state) {
            return doit(pubnub_set_state(d_pb, channel.c_str(), channel_group.c_str(), uuid.c_str(), state.c_str()));
        }

        /// Pass a vector of channels in the @p channel and a vector
        /// of channel groups for @p channel_group and we will put
        /// commas between them. A helper function.
        futres set_state(std::vector<std::string> const &channel, std::vector<std::string> const &channel_group, std::string const &uuid, std::string const &state) {
            return set_state(join(channel, comma), join(channel_group, comma), uuid, state);
        }

        /// Starts a transaction to get the state JSON object for the
        /// given @p channel and/or @pchannel_group of the given @p
        /// uuid 
        /// @see pubnub_set_state
        futres state_get(std::string const &channel, std::string const &channel_group = "", std::string const &uuid = "") {
            return doit(pubnub_state_get(d_pb, channel.c_str(), channel_group.c_str(), uuid.empty() ? NULL : uuid.c_str()));
        }
        futres state_get(std::vector<std::string> const &channel, std::vector<std::string> const &channel_group, std::string const &uuid = "") {
            return state_get(join(channel, comma), join(channel_group, comma), uuid);
        }

        /// Starts a transaction to remove a @p channel_group.
        /// @see pubnub_remove_channel_group
        futres remove_channel_group(std::string const &channel_group) {
            return doit(pubnub_remove_channel_group(d_pb, channel_group.c_str()));
        }

        /// Pass a vector of channel groups for @p channel_group and
        /// we will put commas between them. A helper function.
        futres remove_channel_group(std::vector<std::string> const &channel_group) {
            return remove_channel_group(join(channel_group, comma));
        }

        /// Starts a transaction to remove a @p channel from a @p channel_group.
        /// @see pubnub_remove_channel_from_group
        futres remove_channel_from_group(std::string const &channel, std::string const &channel_group) {
            return doit(pubnub_remove_channel_from_group(d_pb, channel.c_str(), channel_group.c_str()));
        }

        /// Pass a vector of channels in the @p channel and a vector
        /// of channel groups for @p channel_group and we will put
        /// commas between them. A helper function.
        futres remove_channel_from_group(std::vector<std::string> const &channel, std::vector<std::string> const &channel_group) {
            return remove_channel_from_group(join(channel, comma), join(channel_group, comma));
        }

        /// Starts a transaction to add a @p channel to a @p channel_group.
        /// @see pubnub_add_channel_to_group
        futres add_channel_to_group(std::string const &channel, std::string const &channel_group) {
            return doit(pubnub_add_channel_to_group(d_pb, channel.c_str(), channel_group.c_str()));
        }

        /// Pass a vector of channels in the @p channel and a vector
        /// of channel groups for @p channel_group and we will put
        /// commas between them. A helper function.
        futres add_channel_to_group(std::vector<std::string> const &channel, std::vector<std::string> const &channel_group) {
            return add_channel_to_group(join(channel, comma), join(channel_group, comma));
        }

        /// Starts a transaction to get a list of channels belonging
        /// to a @p channel_group.
        /// @see pubnub_list_channel_group
        futres list_channel_group(std::string const &channel_group) {
            return doit(pubnub_list_channel_group(d_pb, channel_group.c_str()));
        }
        futres list_channel_group(std::vector<std::string> const &channel_group) {
            return list_channel_group(join(channel_group, comma));
        }
        
        /// Return the HTTP code (result) of the last transaction.
        /// @see pubnub_last_http_code
        int last_http_code() const { 
            return pubnub_last_http_code(d_pb);
        }

        /// Return the string of the last publish transaction.
        /// @see pubnub_last_publish_result
        std::string last_publish_result() const { 
            return pubnub_last_publish_result(d_pb);
        }

        /// Return the string of the last time token.
        /// @see pubnub_last_time_token
        std::string last_time_token() const { 
            return pubnub_last_time_token(d_pb);
        }

        /// Sets whether to use (non-)blocking I/O according to option @p e.
        /// @see pubnub_set_blocking_io, pubnub_set_non_blocking_io
        int set_blocking_io(blocking_io e) {
            if (blocking == e) {
                return pubnub_set_blocking_io(d_pb);
            }
            else {
                return pubnub_set_non_blocking_io(d_pb);
            }
        }

        /// Sets the SSL/TLS options according to @p options
        /// @see pubnub_set_ssl_options
        void set_ssl_options(ssl_opt options) {
            pubnub_set_ssl_options(
                d_pb, 
                (options & useSSL) != 0,
                (options & reduceSecurityOnError) != 0,
                (options & ignoreSecureConnectionRequirement) != 0
                );
        }
        
        /// Frees the context and any other thing that needs to be
        /// freed/released.
        /// @see pubnub_free
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
