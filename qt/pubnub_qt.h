/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_QT
#define      INC_PUBNUB_QT

#include <stdexcept>

#include <QUrl>
#include <QUuid>
#include <QNetworkAccessManager>
#include <QTimer>
#include <QStringList>
#include <QDebug>
#include <QJsonDocument>
#include <QMutexLocker>
#include <QMap>
#include <QPair>

#include <QtGlobal>
#define QT_6 0x060000

extern "C" {
#include "core/pubnub_server_limits.h"
#include "core/pubnub_api_types.h"
#include "core/pubnub_ccore_limits.h"
#include "core/pubnub_coreapi_ex.h"
#include "core/pubnub_helper.h"
#if PUBNUB_USE_SUBSCRIBE_V2
#include "core/pbcc_subscribe_v2.h"
#endif
#if PUBNUB_USE_ACTIONS_API
#include "core/pbcc_actions_api.h"
#endif
#if PUBNUB_USE_PAM_V3
#include "core/pbcc_grant_token_api.h"
#endif
#include "core/pubnub_crypto.h"
}

#include "cpp/tribool.hpp"
#include "lib/pb_deprecated.h"
#if PUBNUB_USE_SUBSCRIBE_V2
#include "cpp/pubnub_v2_message.hpp"
#endif

QT_BEGIN_NAMESPACE
class QNetworkReply;
class QSslError;
QT_END_NAMESPACE

/** A wrapper class for publish options, enabling a nicer
    usage. Something like:

        pn.publish(chan, msg, publish_options().store(true).ttl(22));
*/
class publish_options {
    pubnub_publish_options d_;
    QString d_meta;

public:

    publish_options() : d_(pubnub_publish_defopts()) {}

    /** If true, the message is stored in history. If false,
        the message is _not_ stored in history.
    */
    publish_options& store(bool store)
    {
        d_.store = store;

        return *this;
    }    

    /** If `true`, the message is replicated, thus will be received by
        all subscribers. If `false`, the message is _not_ replicated
        and will be only delivered to BLOCK event handlers. Setting
        `false` here and `false` on store is sometimes referred to as
        a "fire" (instead of a "publish").
    */
    publish_options& replicate(bool replicate)
    {
        d_.replicate = replicate;

        return *this;
    }

    /** An optional JSON object, used to send additional ("meta") data
        about the message, which can be used for stream filtering.
     */
    publish_options& meta(QString const& meta)
    {
        d_meta = meta;
        d_.meta = d_meta.isEmpty() ? 0 : d_meta.toLatin1().data();

        return *this;
    }

    /** Defines the method by which publish transaction will be performed */
    publish_options& method(pubnub_method method)
    {
        d_.method = method;

        return *this;
    }

    pubnub_publish_options data() { return d_; }
};


/** A wrapper class for set_state options, enabling a nicer
    usage. Something like:

        pn.set_state(chan, state, set_state_options().heartbeat(true));
*/

class set_state_options {
    pubnub_set_state_options d_;
    QString d_channel_group;
    QString d_user_id;

public:
    set_state_options() : d_(pubnub_set_state_options()) {}

    set_state_options& channel_group(QString const& channel_group)
    {
        d_channel_group = channel_group;
        d_.channel_group = d_channel_group.isEmpty() ? 0 : d_channel_group.toLatin1().data();

        return *this;
    }

    set_state_options& channel_group(QStringList const& channel_groups)
    {
        return channel_group(channel_groups.join(","));
    }

    set_state_options& user_id(QString const& user_id)
    {
        d_user_id = user_id;
        d_.user_id = d_user_id.isEmpty() ? 0 : d_user_id.toLatin1().data();

        return *this;
    }

    set_state_options& heartbeat(bool heartbeat)
    {
        d_.heartbeat = heartbeat;

        return *this;
    }

    pubnub_set_state_options data() { return d_; }
};

#if PUBNUB_USE_OBJECTS_API
#define MAX_INCLUDE_DIMENSION 100
#define MAX_ELEM_LENGTH 30
/** A wrapper class for objects api managing include parameter */
class include_options {
    char d_include_c_strings_array[MAX_INCLUDE_DIMENSION][MAX_ELEM_LENGTH + 1];
    size_t d_include_count;
    
public:
    include_options()
        : d_include_count(0)
    {}
    const char** include_c_strings_array()
    {
        return (d_include_count > 0) ? (const char**)d_include_c_strings_array : 0;
    }
    const char** include_to_c_strings_array(QStringList const& inc)
    {
        size_t n = inc.size();
        unsigned i;
        if (n > MAX_INCLUDE_DIMENSION) {
            throw std::range_error("include parameter has too many elements.");
        }
        for (i = 0; i < n; i++) {
            if (inc[i].size() > MAX_ELEM_LENGTH) {
                throw std::range_error("include string element is too long.");
            }
            strcpy(d_include_c_strings_array[i], inc[i].toLatin1().data());
        }
        d_include_count = n;
        return include_c_strings_array();
    }
    include_options(QStringList const& inc)
    {
        include_to_c_strings_array(inc);
    }
    size_t include_count() { return d_include_count; }
};
    
/** A wrapper class for objects api options for manipulating specified requirements
    and paged response, enabling a nicer usage. Something like:
       pbp.get_users(list_options().start(last_bookmark));

    instead of:
       pbp.get_users(nullopt, nullopt, last_bookmark, “”, nullopt);
  */
using namespace pubnub;
class list_options {
    QString d_include;
    size_t d_limit;
    QString d_start;
    QString d_end;
    tribool d_count;

public:
    list_options()
        : d_limit(0)
        , d_count(tribool::not_set)
    {}
    
    list_options& include(QString const& in)
    {
        d_include = in;
        return *this;
    }
    char const* include() { return d_include.isEmpty() ? 0 : d_include.toLatin1().data(); }

    list_options& limit(size_t lim)
    {
        d_limit = lim;
        return *this;
    }
    size_t limit() { return d_limit; }
    list_options& start(QString const& st)
    {
        d_start = st;
        return *this;
    }
    char const* start() { return d_start.isEmpty() ? 0 : d_start.toLatin1().data(); }
    list_options& end(QString const& e)
    {
        d_end = e;
        return *this;
    }
    char const* end() { return d_end.isEmpty() ? 0 : d_end.toLatin1().data(); }
    list_options& count(tribool co)
    {
        d_count = co;
        return *this;
    }
    pubnub_tribool count()
    {
        if (false == d_count) {
            return pbccFalse;
        }
        else if (true == d_count) {
            return pbccTrue;
        }
        return pbccNotSet;
    }
};
#endif /* PUBNUB_USE_OBJECTS_API */

struct pbcc_context;


#if PUBNUB_USE_SUBSCRIBE_V2
/** A wrapper class for subscribe_v2 options, enabling a nicer
    usage. Something like:

        pn.subscribe_v2(chan, subscribe_v2_options().heartbeat(412));
*/
class subscribe_v2_options {
    unsigned    d_heartbeat;
    QString d_chgrp;
    QString d_filter_expr;
    
public:
    subscribe_v2_options() : d_heartbeat(PUBNUB_MINIMAL_HEARTBEAT_INTERVAL) {}
    subscribe_v2_options& channel_group(QString const& chgroup)
    {
        d_chgrp = chgroup;
        return *this;
    }
    subscribe_v2_options& channel_group(QStringList const& chgroup)
    {
        return channel_group(chgroup.join(","));
    }
    subscribe_v2_options& heartbeat(unsigned hb_interval)
    {
        d_heartbeat = hb_interval;
        return *this;
    }
    subscribe_v2_options& filter_expr(QString const& filter_exp)
    {
        d_filter_expr = filter_exp;
        return *this;
    }
    unsigned* get_heartbeat() { return &d_heartbeat; }
    char const* get_chgroup() { return d_chgrp.isEmpty() ? 0 : d_chgrp.toLatin1().data(); }
    char const* get_filter_expr()
    {
        return d_filter_expr.isEmpty() ? 0 : d_filter_expr.toLatin1().data();
    }
};
#endif /* PUBNUB_USE_SUBSCRIBE_V2 */

/** Interface for a cryptor. It is an algorithm class that
 * provides encryption and decryption of arrays of bytes.
 *
 * It is used by the C++ Pubnub context to encrypt and decrypt messages 
 * sent and received from Pubnub.
 *
 * Note that name comes from Rust, where it is used to convert from type to type.
 * The pointer returned by `into_cryptor_ptr()` should be allocated with `new`.
 *
 * @see pubnub_cryptor 
*/
class into_cryptor_ptr {
public:
    ~into_cryptor_ptr() { }

    virtual pubnub_cryptor_t* into_cryptor() = 0;
};


/** Interface for a crypto provider. It is used to encrypt and decrypt 
 * messages sent and received from Pubnub. It makes a conversion from 
 * your cryptographic module to the C interface used by the C++ Pubnub context.
 *
 * It is used by the C++ Pubnub context to encrypt and decrypt messages 
 * sent and received from Pubnub.
 *
 * Note that name comes from Rust, where it is used to convert from type to type.
 * The pointer returned by `into_provider_ptr()` should be allocated with `new`.
 *
 * @see pubnub_crypto_provider_t
*/
class into_crypto_provider_ptr {
public:
    ~into_crypto_provider_ptr() { }

    virtual pubnub_crypto_provider_t* into_provider() = 0;
};


/** Default implementation of the `crypto_provider` interface.
 *
 * It is used by the C++ Pubnub context to encrypt and decrypt messages 
 * sent and received from Pubnub.
 *
 * It is designed to behave like the default PubNub crypto module 
 * returned by `pubnub_crypto_aes_cbc_module_init()`,
 * `pubnub_crypto_legacy_module_init()` and `pubnub_crypto_module_init()` functions.
 *
 * @see crypto_provider
 * @see pubnub_crypto_aes_cbc_module_init
 * @see pubnub_crypto_legacy_module_init
 * @see pubnub_crypto_module_init
*/
class crypto_module: public into_crypto_provider_ptr {
public:
    /* Wrapping constructor for the `pubnub_crypto_provider_t` structure.
     *
     * Note that this function takes ownership of the `crypto_module` pointer.
     * It will be freed when the PubNub `context` object is destroyed.
     *
     * @param crypto_module The `pubnub_crypto_provider_t` structure to wrap.
     */
    crypto_module(pubnub_crypto_provider_t* crypto_module) : d_module(crypto_module) {}

    /* Constructor that translates C++ data into the `pubnub_crypto_provider_init` function.
     *
     * Note that this function takes vector of `into_cryptor_ptr` data, that transletes
     * into the `pubnub_cryptor_t` structure.
     * It copies the data from the vector into the `additional_cryptors` vector and
     * allocates the new C array of `pubnub_cryptor_t` structures.
     *
     * @param cryptor The `pubnub_cryptor_t` structure to wrap.
     */
    crypto_module(into_cryptor_ptr &default_cryptor, QVector<into_cryptor_ptr*> &additional_cryptors)
    {
        size_t size = additional_cryptors.size();
        pubnub_cryptor_t* cryptors = new pubnub_cryptor_t[sizeof(pubnub_cryptor_t*) * size];

        for (size_t i = 0; i < size; ++i) {
            pubnub_cryptor_t* cryptor = additional_cryptors[i]->into_cryptor();
            cryptors[i] = *cryptor;
        }

        this->d_module = pubnub_crypto_module_init(default_cryptor.into_cryptor(), cryptors, size);
    }

    /* Constructor that creates C++ `crypto_module` object that mimics
     * the `pubnub_crypto_aes_cbc_module_init` function. 
     *
     * @param cryptor The `pubnub_cryptor_t` structure to wrap.
     *
     * @return The `crypto_module` object
     *
     * @see pubnub_crypto_aes_cbc_module_init
     */
    static crypto_module aes_cbc(QString &cipher_key)
    {
        return crypto_module(pubnub_crypto_aes_cbc_module_init((uint8_t*)(cipher_key.toStdString().c_str())));
    }

    /* Constructor that creates C++ `crypto_module` object that mimics
     * the `pubnub_crypto_legacy_module_init` function. 
     *
     * @param cryptor The `pubnub_cryptor_t` structure to wrap.
     *
     * @return The `crypto_module` object
     *
     * @see pubnub_crypto_legacy_module_init
     */
    static crypto_module legacy(QString &cipher_key)
    {
        return crypto_module(pubnub_crypto_legacy_module_init((uint8_t*)(cipher_key.toStdString().c_str())));
    }

    /* Encrypts the @p to_encrypt buffer and returns the encrypted
     * buffer.
     *
     * @param to_encrypt The buffer to encrypt
     *
     * @return The encrypted buffer
     */
    std::vector<std::uint8_t> encrypt(QVector<std::uint8_t>& to_encrypt) {
        pubnub_bymebl_t to_encrypt_c;
        to_encrypt_c.ptr = to_encrypt.data();
        to_encrypt_c.size = to_encrypt.size();

        pubnub_bymebl_t result = this->d_module->encrypt(this->d_module, to_encrypt_c);

        if (result.ptr == nullptr) {
            throw std::runtime_error("crypto_module::encrypt: Encryption failed!");
        }

        return std::vector<std::uint8_t>(result.ptr, result.ptr + result.size);
    }

    /* Decrypts the @p to_decrypt buffer and returns the decrypted
     * buffer.
     *
     * @param to_decrypt The buffer to decrypt
     *
     * @return The decrypted buffer
     */
    std::vector<std::uint8_t> decrypt(QVector<std::uint8_t>& to_decrypt) {
        pubnub_bymebl_t to_decrypt_c;
        to_decrypt_c.ptr = to_decrypt.data();
        to_decrypt_c.size = to_decrypt.size();

        pubnub_bymebl_t result = this->d_module->decrypt(this->d_module, to_decrypt_c);

        if (result.ptr == nullptr) {
            throw std::runtime_error("crypto_module::decrypt: Decryption failed!");
        }

        return std::vector<std::uint8_t>(result.ptr, result.ptr + result.size);
    }

    pubnub_crypto_provider_t* into_provider() {
        return this->d_module;
    }

private:
    pubnub_crypto_provider_t* d_module;
};

/** @mainpage Pubnub C-core for Qt

    This is the C-core implementation of the Pubnub client
    for the Qt framework.

    The user interface class to use is \ref pubnub_qt.
*/


/** Pubnub client "context" for Qt.
 *
 * Represents a Pubnub "context". One context handles at most
 * one Pubnub API transaction/operation at a time.
 * It is similar in the way it works with other C-core Pubnub
 * clients, but there are a few differences that are Qt-specific.
 *
 * Unlike the C++ wrapper, which wraps a "full featured" C-core
 * platform, this is a "Qt-native C-core" of sorts. It provides
 * it's own (Qt) API, while using us much of C-core modules as
 * posible.
 *
 * The outcome of an operation is reported to a user via a Qt
 * signal. There is no "sync" or "callback" interface, though
 * Qt signals are similar to callbacks and we might introduce
 * a "sync" interface.
 *
 * This is not a widget, it is a pure "communication" class,
 * which you can use both in Qt GUI and Qt command line
 * applications.
 */
class pubnub_qt : public QObject
{
    Q_OBJECT

public:
    /** Creates a Pubnub Qt context.
     * @param pubkey The publish key to use in the new context
     * @param keysub The subscribe key to use in the new context
     */
    pubnub_qt(QString pubkey, QString keysub);

    ~pubnub_qt();

    /** Options for SSL/TLS transport. These are designed to be used as
     * "bit-masks", for which purpose there are overloaded `&` and `|`
     * (bit-and and bit-or) operators.
     */
    enum ssl_opt {
        /// Should the PubNub client establish the connection to
        /// PubNub using SSL? (default: YES)
        useSSL = 0x01,
        /// When SSL is enabled, should the client fallback to a
        /// non-SSL connection if it experiences issues handshaking
        /// across local proxies, firewalls, etc? (default: YES)
        ignoreSecureConnectionRequirement = 0x02
    };

    Q_DECLARE_FLAGS(ssl_opts, ssl_opt)

    /** Set the authentication key to be used in this context.
     * After construction it is empty (null), thus not used.
     * Pass an empty (or null) string to stop using it.
     */
    void set_auth(QString const &auth);

    /** Returns the current auth key */
    QString auth() const
    {
        QMutexLocker lk(&d_mutex);
        return d_auth;
    }

    /** Set the user_id to be used in this context.
     * After construction it is empty (null), thus not used.
     * Pass an empty (or null) string to stop using it.
     *
     * @deprecated this is provided as a workaround for existing users.
     * Please use `set_user_id` instead.
     */
    PUBNUB_DEPRECATED void set_uuid(QString const &uuid);

    /** Set the user_id to be used in this context.
     * After construction it is empty (null), thus not used.
     * Pass an empty (or null) string to stop using it.
     */
    void set_user_id(QString const &user_id);


    /** Set the user_id with a random UUID value, according to stadard
     * v4 representation.
     *
     * @deprecated random generated uuid/user_id is deprecated.
     */
    PUBNUB_DEPRECATED void set_uuid_v4_random() {
	   QString uuid = QUuid::createUuid().toString();
        set_user_id(uuid.mid(1, uuid.size()-2));
    }
    /** Returns the current user_id value (string)
     *
     * @deprecated this is provided as a workaround for existing users.
     * Please use `user_id` instead.
     */
    PUBNUB_DEPRECATED QString uuid() const;
   
    /** Returns the current user_id value (string)*/
    QString user_id() const;


    /** Sets the Pubnub origin to use, that is, the
     * protocol (http or https) and host part of the URL.
     * By default, host is the generic host and protocol
     * is https, unless SSL support is not included in
     * your Qt.
     */
    void set_origin(QString const& origin);

    /** Returns the current origin */
    QString const& origin() const {
        QMutexLocker lk(&d_mutex);
        return d_origin;
    }

    /** Returns the string of an arrived message, or other element of the
        response to an operation/transaction. Message(s) arrive on finish
        of a subscribe operation, while for other operations this will give
        access to the whole response or the next element of the response.
        That is documented in the function that starts the operation.

        Subsequent call to this function will return the next message (if
        any). All messages are from the channel(s) the last operation was
        for.

        @note Context doesn't keep track of the channel(s) you subscribed
        to. This is a memory saving design decision, as most users won't
        change the channel(s) they subscribe too.
        */
    QString get() const;

    /** Returns all (remaining) messages from a context */
    QStringList get_all() const;

#if PUBNUB_USE_SUBSCRIBE_V2
    /** Returns the v2 message object of an arrived message. Message(s)
        arrive on finish of a subscribe_v2 operation.
        That is documented in the function that starts the operation.

        Subsequent call to this function will return the next message (if
        any). All messages are from the channel(s) the operation was for.
        Whather or not message is empty can be checked through class member
        function is_empty().
        @see pubnub_v2_message.hpp

        @note Context doesn't keep track of the channel(s) you subscribed(v2)
        to. This is a memory saving design decision, as most users won't
        change the channel(s) they subscribe_v2 too.
        */
    v2_message get_v2() const;

    /** Returns all (remaining) v2 messages from a context */
    QVector<v2_message> get_all_v2() const;
#endif
    
    /** Returns a string of a fetched subscribe operation/transaction's
        next channel. Each transaction may hold a list of channels, and
        this functions provides a way to read them.  Subsequent call to
        this function will return the next channel (if any).

        @note You don't have to read all (or any) of the channels before
        you start a new transaction.
        */
    QString get_channel() const;

    /** Returns all (remaining) channels from a context */
    QStringList get_all_channels() const;

    /** Cancels an ongoing API transaction. The outcome is not
     * guaranteed to be #PNR_CANCELLED like in other C-core based
     * APIs, because it depends on what Qt actually does with our
     * request to abort. But, in the regular case, it will be
     * #PNR_CANCELLED.
     */
    void cancel();

    /** Publish the @p message (in JSON format) on @p p channel, using the
        @p p context. This actually means "initiate a publish
        transaction".

        You can't publish if a transaction is in progress on @p p context.

        If transaction is not successful (@c PNR_PUBLISH_FAILED), you can
        get the string describing the reason for failure by calling
        last_publish_result().

        Keep in mind that the time token from the publish operation
        response is _not_ parsed by the library, just relayed to the
        user. Only time-tokens from the subscribe operation are parsed
        by the library.

        Also, for all error codes known at the time of this writing, the
        HTTP error will be set also, so the result of the Pubnub operation
        will not be @c PNR_OK (but you will still be able to get the
        result code and the description).

        @param channel The string with the channel to publish to.
        @param message The message to publish, expected to be in JSON format

        @return #PNR_STARTED on success, an error otherwise
     */
    pubnub_res publish(QString const &channel, QString const &message);

    /** Publish the @p message (in JSON format) on @p p channel, using the
        @p p context. This actually means "initiate a publish
        transaction".

        This function is similar to casual publish, but it allows you to
        specify additional options for the publish operation.

        See @publish and @p publish_options for more details.
    */
    pubnub_res publish(QString const &channel, QString const &message, publish_options& opt);


    /** Function that initiates 'publish' transaction via POST method
        @param channel The string with the channel
        @param message The message to publish, expected to be in JSON format

        @return #PNR_STARTED on success, an error otherwise
     */
    pubnub_res publish_via_post(QString const &channel, QByteArray const &message);

    /** Initiates 'publish' transaction via POST method. Function receives 'Qt Json' document.
        Helpful if you're already using Qt support for Json in your code, ensuring message
        you're sending is valid Json, unlike the case when applying the function that receives
        byte array and doesn't check whether those bytes represent sound Json.
        @param channel The string with the channel
        @param message The message to publish

        @return #PNR_STARTED on success, an error otherwise
     */
    inline pubnub_res publish_via_post(QString const &channel, QJsonDocument const &message) {
        return publish_via_post(channel, message.toJson());
    }

    /** Function that initiates 'publish' transaction via POST method with message gzipped
        if convenient.
        @param channel The string with the channel
        @param message The message to publish, expected to be in JSON format

        @return #PNR_STARTED on success, an error otherwise
     */
    pubnub_res publish_via_post_with_gzip(QString const &channel, QByteArray const &message);

    /** Initiates 'publish' transaction via POST method with message gzipped if convenient.
        Function receives 'Qt Json' document.
        Helpful if you're already using Qt support for Json in your code, ensuring message
        you're sending is valid Json, unlike the case when applying the function that receives
        byte array and doesn't check whether those bytes represent sound Json.
        @param channel The string with the channel
        @param message The message to publish

        @return #PNR_STARTED on success, an error otherwise
     */
    inline pubnub_res publish_via_post_with_gzip(QString const &channel,
                                                 QJsonDocument const &message) {
        return publish_via_post_with_gzip(channel, message.toJson());
    }

    /** Sends a signal @p message (in JSON format) on @p channel.
        This actually means "initiate a signal transaction".
        It has similar behaviour as publish, but unlike publish transaction, signal
        erases previous signal message on server(, on a given channel,) and you
        can not send any metadata.
        There can be only up to one signal message at the time. If it's not renewed
        by another signal, signal message  disappears from channel history after
        certain amount of time.
        Signal message is much shorter and its maximum length( around 500 bytes)
        is smaller than full publish message.

        You can't 'signal' if a transaction is in progress on @p p context.

        If transaction is not successful (@c PNR_PUBLISH_FAILED), you can
        get the string describing the reason for failure by calling
        pubnub_last_publish_result().

        Keep in mind that the time token from the signal operation
        response is _not_ parsed by the library, just relayed to the
        user. Only time-tokens from the subscribe operation are parsed
        by the library.

        Also, for all error codes known at the time of this writing, the
        HTTP error will be set also, so the result of the Pubnub operation
        will not be @c PNR_OK (but you will still be able to get the
        result code and the description).

        @param channel The string with the channel to signal to.
        @param message The signal message to send, expected to be in JSON format
        @return #PNR_STARTED on success, an error otherwise
    */
    pubnub_res signal(QString const &channel, QByteArray const &message);
    
    /** Subscribe to @p channel and/or @p channel_group. This actually
        means "initiate a subscribe operation/transaction". The outcome
        will be retrieved by the "notification" API, which is different
        for different platforms. There are two APIs that are widely
        available - "sync" and "callback".

        Messages published on @p channel and/or @p channel_group since the
        last subscribe transaction will be fetched, unless this is the
        first subscribe on this context after initialization or a serious
        error. In that "first" case, this will just retrieve the current
        time token, and that event is called "connect" in many pubnub
        SDKs, but in C-core we don't treat it any different. For that
        "first" case, you will receive a notification that subscribe has
        finished OK, but there will be no messages in the reply.

        The @p channel and @p channel_group strings may contain multiple
        comma-separated channel (channel group) names, so only one call is
        needed to fetch messages from multiple channels (channel groups).

        If @p channel is empty (or null), then @p channel_group cannot be
        empty (or null) and you will subscribe only to the channel group(s).
        It goes  both ways: if @p channel_group is empty, then @p channel
        cannot be empty and you will subscribe only to the channel(s).

        When auto heartbeat is enabled at compile time both @p channel
        and @p channel_group could be passed as empty strings which suggests
        default behaviour in which case transaction uses channel and
        channel groups that are already subscribed.

        You can't subscribe if a transaction is in progress on the context.

        Also, you can't subscribe if there are unread messages in the
        context (you read messages with pubnub_get()).

        @param channel The string with the channel name (or comma-delimited list
        of channel names) to subscribe to.
        @param channel_group The string with the channel group name (or
        comma-delimited list of channel group names) to subscribe to.

        @return #PNR_STARTED on success, an error otherwise

        @see get
        */
    pubnub_res subscribe(QString const &channel, QString const &channel_group="");

    /** A helper method to subscribe to several channels and or channel groups
     * by giving a (string) list of them.
     */
    pubnub_res subscribe(QStringList const &channel, QStringList const &channel_group=QStringList()) {
        return subscribe(channel.join(","), channel_group.join(","));
    }

#if PUBNUB_USE_SUBSCRIBE_V2
    /** The V2 subscribe. To get messages for subscribe V2, use pb.get_v2().
        - keep in mind that it can provide you with channel and channel group info.

        Basic usage initiating transaction:

        subscribe_v2_options opt;
        pubnub_qt pb;
        pbresult = pb.subscribe_v2(pn, "my_channel", opt.filter_expr("'key' == value"));
        ...

        When auto heartbeat is enabled at compile time both @p channel and
        channel_groups within subscribe_v2 @p opt options could be passed as empty
        strings which suggests default behaviour in which case transaction uses
        channel and channel groups that are already subscribed.

        @param channel The string with the channel name (or comma-delimited list of
                       channel names) to subscribe for.
        @param opt Subscribe V2 options
        @return #PNR_STARTED on success, an error otherwise

        @see get_v2
      */
    pubnub_res subscribe_v2(QString const &channel, subscribe_v2_options opt);

    /** A helper method to subscribe_v2 to several channels and/or channel groups
     * by giving a (string) list of channels and passing suitable options.
     */
    pubnub_res subscribe_v2(QStringList const &channel, subscribe_v2_options opt) {
        return subscribe_v2(channel.join(","), opt);
    }
#endif /* PUBNUB_USE_SUBSCRIBE_V2 */

    /** Leave the @p channel. This actually means "initiate a leave
        transaction".  You should leave channel(s) when you want to
        subscribe to another in the same context to avoid loosing
        messages. Also, it is useful for tracking presence.

        When auto heartbeat is enabled at compile time both @p channel
        and @p channel group could be passed as empty strings which
        suggests default behaviour in which case transaction uses channel
        and channel groups that are already subscribed and leaves them all.

        You can't leave if a transaction is in progress on the context.

        @param channel The string with the channel name (or
        comma-delimited list of channel names) to leave from.
        @param channel_group The string with the channel group name (or
        comma-delimited list of channel group names) to leave from.

        @return #PNR_STARTED on success, an error otherwise
    */
    pubnub_res leave(QString const &channel, QString const &channel_group="");

    /** A helper method to leave from several channels and or channel groups
     * by giving a (string) list of them.
     */
    pubnub_res leave(QStringList const &channel, QStringList const &channel_group=QStringList()) {
        return leave(channel.join(","), channel_group.join(","));
    }

    /** Get the current Pubnub time token . This actually means "initiate
        a time transaction". Since time token is in the response to most
        Pubnub REST API calls, this is reserved mostly when you want to
        get a high-quality seed for a random number generator, or some
        such thing.

        If transaction is successful, the gotten time will be the only
        message you can get with get(). It will be a (large) JSON
        integer.

        You can't get time if a transaction is in progress on the context.

        @return #PNR_STARTED on success, an error otherwise
    */
    pubnub_res time();

    /** Get the message history for the @p channel. This actually means
        "initiate a history transaction/operation".

        If transaction is successful, the gotten messages will be
        available via the get(), yielding exactly
        three messages (or, rather, elements).  The first will be a JSON
        array of gotten messages, and the second and third will be the
        timestamps of the first and the last message from that array.

        Also, if you select to @c include_token, then the JSON array
        you get will not be a simple array of gotten messages, but
        rather an array of JSON objects, having keys `message` with
        value the actual message, and `timetoken` with the time token
        of that particular message.

        You can't get history if a transaction is in progress on the context.

        @param channel The string with the channel name to get message
        history for. This _can't_ be a comma separated list of channels.
        @param count Maximum number of messages to get. If there are less
        than this available on the @c channel, you'll get less, but you
        can't get more.
        @param include_token If true, include the time token for every
        gotten message

        @return #PNR_STARTED on success, an error otherwise
    */
    pubnub_res history(QString const &channel, unsigned count = 100, bool include_token = false);

    /** This is the "extended" history, with all the options and no
        defaults.

        @param channel The string with the channel name to get message
        history for. This _can't_ be a comma separated list of channels.

        @param count Maximum number of messages to get. If there are less
        than this available on the @c channel, you'll get less, but you
        can't get more.

        @param include_token If true, include the time token for every
        gotten message

        @param start Lets you select a “start date”, in Timetoken
        format. If not provided, it will default to current time. Page
        through results by providing a start OR end time
        token. Retrieve a slice of the time line by providing both a
        start AND end time token. start is ‘exclusive’ – that is, the
        first item returned will be the one immediately after the
        start Timetoken value.

        @param reverse Direction of time traversal. False means
        timeline is traversed newest to oldest.

        @param include_meta If true, transaction response will include
        metadata for every gotten message

        @param end Lets you select an “end date”, in Timetoken
        format. If not provided, it will provide up to the number of
        messages defined in the “count” parameter. Page through
        results by providing a start OR end time token. Retrieve a
        slice of the time line by providing both a start AND end time
        token. End is ‘exclusive’ – that is, if a message is
        associated exactly with the end Timetoken, it will be included
        in the result.

        @param string_token If false, the returned start and end
        Timetoken values will be returned as long ints. If 1, or true,
        the start and end Timetoken values will be returned as
        strings.

        @return #PNR_STARTED on success, an error otherwise
     */
    pubnub_res history(QString const &channel,
                       unsigned count,
                       bool include_token,
                       QString const& start,
                       bool reverse,
                       bool include_meta,
                       QString const& end,
                       bool string_token);

    /* In case the server reported en error in the response,
       we'll read the error message using this function
       @retval error_message on successfully read error message,
       @retval empty_string otherwise
     */
    QString get_error_message();

    /* Get counts of received(unread) messages for each channel from
       @p channel list starting(in time) with @p timetoken(Meanning
       'initiates 'advanced history' message_counts operation/transaction')

       If successful message will be available through get_channel_message_counts()
       in the map of channel_name-message_counts
       @return #PNR_STARTED on success, an error otherwise
     */
    pubnub_res message_counts(QString const& channel, QString const& timetoken);

    /* Get counts of received(unread) messages for each channel from
       @p channel list starting(in time) with @p timetoken(Meanning
       'initiates 'advanced history' message_counts operation/transaction')

       If successful message will be available through get_channel_message_counts()
       in the map of channel_name-message_counts
       @return #PNR_STARTED on success, an error otherwise
     */
    pubnub_res message_counts(QStringList const& channel, QString const& timetoken);

    /* Get counts of received(unread) messages for each channel from
       @p channel list starting(in time) with @p channel_timetoken(per channel) list.
       (Meanning 'initiates 'advanced history' message_counts operation/transaction')

       If successful message will be available through get_channel_message_counts()
       in the map of channel_name-message_counts
       @return #PNR_STARTED on success, an error otherwise
     */
    pubnub_res message_counts(QString const& channel,
                              QStringList const& channel_timetoken);

    /* Get counts of received(unread) messages for each channel from
       @p channel list starting(in time) with @p channel_timetoken(per channel) list.
       (Meanning 'initiates 'advanced history' message_counts operation/transaction')

       If successful message will be available through get_channel_message_counts()
       in the map of channel_name-message_counts
       @return #PNR_STARTED on success, an error otherwise
     */
    pubnub_res message_counts(QStringList const& channel,
                              QStringList const& channel_timetoken);

    /* Starts 'advanced history' pubnub_message_counts transaction
       for unread messages on @p channel_timetokens(channel, ch_timetoken pairs)

       If successful message will be available through get_channel_message_counts()
       in the map of channel_name-message_counts
       @return #PNR_STARTED on success, an error otherwise
     */
    pubnub_res message_counts(QVector<QPair<QString, QString>> const& channel_timetokens);

    /* Extracts channel-message_count paired map from the response on
       'advanced history' pubnub_message_counts transaction.
       If there is no key "channels" in the response, or the corresponding
       json value is empty returns an empty map.
     */
    QMap<QString, size_t> get_channel_message_counts();

    /* Inform Pubnub that we're still working on @p channel and/or @p
       channel_group.  This actually means "initiate a heartbeat
       transaction". It can be thought of as an update against the
       "presence database".

       If transaction is successful, the response will be a available
       via get() as one message, a JSON object. Following keys
       are always present:
       - "status": the HTTP status of the operation (200 OK, 40x error, etc.)
       - "message": the string/message describing the status ("OK"...)
       - "service": should be "Presence"

       If @p channel is empty, then @p channel_group cannot be and
       you will subscribe only to the channel group(s). It goes both ways:
       if @p channel_group is empty, then @p channel can't be and
       you will subscribe only to the channel(s).

       When auto heartbeat is enabled at compile time both @p channel
       and @p channel_group could be passed as empty strings which
       suggests default behaviour in which case transaction uses channel
       and channel groups that are already subscribed.

       You can't initiate heartbeat if a transaction is in progress
       on the context.

       @param channel The string with the channel name (or comma-delimited
                      list of channel names) to get presence info for.
       @param channel_group The string with the channel group name (or
                            comma-delimited list of channel group names)
                            to get presence info for.
       @return #PNR_STARTED on success, an error otherwise
     */
    pubnub_res heartbeat(QString const& channel, QString const& channel_group = "");

    /** A helper method for heartbeat on several channels and/or channel groups
     * by giving a (string) list of channels .
     */
    pubnub_res heartbeat(QStringList const& channel, QString const& channel_group = "") {
        return heartbeat(channel.join(","), channel_group);
    }

    /** A helper method for heartbeat on several channels and/or channel groups
     * by giving a (string) list of channel groups.
     */
    pubnub_res heartbeat(QString const& channel, QStringList const& channel_group) {
        return heartbeat(channel, channel_group.join(","));
    }

    /** A helper method for heartbeat on several channels and/or channel groups
     * by giving a (string) lists of them.
     */
    pubnub_res heartbeat(QStringList const& channel, QStringList const& channel_group) {
        return heartbeat(channel, channel_group.join(","));
    }

    /** Get the currently present users on a @p channel and/or @p
        channel_group. This actually means "initiate a here_now
        transaction". It can be thought of as a query against the
        "presence database".

        If transaction is successful, the response will be a available
        via get() as one message, a JSON object. Following keys
        are always present:
        - "status": the HTTP status of the operation (200 OK, 40x error, etc.)
        - "message": the string/message describing the status ("OK"...)
        - "service": should be "Presence"

        If doing a query on a single channel, following keys are present:
        - "uuids": an array of UUIDs of currently present users
        - "occupancy": the number of currently present users in the channel

        If doing a query on more channels, a key "payload" is present,
        which is a JSON object whose keys are:

        - "channels": a JSON object with keys being the names of the
        channels and their values JSON objects with keys "uuids" and
        "occupancy" with the meaning the same as for query on a single
        channel
        - "total_channels": the number of channels for which the
        presence is given (in "payload")
        - "total_occupancy": total number of users present in all channels

        If @p channel is empty or null, then @p channel_group cannot be
        empty or null and you will get presence info only for the channel group.
        It goes both ways: if @p channel_group is empty, then @p channel
        cannot be empty and you will get presence info only for the channel.

        You can't get list of currently present users if a transaction is
        in progress on the context.

        @param channel The string with the channel name (or
        comma-delimited list of channel names) to get presence info for.
        @param channel_group The string with the channel name (or
        comma-delimited list of channel group names) to get presence info for.

        @return #PNR_STARTED on success, an error otherwise
    */
    pubnub_res here_now(QString const &channel, QString const &channel_group="");

    /** A helper method to get "here now" info from several channels
     * and or channel groups by giving a (string) list of them.
     */
    pubnub_res here_now(QStringList const &channel, QStringList const &channel_group=QStringList()) {
        return here_now(channel.join(","), channel_group.join(","));
    }

    /** Get the currently present users on all channel. This actually
        means "initiate a global here_now transaction". It can be thought
        of as a query against the "presence database".

        If transaction is successful, the response will be the same
        as for "multi-channel" response for pubnub_here_now(), if
        we queried against all currently available channels.

        You can't get list of currently present users if a transaction is
        in progress on the context.

        @param channel The string with the channel name (or
        comma-delimited list of channel names) to get presence info for.

        @return #PNR_STARTED on success, an error otherwise
    */
    pubnub_res global_here_now();

    /** Get the currently present users on a @p channel and/or @p
        channel_group. This actually means "initiate a here_now
        transaction". It can be thought of as a query against the
        "presence database".

        If transaction is successful, the response will be a available
        via pubnub_get() as one message, a JSON object with keys:

        - "status": the HTTP status of the operation (200 OK, 40x error, etc.)
        - "message": the string/message describing the status ("OK"...)
        - "service": should be "Presence"
        - "payload": JSON object with a key "channels" which is an
        array of channels this user is present in

        You can't get channel presence for the user if a transaction is
        in progress on the context.

        @param uuid The UUID of the user to get the channel presence.
        If empty or null, the current UUID of the context will be used.

        @return #PNR_STARTED on success, an error otherwise
    */
    pubnub_res where_now(QString const &uuid="");

    /** Sets some state for the @p channel and/or @channel_group for a
        user, identified by @p uuid. This actually means "initiate a set
        state transaction". It can be thought of as an update against the
        "presence database".

        "State" has to be a JSON object (IOW, several "key-value" pairs).

        If transaction is successful, the response will be a available
        via pubnub_get() as one message, a JSON object with following keys:
        - "status": the HTTP status of the operation (200 OK, 40x error, etc.)
        - "message": the string/message describing the status ("OK"...)
        - "service": should be "Presence"
        - "payload" the state

        This will set the same state to all channels identified by
        @p channel and @p channel_group.

        If @p channel is empty or null, then @p channel_group cannot be
        empty or null and you will set state only for the channel group.
        It goes both ways: if @p channel_group is empty, then @p channel
        cannot be empty and you will set state only for the channel.

        You can't set state of channels if a transaction is in progress on
        the context.

        @param channel The string with the channel name (or
        comma-delimited list of channel names) to set state for.
        @param channel_group The string with the channel name (or
        comma-delimited list of channel group names) to set state for.
        @param uuid The UUID of the user for which to set state for.
        If empty or null, the current UUID of the context will be used.
        @param state Has to be a JSON object

        @return #PNR_STARTED on success, an error otherwise
    */
    pubnub_res set_state(QString const &channel, QString const& channel_group, QString const &uuid, QString const &state);

    /** A helper method to set state from several channels
     * and or channel groups by giving a (string) list of them.
     */
    pubnub_res set_state(QStringList const &channel, QStringList const& channel_group, QString const &uuid, QString const &state) {
        return set_state(channel.join(","), channel_group.join(","), uuid, state);
    }

    /** Sets some state for the @p channel and/or channel_group defined
        in @options for a user, identified by @p uuid or provided in @options.
        This actually means "initiate a set state transaction".
        It can be thought of as an update against the "presence database".

        "State" has to be a JSON object (IOW, several "key-value" pairs).

        If transaction is successful, the response will be a available
        via pubnub_get() as one message, a JSON object with following keys:
        - "status": the HTTP status of the operation (200 OK, 40x error, etc.)
        - "message": the string/message describing the status ("OK"...)
        - "service": should be "Presence"
        - "payload" the state

        This will set the same state to all channels identified by
        @p channel and @p channel_group.

        If @p channel is empty or null, then @p channel_group cannot be
        empty or null and you will set state only for the channel group.
        It goes both ways: if @p channel_group is empty, then @p channel
        cannot be empty and you will set state only for the channel.

        You can't set state of channels if a transaction is in progress on
        the context.

        @param channel The string with the channel name (or
        comma-delimited list of channel names) to set state for.
        @param state Has to be a JSON object
        @options set_state options provided by user

        @return #PNR_STARTED on success, an error otherwise
    */
    pubnub_res set_state(QString const &channel, QString const &state, set_state_options &options);

    /** A helper method to set state from several channels
     * and or channel groups by giving a (string) list of them.
     */
    pubnub_res set_state(QStringList const &channel, QString const &state, set_state_options &options) {
        return set_state(channel.join(","), state, options);
    }




    /** Gets some state for the @p channel and/or @channel_group for a
        user, identified by @p uuid. This actually means "initiate a get
        state transaction". It can be thought of as a query against the
        "presence database".

        If transaction is successful, the response will be a available
        via pubnub_get() as one message, a JSON object with following keys:
        - "status": the HTTP status of the operation (200 OK, 40x error, etc.)
        - "message": the string/message describing the status ("OK"...)
        - "service": should be "Presence"
        - "payload": if querying against one channel the gotten state
        (a JSON object), otherwise a JSON object with the key "channels"
        whose value is a JSON object with keys the name of the channels
        and their respective values JSON objects of the gotten state

        If @p channel is empty or null, then @p channel_group cannot be
        empty or null and you will get state only for the channel group.
        It goes both ways: if @p channel_group is empty, then @p channel
        cannot be empty and you will get state only for the channel.

        You can't set state of channels if a transaction is in progress on
        the context.

        @param channel The string with the channel name (or
        comma-delimited list of channel names) to set state for.
        @param channel_group The string with the channel name (or
        comma-delimited list of channel group names) to set state for.
        @param uuid The UUID of the user for which to set state for.
        If empty or null, the current UUID of the context will be used.

        @return #PNR_STARTED on success, an error otherwise
    */
    pubnub_res state_get(QString const &channel, QString const& channel_group="", QString const &uuid="");

    /** A helper method to get state from several channels
     * and or channel groups by giving a (string) list of them.
     */
    pubnub_res state_get(QStringList const &channel, QStringList const& channel_group=QStringList(), QString const &uuid="") {
        return state_get(channel.join(","), channel_group.join(","), uuid);
    }

    /** Removes a @p channel_group and all its channels. This actually
        means "initiate a remove_channel_group transaction". It can be
        thought of as an update against the "channel group database".

        If transaction is successful, the response will be a available via
        pubnub_get_channel() as one "channel", a JSON object with keys:

        - "service": should be "channel-registry"
        - "status": the HTTP status of the operation (200 OK, 40x error, etc.)
        - "error": true on error, false on success
        - "message": the string/message describing the status ("OK"...)

        You can't remove a channel group if a transaction is in progress
        on the context.

        @param channel_group The channel group to remove

        @return #PNR_STARTED on success, an error otherwise
    */
    pubnub_res remove_channel_group(QString const& channel_group);

    /** Removes a @p channel from the @p channel_group . This actually
        means "initiate a remove_channel_from_channel_group
        transaction". It can be thought of as an update against the
        "channel group database".

        You can't remove the last channel from a channel group. To do
        that, remove the channel group itself.

        If transaction is successful, the response will be a available via
        pubnub_get_channel() as one "channel", a JSON object with keys:

        - "service": should be "channel-registry"
        - "status": the HTTP status of the operation (200 OK, 40x error, etc.)
        - "error": true on error, false on success
        - "message": the string/message describing the status ("OK"...)

        You can't remove a channel from a channel group if a transaction
        is in progress on the context.

        @param channel_group The channel to remove
        @param channel_group The channel group to remove from

        @return #PNR_STARTED on success, an error otherwise
    */
    pubnub_res remove_channel_from_group(QString const& channel, QString const& channel_group);

    /** A helper method to remove channels from channel group by
     * giving (string) lists of them.
     */
    pubnub_res remove_channel_from_group(QStringList const& channel, QString const& channel_group) {
        return remove_channel_from_group(channel.join(","), channel_group);
    }

    /** Adds a @p channel to the @p channel_group . This actually means
        "initiate a add_channel_to_channel_group transaction". It can be
        thought of as an update against the "channel group database".

        If the channel group doesn't exist, this implicitly adds (creates)
        it.

        If transaction is successful, the response will be a available
        via pubnub_get_channel() as one "channel", a JSON object with keys:

        - "service": should be "channel-registry"
        - "status": the HTTP status of the operation (200 OK, 40x error, etc.)
        - "error": true on error, false on success
        - "message": the string/message describing the status ("OK"...)

        You can't add a channel to a channel group if a transaction
        is in progress on the context.

        @param channel The channel to add
        @param channel_group The channel group to add to

        @return #PNR_STARTED on success, an error otherwise
    */
    pubnub_res add_channel_to_group(QString const& channel, QString const& channel_group);

    /** A helper method to add channels to channel group by
     * giving (string) lists of them.
     */
    pubnub_res add_channel_to_group(QStringList const& channel, QString const& channel_group) {
        return add_channel_to_group(channel.join(","), channel_group);
    }

    /** Lists all channels of a @p channel_group. This actually
        means "initiate a list_channel_group transaction". It can be
        thought of as a query against the "channel group database".

        If transaction is successful, the response will be a available via
        pubnub_get_channel() as one "channel", a JSON object with keys:

        - "service": should be "channel-registry"
        - "status": the HTTP status of the operation (200 OK, 40x error, etc.)
        - "error": true on error, false on success
        - "payload": JSON object with keys "group" with value the string
        of the channel group name and "channels" with value a JSON array
        of strings with names of the channels that belong to the group

        You can't remove a channel group if a transaction is in progress
        on the context.

        @param channel_group The channel group to list

        @return #PNR_STARTED on success, an error otherwise
    */
    pubnub_res list_channel_group(QString const& channel_group);

#if PUBNUB_USE_OBJECTS_API

    /** Initiates transaction that returns the space memberships of the user specified
        by @p user_id, optionally including the custom data objects for...

        If transaction is successful, the response(a JSON object) will have key
        "data" with corresponding value. If not, there should be "error" key 'holding'
        error description. If there is neither of the two keys mentioned, response parsing
        function returns response format error.
        Complete answer will be available via pubnub_get().

        @param user_id The User ID for which to retrieve the space memberships for.
        @param options options for manipulating specified requirements
                       and paginated response
        @return #PNR_STARTED on success, an error otherwise
      */
    pubnub_res get_memberships(QString const& user_id, list_options& options);

    /** Initiates transaction that updates the space memberships for the user specified
        by @p user_id. Uses the `update` property on the @p set_obj to perform that
        operations on one, or more memberships.
        An example for @set_obj:
          [
            {
              "id": "main-space-id",
              "custom": {
                "starred": true
              }
            },
            {
              "id": "space-0",
              "some_key": {
                "other_key": "other_value"
              }
            }
          ]
    
        If transaction is successful, the response(a JSON object) will have key
        "data" with corresponding value. If not, there should be "error" key 'holding'
        error description. If there is neither of the two keys mentioned, response parsing
        function returns response format error.
        Complete answer will be available via pubnub_get().
        Contains the user's space memberships, optionally including the custom data objects.

        @param user_id The User ID for which to update the space memberships for.
        @param set_obj The JSON object that defines the updates to perform.
        @param include string with additional/complex attributes to include in response.
                       Use empty list if you don't want to retrieve additional attributes.
        @return #PNR_STARTED on success, an error otherwise
      */
    pubnub_res set_memberships(QString const& metadata_uuid,
                                  QByteArray const& set_obj,
                                  QString& include);

    /** Initiates transaction that updates the space memberships for the user specified
        by @p user_id. Uses the `update` property on the @p set_obj to perform that
        operation on one, or more memberships.
        An example for @set_obj:
          [
            {
              "id": "my-space-id"
              "some_key": {
                "other_key": "other_value"
              }
            },
            {
              "id": "main-space-id",
              "custom": {
                "starred": true
              }
            }
          ]

        Function receives 'Qt Json' document.
        Helpful if you're already using Qt support for Json in your code, ensuring message
        you're sending is valid Json, unlike the case when applying the function that
        receives byte array and doesn't check whether those bytes represent sound Json.
    
        If transaction is successful, the response(a JSON object) will have key
        "data" with corresponding value. If not, there should be "error" key 'holding'
        error description. If there is neither of the two keys mentioned, response parsing
        function returns response format error.
        Complete answer will be available via pubnub_get().
        Contains the user's space memberships, optionally including the custom data objects.

        @param user_id The User ID for which to update the space memberships for.
        @param set_obj The JSON object that defines the updates to perform.
        @param include string with additional/complex attributes to include in response.
                       Use empty list if you don't want to retrieve additional attributes.
        @return #PNR_STARTED on success, an error otherwise
      */
    pubnub_res set_memberships(QString const& user_id,
                                  QJsonDocument const& set_obj,
                                  QString& include) {
        return set_memberships(user_id, set_obj.toJson(), include);
    }

    /** Initiates transaction that returns all users in the space specified by @p space_id,
        optionally including the custom data objects for...

        If transaction is successful, the response(a JSON object) will have key
        "data" with corresponding value. If not, there should be "error" key 'holding'
        error description. If there is neither of the two keys mentioned, response parsing
        function returns response format error.
        Complete answer will be available via pubnub_get().

        @param space_id The Space ID for which to retrieve all users in the space.
        @param options options for manipulating specified requirements
                       and paginated response
        @return #PNR_STARTED on success, an error otherwise
      */
    pubnub_res get_members(QString const& space_id, list_options& options);

    /** Initiates transaction that adds the list of members to the space specified by
        @p space_id. Use the `add` property on the @p update_obj to perform that
        operation on one or more members.
        An example for @update_obj:
          [
            {
              "id": "user-1-id"
            },
            {
              "id": "user-2-id",
              "custom": {
                "role": “moderator”
              }
            }
          ]

        If transaction is successful, the response(a JSON object) will have key
        "data" with corresponding value. If not, there should be "error" key 'holding'
        error description. If there is neither of the two keys mentioned, response parsing
        function returns response format error.
        Complete answer will be available via pubnub_get().
        Contains the spaces's memberships, optionally including the custom data objects for...

        @param space_id The Space ID for which to add the list of members to the space.
        @param update_obj The JSON object that defines the updates to perform.
        @param include string with additional/complex attributes to include in response.
                       Use empty list if you don't want to retrieve additional attributes.
        @return #PNR_STARTED on success, an error otherwise
      */
    pubnub_res add_members(QString const& space_id,
                           QByteArray const& update_obj,
                           QString& include);

    /** Initiates transaction that adds the list of members to the space specified by
        @p space_id. Uses the `add` property on the @p update_obj to perform that
        operation on one or more members.
        An example for @update_obj:
          [
            {
              "id": "user-2-id",
              "custom": {
                "role": “moderator”
              }
            },
            {
              "id": "user-0-id"
            }
          ]

        Function receives 'Qt Json' document.
        Helpful if you're already using Qt support for Json in your code, ensuring message
        you're sending is valid Json, unlike the case when applying the function that
        receives byte array and doesn't check whether those bytes represent sound Json.

        If transaction is successful, the response(a JSON object) will have key
        "data" with corresponding value. If not, there should be "error" key 'holding'
        error description. If there is neither of the two keys mentioned, response parsing
        function returns response format error.
        Complete answer will be available via pubnub_get().
        Contains the spaces's memberships, optionally including the custom data objects for...

        @param space_id The Space ID for which to add the list of members to the space.
        @param update_obj The JSON object that defines the updates to perform.
        @param include string with additional/complex attributes to include in response.
                       Use empty list if you don't want to retrieve additional attributes.
        @return #PNR_STARTED on success, an error otherwise
      */
    pubnub_res add_members(QString const& space_id,
                           QJsonDocument const& update_obj,
                           QString& include) {
        return add_members(space_id, update_obj.toJson(), include);
    }

    /** Initiates transaction that updates the list of members in the space specified by
        @p space_id. Use the `update` property on the @p update_obj to perform that
        operation on one or more members.
        An example for @update_obj:
          [
            {
              "id": "user-1-id"
              "custom": {
                "starred": true
              }
            },
            {
              "id": "user-2-id",
              "custom": {
                "role": “moderator”
              }
            }
          ]

        If transaction is successful, the response(a JSON object) will have key
        "data" with corresponding value. If not, there should be "error" key 'holding'
        error description. If there is neither of the two keys mentioned, response parsing
        function returns response format error.
        Complete answer will be available via pubnub_get().
        Contains the spaces's memberships, optionally including the custom data objects for...

        @param space_id The Space ID for which to update the list of members in the space.
        @param update_obj The JSON object that defines the updates to perform.
        @param include string with additional/complex attributes to include in response.
                       Use empty list if you don't want to retrieve additional attributes.
        @return #PNR_STARTED on success, an error otherwise
      */
    pubnub_res update_members(QString const& space_id,
                              QByteArray const& update_obj,
                              QString& include);

    /** Initiates transaction that updates the list of members in the space specified by
        @p space_id. Uses the `update` property on the @p update_obj to perform that
        operation on one or more members.
        An example for @update_obj:
          [
            {
              "id": "user-2-id",
              "custom": {
                "role": “moderator”
              }
            },
            {
              "id": "user-0-id"
              "custom": {
                "starred": true
              }
            }
          ]

        Function receives 'Qt Json' document.
        Helpful if you're already using Qt support for Json in your code, ensuring message
        you're sending is valid Json, unlike the case when applying the function that
        receives byte array and doesn't check whether those bytes represent sound Json.

        If transaction is successful, the response(a JSON object) will have key
        "data" with corresponding value. If not, there should be "error" key 'holding'
        error description. If there is neither of the two keys mentioned, response parsing
        function returns response format error.
        Complete answer will be available via pubnub_get().
        Contains the spaces's memberships, optionally including the custom data objects for...

        @param space_id The Space ID for which to update the list of members in the space.
        @param update_obj The JSON object that defines the updates to perform.
        @param include string with additional/complex attributes to include in response.
                       Use empty list if you don't want to retrieve additional attributes.
        @return #PNR_STARTED on success, an error otherwise
      */
    pubnub_res update_members(QString const& space_id,
                              QJsonDocument const& update_obj,
                              QString& include) {
        return update_members(space_id, update_obj.toJson(), include);
    }

    /** Initiates transaction that removes the list of members from the space specified by
        @p space_id. Use the `remove` property on the @p update_obj to perform that
        operation on one or more members.
        An example for @update_obj:
          [
            {
              "id": "user-1"
            },
            {
              "id": "user-2",
              "custom": {
                "role": “moderator”
              }
            }
          ]

        If transaction is successful, the response(a JSON object) will have key
        "data" with corresponding value. If not, there should be "error" key 'holding'
        error description. If there is neither of the two keys mentioned, response parsing
        function returns response format error.
        Complete answer will be available via pubnub_get().
        Contains the spaces's memberships, optionally including the custom data objects for...

        @param space_id The Space ID for which to remove the list of members from the space.
        @param update_obj The JSON object that defines the updates to perform.
        @param include string with additional/complex attributes to include in response.
                       Use empty list if you don't want to retrieve additional attributes.
        @return #PNR_STARTED on success, an error otherwise
      */
    pubnub_res remove_members(QString const& space_id,
                              QByteArray const& update_obj,
                              QString& include);

    /** Initiates transaction that removes the list of members from the space specified by
        @p space_id. Uses the `remove` property on the @p update_obj to perform that
        operation on one or more members.
        An example for @update_obj:
          [
            {
              "id": "user-2",
              "custom": {
                "role": “moderator”
              }
            },
            {
              "id": "user-0"
            }
          ]

        Function receives 'Qt Json' document.
        Helpful if you're already using Qt support for Json in your code, ensuring message
        you're sending is valid Json, unlike the case when applying the function that
        receives byte array and doesn't check whether those bytes represent sound Json.

        If transaction is successful, the response(a JSON object) will have key
        "data" with corresponding value. If not, there should be "error" key 'holding'
        error description. If there is neither of the two keys mentioned, response parsing
        function returns response format error.
        Complete answer will be available via pubnub_get().
        Contains the spaces's memberships, optionally including the custom data objects for...

        @param space_id The Space ID for which to remove the list of members from the space.
        @param update_obj The JSON object that defines the updates to perform.
        @param include string with additional/complex attributes to include in response.
                       Use empty list if you don't want to retrieve additional attributes.
        @return #PNR_STARTED on success, an error otherwise
      */
    pubnub_res remove_members(QString const& space_id,
                              QJsonDocument const& update_obj,
                              QString& include) {
        return remove_members(space_id, update_obj.toJson(), include);
    }
#endif /* PUBNUB_USE_OBJECTS_API */

#if PUBNUB_USE_ACTIONS_API
    /** Adds new type of message called action as a support for user reactions on a published
        messages.
        Json string @p value is checked for its quotation marks at its ends. If any of the
        quotation marks is missing function returns parameter error.
        If the transaction is finished successfully response will have 'data' field with
        added action data. If there is no data, nor error description in the response,
        response parsing function returns format error.
        @param channel The channel on which action is referring to.
        @param message_timetoken The timetoken(unquoted) of a published message action is
                                 applying to
        @param actype Action type
        @param value Json string describing the action that is to be added
        @return #PNR_STARTED on success, an error otherwise
      */
    pubnub_res add_message_action(QString const& channel,
                                  QString const& message_timetoken,
                                  pubnub_action_type actype,
                                  QString const& value);

    /** Function receives 'Qt Json' document for @p value. Helpful if you're already using Qt
        support for Json in your code, ensuring that value you are passing is valid Json.
        @see add_message_action()
      */
    pubnub_res add_message_action(QString const& channel,
                                  QString const& message_timetoken,
                                  pubnub_action_type actype,
                                  QJsonDocument const& value) {
        return add_message_action(channel, message_timetoken, actype, value.toJson());
    }

    /** Searches the response, if previous transaction had been 'add_message_action' and was
        accomplished successfully) and retrieves timetoken of a message action was added on.
        If key expected is not found, preconditions(about right transaction) were not fulfilled,
        or error was encountered, returnes an empty string.
        @return Message timetoken value including its quotation marks
      */
    QString get_message_timetoken();

    /** Searches the response, if previous transaction had been 'add_message_action' and was
        accomplished successfully) and retrieves timetoken of a resently added action.
        If key expected is not found, preconditions were not fulfilled, or error was encountered,
        returnes an empty string.
        @return Action timetoken value including its quotation marks
      */
    QString get_message_action_timetoken();

    /** Initiates transaction that deletes(removes) previously added action on a published message.
        If there is no success confirming data, nor error description in the response it is
        considered format error.
        @param channel The channel on which action was previously added.
        @param message_timetoken The timetoken of a published message action was applied to.
                                 (Obtained from the response when action was added and expected
                                  with its quotation marks at both ends)
        @param action_timetoken The action timetoken when it was added(Gotten from the transaction
                                response when action was added and expected with its quotation
                                marks at both ends)
        @return #PNR_STARTED on success, an error otherwise
      */
    pubnub_res remove_message_action(QString const& channel,
                                     QString const& message_timetoken,
                                     QString const& action_timetoken);

    /** Initiates transaction that returns all actions added on a given @p channel between @p start
        and @p end action timetoken.
        The response to this transaction can be partial and than it contains the hyperlink string
        value to the rest.
        @see get_message_actions_more()
        If there is no actions data, nor error description in the response it is considered
        format error.
        @param channel The channel on which actions are observed.
        @param start Start action timetoken(unquoted). Can be an empty string meaning there is
                     no lower limitation in time.
        @param end End action timetoken(unquoted). Can be an empty string in which case upper
                   time limit is present moment.
        @param limit Number of actions to return in response. Regular values 1 - 100. If you set `0`,
                     that means “use the default”. At the time of this writing, default was 100.
                     Any value greater than 100 is considered an error.
        @return #PNR_STARTED on success, an error otherwise
      */
    pubnub_res get_message_actions(QString const& channel,
                                   QString const& start,
                                   QString const& end,
                                   size_t limit=0);

    /** This function expects previous transaction to be the one for reading the actions and
        that it was successfully accomplished. If it is not the case, returns corresponding
        error.
        When preconditions are fulfilled, it searches the hyperlink to the rest in the existing
        response buffer which it uses for obtaining another part of the server response.
        Anotherwords, once the hyperlink is found in the existing response it is used for
        initiating new request and function than behaves, essentially, as get_message_actions().
        If there is no hyperlink encountered in the previous transaction response it
        returns success: PNR_GOT_ALL_ACTIONS meaning that the answer is complete.
        @retval PNR_STARTED transaction successfully initiated.
        @retval PNR_GOT_ALL_ACTIONS transaction successfully finished.
        @retval corresponding error otherwise
      */
    pubnub_res get_message_actions_more();

    /** Initiates transaction that returns all actions added on a given @p channel between @p start
        and @p end message timetoken.
        The response to this transaction can be partial and than it contains the hyperlink string
        value to the rest.
        @see history_with_message_actions_more()
        If there is no actions data, nor error description in the response it is considered
        format error.
        @param channel The channel on which actions are observed.
        @param start Start message timetoken(unquoted). Can be an empty string meaning there is
                     no lower limitation in time.
        @param end End message timetoken(unquoted). Can be an empty string in which case upper
                   time limit is present moment.
        @param limit Number of messages to return in response. Regular values 1 - 100. If you
                     set `0`, that means “use the default”. At the time of this writing, default
                     was 100. Any value greater than 100 is considered an error.
        @return #PNR_STARTED on success, an error otherwise
      */
    pubnub_res history_with_message_actions(QString const& channel,
                                            QString const& start,
                                            QString const& end,
                                            size_t limit=0);

    /** This function expects previous transaction to be the one for reading the history with
        actions and that it was successfully accomplished. If it is not the case, returns
        corresponding error.
        When preconditions are fulfilled it searches for the hyperlink to the rest of the answer
        in the existing response buffer which it uses for obtaining another part of the server
        response. Anotherwords, once the hyperlink is found in the existing response it is used
        for initiating new request and function than behaves as history_with_message_actions().
        If there is no hyperlink encountered in the previous transaction response it
        returns success: PNR_GOT_ALL_ACTIONS meaning that the answer is complete.
        @retval PNR_STARTED transaction successfully initiated.
        @retval PNR_GOT_ALL_ACTIONS transaction successfully finished.
        @retval corresponding error otherwise
      */
    pubnub_res history_with_message_actions_more();
#endif /* PUBNUB_USE_ACTIONS_API */

#if PUBNUB_USE_AUTO_HEARTBEAT
    /** Enables keeping presence on subscribed channels and channel groups
     * by performing heartbeat transacton periodically
     * @param period_sec auto heartbeat period in seconds. If it is shorter than minimal
                         permitted uses minimal permitted 
     * @return 0  OK, -1 auto heartbeat not supported
     */
    int enable_auto_heartbeat(size_t period_sec);

    /** Sets(changes) heartbeat period for auto heartbeat on subscribed channels
     * and channel groups. Returns an error if auto heartbeat is disabled.
     * @param period_sec new auto heartbeat period in seconds. If it is shorter than
                         minimal permitted uses minimal permitted
     * @return 0  OK, -1 auto heartbeat is disabled
     */
    int set_heartbeat_period(size_t period_sec);

    /** Disables auto heartbeat on subscribed channels and channel groups
     */
    void disable_auto_heartbeat();

    /** Returns whether(, or not) auto heartbeat on subscribed channels and channel
     * groups is enabled
     */
    bool is_auto_heartbeat_enabled();
#endif /* PUBNUB_USE_AUTO_HEARTBEAT */

    /** Returns the HTTP code of the last transaction. If the
     *  transaction was succesfull, will return 0.
     */
    int last_http_code() const;

    /** Return the result string that Pubnub returned in the
     * reply to the last publish transaction. If the last
     * transaction was not a publish one, will return an
     * empty string, as will do on any other kind of error.
     */
    QString last_publish_result() const;

    /** Return the result of parsing the last publish transaction's
     * response from the server.
     */
    pubnub_publish_res parse_last_publish_result();

    /** Returns the time token of the last subscribe
     * operation. After init or a serious error,
     * this will be "0".
     */
    QString last_time_token() const;

    /** Use HTTP Keep-Alive on the context for subsequent transactions.
     */
    void use_http_keep_alive() {
        d_use_http_keep_alive = true;
    }

    /** Don't use HTTP Keep-Alive on the context for subsequent
     * transactions.
     */
    void dont_use_http_keep_alive() {
        d_use_http_keep_alive = false;
    }

#ifndef QT_NO_SSL
    /** Set SSL options for this context */
    void set_ssl_options(ssl_opts options);
#endif

    /** Sets the duration of the transaction timeout.
     * @param duration_ms the Duration of the transaction timeout, in milliseconds
     * @return 0: OK, else: error, timeout not set
     */
    int set_transaction_timeout(int duration_ms);

    /** Sets the duration of the transaction timeout.
     * @param t Duration of the transaction timeout
     * @return 0: OK, else: error, timeout not set
     */
    int set_transaction_timeout(QTime t) {
        return set_transaction_timeout(t.msecsSinceStartOfDay());
    }

    /** Returns the current transaction duration, in milliseconds. */
    int transaction_timeout_get();

    /** Give sthe current transaction duration in @p t parameter.
     */
    void transaction_timeout_get(QTime& t) {
        t.fromMSecsSinceStartOfDay(transaction_timeout_get());
    }

#if PUBNUB_CRYPTO_API
    /// Set the crypto module to be set by the context
    ///
    /// This function sets the crypto module to be used by the context
    /// for encryption and decryption of messages.
    /// @see pubnub_set_crypto_module()
    void set_crypto_module(into_crypto_provider_ptr &crypto)
    {
        pbcc_set_crypto_module(&*d_context, crypto.into_provider());
    }

    /// Get the crypto module used by the context
    ///
    /// This function gets the crypto module used by the context
    /// for encryption and decryption of messages.
    /// @see pubnub_get_crypto_module()
    pubnub_crypto_provider_t *get_crypto_module()
    {
        return pbcc_get_crypto_module(&*d_context);
    }
#endif /* PUBNUB_CRYPTO_API */


private slots:
    void httpFinished();
    void transactionTimeout();

#ifndef QT_NO_SSL
    void sslErrors(QNetworkReply* reply, QList<QSslError> const& errors);
#endif
#if PUBNUB_USE_AUTO_HEARTBEAT
    void auto_heartbeatTimeout();
#endif

signals:
    /** This signal is sent on the outcome of the transaction/operation.
        @param result The final result (outcome)
     */
    void outcome(pubnub_res result);

private:

    /// Common function that starts any of the requests
    pubnub_res startRequest(pubnub_res result, pubnub_trans transaction);

    /// Common function which processes the data received in response
    pubnub_res finish(QByteArray const &data, int http_code);

#if PUBNUB_USE_AUTO_HEARTBEAT
    /// Checks whether to use saved channels and channel groups
    bool check_if_default_channel_and_groups(QString const& channel,
                                             QString const& channel_group,
                                             QString& prep_channels,
                                             QString& prep_channel_groups);
    /// Prepares channels and channel groups to be used in transaction request
    void auto_heartbeat_prepare_channels_and_ch_groups(QString const& channel,
                                                       QString const& channel_group,
                                                       QString& prep_channels,
                                                       QString& prep_channel_groups);
    /// Removes @p channel list of channels and @p channel_group channel groups
    /// from saved(subscribed) channels and channel groups
    void update_channels_and_ch_groups(QString const& channel,
                                       QString const& channel_group);
    /// Starts auto heartbeat timer after corresponding transactions
    void start_auto_heartbeat_timer(pubnub_res pbres);
    /// Stops auto heartbeat inconditionally
    void stop_auto_heartbeat();
    /// Stops auto heartbeat before corresponding transactions
    void stop_auto_heartbeat_before_transaction(pubnub_trans transaction);
#endif

    /// The publish key
    QByteArray d_pubkey;

    /// The subscribe key
    QByteArray d_keysub;

    /// Auth key to use (can be empty - none)
    QByteArray d_auth;

    /// Qt's Network Access Manager
    QNetworkAccessManager d_qnam;

    /// Qt's Reply (from a HTTP request)
    QScopedPointer<QNetworkReply> d_reply;

    /// C-core context
    QScopedPointer<pbcc_context> d_context;

    /// Last Pubnub transaction
    pubnub_trans d_trans;

    /// Last HTTP code
    unsigned d_http_code;

    /// Origin (protocol and host of the URL) to use
    QString d_origin;

#ifndef QT_NO_SSL
    /// SSL options
    ssl_opts d_ssl_opts;
#endif    
    /// Transaction timeout duration, in milliseconds
    int d_transaction_timeout_duration_ms;

    /// Transaction timed out indicator
    bool d_transaction_timed_out;

    /// Transaction timer
    QTimer *d_transactionTimer;
#if PUBNUB_USE_AUTO_HEARTBEAT
    /// Auto heartbeat is enabled and pulsing whenever subscription is not in progress
    bool d_auto_heartbeat_enabled;
    /// Auto heartbeat period in seconds
    size_t d_auto_heartbeat_period_sec;
    /// Auto heartbeat timer
    QTimer *d_auto_heartbeatTimer;
    /// Subscribed channels
    QStringList d_channels;
    /// Subscribed channel groups
    QStringList d_channel_groups;
#endif /* PUBNUB_USE_AUTO_HEARTBEAT */

    /// To Keep-Alive or not to Keep-Alive
    bool d_use_http_keep_alive;

    ///  transaction method(GET, POST, PATCH, or DELETE)
    enum pubnub_method d_method;

    /// Message to send via POST method
    QByteArray d_message_to_send;

#if QT_VERSION >= QT_6
    mutable QRecursiveMutex d_mutex;
#else
    mutable QMutex d_mutex;
#endif
};


Q_DECLARE_OPERATORS_FOR_FLAGS(pubnub_qt::ssl_opts)


#endif // !defined INC_PUBNUB_QT
