/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_QT
#define      INC_PUBNUB_QT

#include <QUrl>
#include <QUuid>
#include <QNetworkAccessManager>
#include <QTimer>

extern "C" {
#include "pubnub_api_types.h"
#include "pubnub_helper.h"
}

QT_BEGIN_NAMESPACE
class QNetworkReply;
class QSslError;
QT_END_NAMESPACE


struct pbcc_context;

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

    /** Options for Publish v2. These are designed to be used as
     * "bit-masks", for which purpose there are overloaded `&` and `|`
     * (bit-and and bit-or) operators.
     */
    enum pubv2_opt {
        /// If not set, message will not be stored in history of the
        /// channel
        store_in_history = 0x01,
        /// If set, message will not be stored for delayed or repeated
        /// retrieval or display
        eat_after_reading = 0x02
    };

    Q_DECLARE_FLAGS(pubv2_opts, pubv2_opt)

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

    Q_DECLARE_FLAGS(ssl_opts, ssl_opt)

    /** Set the authentication key to be used in this context.
     * After construction it is empty (null), thus not used.
     * Pass an empty (or null) string to stop using it.
     */
    void set_auth(QString const &auth);

    /** Returns the current auth key */
    QString auth() const { return d_auth; }

    /** Set the UUID to be used in this context.
     * After construction it is empty (null), thus not used.
     * Pass an empty (or null) string to stop using it.
     */
    void set_uuid(QString const &uuid);

    /** Set the UUID to a random value, according to stadard
     * v4 representation.
     */
    void set_uuid_v4_random() {
        QString uuid = QUuid::createUuid().toString();
        set_uuid(uuid.mid(1, uuid.size()-2));
    }

    /** Returns the current UUID value (string) */
    QString uuid() const { return d_uuid; }

    /** Sets the Pubnub origin to use, that is, the
     * protocol (http or https) and host part of the URL.
     * By default, host is the generic host and protocol
     * is https, unless SSL support is not included in
     * your Qt.
     */
    void set_origin(QString const& origin);

    /** Returns the current origin */
    QString const& origin() const {
        return d_origin;
    }

    /** Returns the string of an arrived message or other element of the
        response to an operation/transaction. Message(s) arrive on finish
        of a subscribe operation, while for  other operations this will give
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


    /** Returns a string of a fetched subscribe operation/transaction's
        next channel.  Each transaction may hold a list of channels, and
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

        You can't publish if a transaction is in progress in @p p context.

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

        @param channel The string with the channel (or comma-delimited list
        of channels) to publish to.
        @param message The message to publish, expected to be in JSON format

        @return #PNR_STARTED on success, an error otherwise
     */
    pubnub_res publish(QString const &channel, QString const &message);

    /** Publish the @p message (in JSON format) on @p p channel, using the
        @p p context, utilizing the v2 API. This actually means "initiate
        a publish transaction".

        Basically, this is an extension to the pubnub_publish() (v1),
        with some additional options.

        You can't publish if a transaction is in progress in @p p context.

        @param channel The string with the channel (or comma-delimited list
        of channels) to publish to.
        @param message The message to publish, expected to be in JSON format
        @param store_in_history If `false`, message will not be stored in
        history of the channel
        @param eat_after_reading If `true`, message will not be stored for
        delayed or repeated retrieval or display

        @return #PNR_STARTED on success, an error otherwise
        */
    pubnub_res publishv2(QString const &channel, QString const &message, pubv2_opts options);

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

    /** Leave the @p channel. This actually means "initiate a leave
        transaction".  You should leave channel(s) when you want to
        subscribe to another in the same context to avoid loosing
        messages. Also, it is useful for tracking presence.

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
    
#ifndef QT_NO_SSL
    /** Set SSL options for this context */
    void set_ssl_options(ssl_opts options);
#endif

    /** Sets the duration of the transaction timeout.
     * @param duration_ms the Duration of the transaction timeout, in milliseconds
     * @return 0: OK, else: error, timeout not set
     */
    int set_transaction_timeout(int duration_ms);

    /** Returns the current transaction duration, in milliseconds. */
    int transaction_timeout_get();

private slots:
    void httpFinished();
    void transactionTimeout();

#ifndef QT_NO_SSL
    void sslErrors(QNetworkReply* reply, QList<QSslError> const& errors);
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

    /// The publish key
    QByteArray d_pubkey;

    /// The subscribe key
    QByteArray d_keysub;

    /// UUID to use (can be empty - none)
    QByteArray d_uuid;

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
};


Q_DECLARE_OPERATORS_FOR_FLAGS(pubnub_qt::pubv2_opts)

Q_DECLARE_OPERATORS_FOR_FLAGS(pubnub_qt::ssl_opts)


#endif // !defined INC_PUBNUB_QT
