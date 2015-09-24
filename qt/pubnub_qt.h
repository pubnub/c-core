/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_QT
#define      INC_PUBNUB_QT

#include <QUrl>
#include <QUuid>
#include <QNetworkAccessManager>

extern "C" {
#include "pubnub_api_types.h"
}

QT_BEGIN_NAMESPACE
class QNetworkReply;
class QSslError;
QT_END_NAMESPACE


struct pbcc_context;


class pubnub_qt : public QObject
{
    Q_OBJECT

public:
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
    Q_DECLARE_FLAGS(ssl_opts, ssl_opt);

    void set_auth(QString const &auth);
    QString auth() const { return d_auth; }

    void set_uuid(QString const &uuid);
    void set_uuid_v4_random() {
        QString uuid = QUuid::createUuid().toString();
        set_uuid(uuid.mid(1, uuid.size()-2));
    }
    QString uuid() const { return d_uuid; }

    void set_origin(QString const& origin) {
        d_origin = origin;
    }
    QString const& origin() const {
        return d_origin;
    }

    QString get() const;
    QStringList get_all() const;
    QString get_channel() const;
    QStringList get_all_channels() const;
    
    void cancel();

    pubnub_res publishv2(QString const &channel, QString const &message, pubv2_opts options);
    pubnub_res publishv2(QStringList const &channel, QString const &message, pubv2_opts options) {
        return publishv2(channel.join(","), message, options);
    }

    pubnub_res publish(QString const &channel, QString const &message);
    pubnub_res publish(QStringList const &channel, QString const &message) {
        return publish(channel.join(","), message);
    }

    pubnub_res subscribe(QString const &channel, QString const &channel_group="");
    pubnub_res subscribe(QStringList const &channel, QStringList const &channel_group=QStringList()) {
        return subscribe(channel.join(","), channel_group.join(","));
    }

    pubnub_res leave(QString const &channel, QString const &channel_group="");
    pubnub_res leave(QStringList const &channel, QStringList const &channel_group=QStringList()) {
        return leave(channel.join(","), channel_group.join(","));
    }

    pubnub_res time();

    pubnub_res history(QString const &channel, QString const &channel_group = "", unsigned count = 100);
    pubnub_res history(QStringList const &channel, QStringList const &channel_group=QStringList(), unsigned count = 100) {
        return history(channel.join(","), channel_group.join(","), count);
    }

    pubnub_res historyv2(QString const &channel, QString const &channel_group = "", unsigned count = 100, bool include_token = false);
    pubnub_res historyv2(QStringList const &channel, QStringList const &channel_group = QStringList(), unsigned count = 100, bool include_token = false) {
        return historyv2(channel.join(","), channel_group.join(","), count, include_token);
    }

    pubnub_res here_now(QString const &channel, QString const &channel_group="");
    pubnub_res here_now(QStringList const &channel, QStringList const &channel_group=QStringList()) {
        return here_now(channel.join(","), channel_group.join(","));
    }

    pubnub_res global_here_now();

    pubnub_res where_now(QString const &uuid="");

    pubnub_res set_state(QString const &channel, QString const& channel_group, QString const &uuid, QString const &state);
    pubnub_res set_state(QStringList const &channel, QStringList const& channel_group, QString const &uuid, QString const &state) {
        return set_state(channel.join(","), channel_group.join(","), uuid, state);
    }

    pubnub_res state_get(QString const &channel, QString const& channel_group="", QString const &uuid="");
    pubnub_res state_get(QStringList const &channel, QStringList const& channel_group=QStringList(), QString const &uuid="") {
        return state_get(channel.join(","), channel_group.join(","), uuid);
    }

    pubnub_res remove_channel_group(QString const& channel_group);
    pubnub_res remove_channel_group(QStringList const& channel_group) {
        return remove_channel_group(channel_group.join(","));
    }

    pubnub_res remove_channel_from_group(QString const& channel, QString const& channel_group);
    pubnub_res remove_channel_from_group(QStringList const& channel, QStringList const& channel_group) {
        return remove_channel_from_group(channel.join(","), channel_group.join(","));
    }

    pubnub_res add_channel_to_group(QString const& channel, QString const& channel_group);
    pubnub_res add_channel_to_group(QStringList const& channel, QStringList const& channel_group) {
        return add_channel_to_group(channel.join(","), channel_group.join(","));
    }

    pubnub_res list_channel_group(QString const& channel_group);
    pubnub_res list_channel_group(QStringList const& channel_group) {
        return list_channel_group(channel_group.join(","));
    }
    

    int last_http_code() const;
    QString last_publish_result() const;
    QString last_time_token() const;
    
#ifndef QT_NO_SSL
    void set_ssl_options(ssl_opts options);
#endif

private slots:
    void httpFinished();

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

    /// Origin (protocol and host of the URL) to use
    QString d_origin;

    /// Qt's Network Access Manager
    QNetworkAccessManager d_qnam;

    /// Qt's Reply (from a HTTP request)
    QScopedPointer<QNetworkReply> d_reply;
    
    /// C-core context
    QScopedPointer<pbcc_context> d_context;

    /// Last Pubnub transaction
    pubnub_trans d_trans;

#ifndef QT_NO_SSL
    /// SSL options
    ssl_opts d_ssl_opts;
#endif
};


Q_DECLARE_OPERATORS_FOR_FLAGS(pubnub_qt::pubv2_opts)

Q_DECLARE_OPERATORS_FOR_FLAGS(pubnub_qt::ssl_opts)


#endif // !defined INC_PUBNUB_QT
