/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_COMMON_HPP
#define      INC_PUBNUB_COMMON_HPP

#include "pubnub_qt.h"

#include <string>
#include <vector>

#include <QMutexLocker>
#include <QWaitCondition>

extern "C" {
#include "pubnub_helper.h"
}


namespace pubnub {

    /** An algorithm to join (contatenate) the elements of the given
        range [first, last) to a string-like object, separating
        elements with the @p separator.

        A more abstract way to think about this algorithm is
        "accumulate (left fold) with a separator between elements"
        that can be used on any object that has an `+=` operator.
        
        It is implied that the range is from a container of elements
        that have the same type as the @p result, or of types that can
        be added (via `+=`) to the type of the @p result.

        The result type only has to support the `+=` operator and be
        copy or move constructible.

        @param first Input iterator of the start of the range
        @param last Input iterator of the end of the range (one past the
        last element)
        @param separator The object to put (add) between elements
        in the result.
        @param result The object to which put (add) all the elements
        and the separator - the initial value.
        @return The final result, after all additions (if any)
     */
    template <typename I, typename S>
    S join(I first, I last, S const &separator, S result) {
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

    /** Pass a STL-like container for @p container, we'll do the join
        from container.begin() to container.end(). A helper function
        to use you know that you want to join all the elements of a
        container.
     */
    template <typename C, typename S>
    S join(C container, S const &separator, S result) {
        return join(container.begin(), container.end(), separator, result);
    }

    static const std::string comma = ",";

    /** We actually know we always join strings and commas, so, this
        helps simplify things.
     */
    template <typename C> std::string join(C container) {
        return join(container, comma, std::string());
    }

    class context;

    /** A future (pending) result of a Pubnub
     * transaction/operation/request.  It is somewhat similar to the
     * std::future<> from C++11.
     *
     * It has the same interface for both Pubnub C client's "sync" and
     * "callback" interfaces, so the C++ user code is always the same,
     * you just select the "back-end" during the build.
     */
    class futres : public QObject {
        Q_OBJECT

    private slots:
        void onOutcome(pubnub_res result) { 
            signal(result); 
        }

    public:
        futres(context &ctx, pubnub_res initial);

        ~futres() {}

        /// Gets the last (or latest) result of the transaction.
        pubnub_res last_result();

        /// Starts the await. Only useful for the callback interface
        void start_await();

        /// Ends the await of the transaction to end and returns the
        /// final result (outcome)
        pubnub_res end_await();

        /// Awaits the end of transaction and returns its final result
        /// (outcome).
        pubnub_res await() {
            start_await();
            return end_await();
        }

        // C++11 std::future<> compatible API

        /// Same as await()
        pubnub_res get() { return await(); }

        /// Return whether this object is valid
        bool valid() const { return true; }

        /// Just wait for the transaction to end, don't get the
        /// outcome
        void wait() /*const*/ { await(); }

        // C++17 (somewhat) compatbile API

        /// Pass a function, function object (or lambda in C++11)
        /// which accepts a Pubnub context and pubnub_res and it will
        /// be called when the transaction ends.
        template<typename F> void then(F f) {
            f(d_ctx, await());
        }

        /// Returns if the transaction is over
        bool is_ready() const {
            //QMutexLocker lk(&d_mutex);
            return d_triggered;
        }
        
        // We can construct from a temporary
#if __cplusplus >= 201103L
        futres(futres &&x);
        futres(futres const &x) = delete;
#else
        futres(futres const &x);
#endif

    private:
        // Pubnub future result is non-copyable
        futres(futres &x);

        void signal(pubnub_res result) {
            qDebug() << "signal" << result << d_triggered << this;
            d_triggered = true;
            d_result = result;
        }

        /// The C++ Pubnub context of this future result
        context &d_ctx;

        /// The current result
        pubnub_res d_result;

        bool d_triggered;
    };
	
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
        pubnub_qt d_pbqt;
    public:
        /** Create the context, that will use @p pubkey as its
            publish key and @subkey as its subscribe key for its
            lifetime. These cannot be changed, to use another
            pair, create a new context.
        */
        context(std::string pubkey, std::string subkey);

        context(std::string pubkey, std::string subkey, std::string origin) :
            context(pubkey, subkey) {
            set_origin(origin);
        }

        /** Sets the origin to @p origin on this context. This may fail.
            To reset to default, pass an empty string.
         */
        int set_origin(std::string const &origin) {
            d_pbqt.set_origin(QString::fromStdString(origin));
            return 0;
        }
        /** Returns the current origin for this context
         */
        std::string origin() const { return d_pbqt.origin().toStdString(); }
        
        /** Sets the `auth` key to @p auth. If @p auth is an
            empty string, `auth` will not be used.
            @see pubnub_set_auth
         */
        void set_auth(std::string const &auth) {
            d_pbqt.set_auth(QString::fromStdString(auth));
        }
        /// Returns the current `auth` key for this context
        std::string auth() const { return d_pbqt.auth().toStdString(); }

        /** Sets the UUID to @p uuid. If @p uuid is an empty string,
            UUID will not be used.
         */
        void set_uuid(std::string const &uuid) {
            d_pbqt.set_uuid(QString::fromStdString(uuid));
        }
        /// Set the UUID to a random-generated one
        int set_uuid_v4_random() {
            d_pbqt.set_uuid_v4_random();
            return 0;
        }
        /// Returns the current UUID
        std::string uuid() const { return d_pbqt.uuid().toStdString(); }

        /// Returns the next message from the context. If there are
        /// none, returns an empty string.
        std::string get() const { return d_pbqt.get().toStdString(); }

        /// Returns a vector of all messages from the context.
        std::vector<std::string> get_all() const { 
            std::vector<std::string> result;
            QStringList qsl = d_pbqt.get_all();
            for (int i = 0; i < qsl.size(); ++i) {
                result.push_back(qsl[i].toStdString());
            }
            return result;
        }

        /// Returns the next channel string from the context.
        /// If there are none, returns an empty string
        std::string get_channel() const { return d_pbqt.get_channel().toStdString(); }

        /// Returns a vector of all channel strings from the context
        std::vector<std::string> get_all_channels() const { 
            std::vector<std::string> result;
            QStringList qsl = d_pbqt.get_all_channels();
            for (int i = 0; i < qsl.size(); ++i) {
                result.push_back(qsl[i].toStdString());;
            }
            return result;
        }
        
        /// Cancels the transaction, if any is ongoing. If none is
        /// ongoing, it is ignored.
        void cancel() { d_pbqt.cancel(); }
        
        /// Publishes a @p message on the @p channel. The @p channel
        /// can have many channels separated by a comma
        futres publish(std::string const &channel, std::string const &message) {
            return doit(d_pbqt.publish(QString::fromStdString(channel), QString::fromStdString(message)));
        }
#if 0
        /// Publishes a @p message on the @p channel using v2, with the
        /// options set in @p options.
        futres publishv2(std::string const &channel, std::string const &message,
                         pubv2_opt options) {
            return doit(d_pbqt.publishv2(QString::fromStdString(channel), QString::fromStdString(message), (options & store_in_history) != 0, (options & eat_after_reading) != 0));
        }
#endif
        /// Subscribes to @p channel and/or @p channel_group
        futres subscribe(std::string const &channel, std::string const &channel_group = "") {
            return doit(d_pbqt.subscribe(QString::fromStdString(channel), QString::fromStdString(channel_group)));
        }

        /// Pass a vector of channels in the @p channel and a vector
        /// of channel groups for @p channel_group and we will put
        /// commas between them. A helper function.
        futres subscribe(std::vector<std::string> const &channel, std::vector<std::string> const &channel_group) {
            return subscribe(join(channel), join(channel_group));
        }

        /// Leaves a @p channel and/or @p channel_group
        /// @see pubnub_leave
        futres leave(std::string const &channel, std::string const &channel_group) {
            return doit(d_pbqt.leave(QString::fromStdString(channel), QString::fromStdString(channel_group)));
        }

        /// Pass a vector of channels in the @p channel and a vector
        /// of channel groups for @p channel_group and we will put
        /// commas between them. A helper function.
        futres leave(std::vector<std::string> const &channel, std::vector<std::string> const &channel_group) {
            return leave(join(channel), join(channel_group));
        }

        /// Starts a "get time" transaction
        futres time() {
            return doit(d_pbqt.time());
        }

        /// Starts a transaction to get message history for @p channel
        /// with the limit of max @p count
        /// messages to retrieve, and optionally @p include_token to get
        /// a time token for each message.
        futres history(std::string const &channel, unsigned count = 100, bool include_token = false) {
            return doit(d_pbqt.history(QString::fromStdString(channel), count, include_token));
        }

        /// Starts a transaction to get a list of currently present
        /// UUIDs on a @p channel and/or @p channel_group
        futres here_now(std::string const &channel, std::string const &channel_group = "") {
            return doit(d_pbqt.here_now(QString::fromStdString(channel), QString::fromStdString(channel_group)));
        }

        /// Pass a vector of channels in the @p channel and a vector
        /// of channel groups for @p channel_group and we will put
        /// commas between them. A helper function.
        futres here_now(std::vector<std::string> const &channel, std::vector<std::string> const &channel_group) {
            return here_now(join(channel), join(channel_group));
        }

        /// Starts a transaction to get a list of currently present
        /// UUIDs on all channels
        /// @see pubnub_global_here_now
        futres global_here_now() {
            return doit(d_pbqt.global_here_now());
        }
        
        /// Starts a transaction to get a list of channels the @p uuid
        /// is currently present on. If @p uuid is not given (or is an
        /// empty string) it will 
        /// @see pubnub_where_now
        futres where_now(std::string const &uuid = "") {
            return doit(d_pbqt.where_now(QString::fromStdString(uuid)));
        }

        /// Starts a transaction to set the @p state JSON object for the
        /// given @p channel and/or @pchannel_group of the given @p uuid
        /// @see pubnub_set_state
        futres set_state(std::string const &channel, std::string const &channel_group, std::string const &uuid, std::string const &state) {
            return doit(d_pbqt.set_state(QString::fromStdString(channel), QString::fromStdString(channel_group), QString::fromStdString(uuid), QString::fromStdString(state)));
        }

        /// Pass a vector of channels in the @p channel and a vector
        /// of channel groups for @p channel_group and we will put
        /// commas between them. A helper function.
        futres set_state(std::vector<std::string> const &channel, std::vector<std::string> const &channel_group, std::string const &uuid, std::string const &state) {
            return set_state(join(channel), join(channel_group), uuid, state);
        }

        /// Starts a transaction to get the state JSON object for the
        /// given @p channel and/or @pchannel_group of the given @p
        /// uuid 
        /// @see pubnub_set_state
        futres state_get(std::string const &channel, std::string const &channel_group = "", std::string const &uuid = "") {
            return doit(d_pbqt.state_get(QString::fromStdString(channel), QString::fromStdString(channel_group), QString::fromStdString(uuid)));
        }
        futres state_get(std::vector<std::string> const &channel, std::vector<std::string> const &channel_group, std::string const &uuid = "") {
            return state_get(join(channel), join(channel_group), uuid);
        }

        /// Starts a transaction to remove a @p channel_group.
        /// @see pubnub_remove_channel_group
        futres remove_channel_group(std::string const &channel_group) {
            return doit(d_pbqt.remove_channel_group(QString::fromStdString(channel_group)));
        }

        /// Starts a transaction to remove a @p channel from a @p channel_group.
        /// @see pubnub_remove_channel_from_group
        futres remove_channel_from_group(std::string const &channel, std::string const &channel_group) {
            return doit(d_pbqt.remove_channel_from_group(QString::fromStdString(channel), QString::fromStdString(channel_group)));
        }

        /// Pass a vector of channels in the @p channel and we will
        /// put commas between them. A helper function.
        futres remove_channel_from_group(std::vector<std::string> const &channel, std::string const &channel_group) {
            return remove_channel_from_group(join(channel), channel_group);
        }

        /// Starts a transaction to add a @p channel to a @p channel_group.
        /// @see pubnub_add_channel_to_group
        futres add_channel_to_group(std::string const &channel, std::string const &channel_group) {
            return doit(d_pbqt.add_channel_to_group(QString::fromStdString(channel), QString::fromStdString(channel_group)));
        }

        /// Pass a vector of channels in the @p channel and we will
        /// put commas between them. A helper function.
        futres add_channel_to_group(std::vector<std::string> const &channel, std::string const &channel_group) {
            return add_channel_to_group(join(channel), channel_group);
        }

        /// Starts a transaction to get a list of channels belonging
        /// to a @p channel_group.
        futres list_channel_group(std::string const &channel_group) {
            return doit(d_pbqt.list_channel_group(QString::fromStdString(channel_group)));
        }
        
        /// Return the HTTP code (result) of the last transaction.
        int last_http_code() const { return d_pbqt.last_http_code(); }

        /// Return the string of the last publish transaction.
        std::string last_publish_result() const { 
            return d_pbqt.last_publish_result().toStdString();
        }

        pubnub_publish_res parse_last_publish_result() {
            return d_pbqt.parse_last_publish_result();
        }

        /// Return the string of the last time token.
        std::string last_time_token() const { return d_pbqt.last_time_token().toStdString(); }

        /// Sets whether to use (non-)blocking I/O according to option @p e.
        int set_blocking_io(blocking_io /*e*/) { return -1; }

#if 0
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
#endif        
        /// Frees the context and any other thing that needs to be
        /// freed/released.
        ~context() {}

    private:
        // pubnub context is not copyable
        context(context const &);

        friend futres;

        // internal helper function
        futres doit(pubnub_res e) {
            return futres(*this, e);
        }

    };

}

/// Helper to put a `pubnub_res` to a standard stream
std::ostream& operator<<(std::ostream&out, pubnub_res e);


#endif // !defined INC_PUBNUB_COMMON_HPP
