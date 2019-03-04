/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

extern "C" {
#include "core/pubnub_ccore_pubsub.h"
#include "core/pubnub_ccore.h"
#include "core/pubnub_assert.h"
#include "lib/pbcrc32.c"
#include "core/pubnub_memory_block.h"
#include "core/pubnub_advanced_history.h"
#define MAX_ERROR_MESSAGE_LENGTH 100
}

#include "pubnub_qt.h"

#include <QtNetwork>

/* Minimal acceptable message length difference, between unpacked and packed message, in percents */
#define PUBNUB_MINIMAL_ACCEPTABLE_COMPRESSION_RATIO 10
#define GZIP_HEADER_AND_FOOTER_LENGTH 18
#if PUBNUB_THREADSAFE
#define KEEP_THREAD_SAFE() QMutexLocker lk(&d_mutex)
#else
#define KEEP_THREAD_SAFE()
#endif

pubnub_qt::pubnub_qt(QString pubkey, QString keysub)
    : d_pubkey(pubkey.toLatin1())
    , d_keysub(keysub.toLatin1())
    , d_context(new pbcc_context)
    , d_http_code(0)
    ,
#ifdef QT_NO_SSL
    d_origin("http://pubsub.pubnub.com")
    , d_ssl_opts(0)
    ,
#else
    d_origin("https://pubsub.pubnub.com")
    , d_ssl_opts(useSSL)
    ,
#endif
    d_transaction_timeout_duration_ms(10000)
    , d_transaction_timed_out(false)
    , d_transactionTimer(new QTimer(this))
    , d_use_http_keep_alive(true)
    , d_mutex(QMutex::Recursive)
{
    pbcc_init(d_context.data(), d_pubkey.data(), d_keysub.data());
    connect(&d_qnam,
            SIGNAL(sslErrors(QNetworkReply*, QList<QSslError>)),
            this,
            SLOT(sslErrors(QNetworkReply*, QList<QSslError>)));
    connect(d_transactionTimer, SIGNAL(timeout()), this, SLOT(transactionTimeout()));
}


pubnub_qt::~pubnub_qt()
{
    KEEP_THREAD_SAFE();
    pbcc_deinit(d_context.data());
}


pubnub_res pubnub_qt::startRequest(pubnub_res result, pubnub_trans transaction)
{
    if (PNR_STARTED == result) {
        QUrl url(d_origin
                 + QString::fromLatin1(d_context->http_buf, d_context->http_buf_len));
        d_trans = transaction;

        QNetworkReply* p = d_reply.take();
        if (p) {
            p->deleteLater();
        }
        d_transaction_timed_out = false;
        QNetworkRequest req(url);
        if (!d_use_http_keep_alive) {
            req.setRawHeader("Connection", "Close");
        }
        if (PBTT_PUBLISH == transaction) {
            switch (d_publish_method) {
            case pubnubPublishViaPOSTwithGZIP:
                req.setRawHeader("Content-Encoding", "gzip");
                /* FALLTHRU */
            case pubnubPublishViaPOST:
                req.setRawHeader("Content-Type", "application/json");
                req.setRawHeader("Content-Length",
                                 QByteArray::number(d_message_to_publish.size()));
                d_reply.reset(d_qnam.post(req, d_message_to_publish));
                break;
            case pubnubPublishViaGET:
                d_reply.reset(d_qnam.get(req));
                break;
            default:
                break;
            }
        }
        else {
            d_reply.reset(d_qnam.get(req));
        }
        connect(d_reply.data(), SIGNAL(finished()), this, SLOT(httpFinished()));
        d_transactionTimer->start(d_transaction_timeout_duration_ms);
    }
    return result;
}

void pubnub_qt::set_uuid(QString const& uuid)
{
    KEEP_THREAD_SAFE();
    pbcc_set_uuid(d_context.data(), uuid.isEmpty() ? NULL : uuid.toLatin1().data());
}

QString pubnub_qt::uuid() const
{
    QMutexLocker lk(&d_mutex);
    char const*  uuid = pbcc_uuid_get(d_context.data());
    return QString((NULL == uuid) ? "" : uuid);
}

void pubnub_qt::set_auth(QString const& auth)
{
    QMutexLocker lk(&d_mutex);
    d_auth = auth.toLatin1();
    pbcc_set_auth(d_context.data(), d_auth.data());
}

QString pubnub_qt::get() const
{
    KEEP_THREAD_SAFE();
    return pbcc_get_msg(d_context.data());
}


QStringList pubnub_qt::get_all() const
{
    QStringList all;
    KEEP_THREAD_SAFE();
    while (char const* msg = pbcc_get_msg(d_context.data())) {
        if (nullptr == msg) {
            break;
        }
        all.push_back(msg);
    }
    return all;
}


QString pubnub_qt::get_channel() const
{
    KEEP_THREAD_SAFE();
    return pbcc_get_channel(d_context.data());
}


QStringList pubnub_qt::get_all_channels() const
{
    QStringList all;
    KEEP_THREAD_SAFE();
    while (char const* msg = pbcc_get_channel(d_context.data())) {
        if (nullptr == msg) {
            break;
        }
        all.push_back(msg);
    }
    return all;
}


void pubnub_qt::cancel()
{
    KEEP_THREAD_SAFE();
    if (d_reply) {
        d_reply->abort();
    }
}


pubnub_res pubnub_qt::publish(QString const& channel, QString const& message)
{
    QMutexLocker lk(&d_mutex);
    d_publish_method = pubnubPublishViaGET;
    return startRequest(pbcc_publish_prep(d_context.data(),
                                          channel.toLatin1().data(),
                                          message.toLatin1().data(),
                                          true,
                                          false,
                                          NULL,
                                          d_publish_method),
                        PBTT_PUBLISH);
}


pubnub_res pubnub_qt::publish_via_post(QString const&    channel,
                                       QByteArray const& message)
{
    QMutexLocker lk(&d_mutex);
    d_publish_method     = pubnubPublishViaPOST;
    d_message_to_publish = message;
    return startRequest(pbcc_publish_prep(d_context.data(),
                                          channel.toLatin1().data(),
                                          d_message_to_publish.data(),
                                          true,
                                          false,
                                          NULL,
                                          d_publish_method),
                        PBTT_PUBLISH);
}

static QByteArray pack_message_to_gzip(QByteArray const& message)
{
    auto        compressedData = qCompress(message);
    QByteArray  header;
    QDataStream ds1(&header, QIODevice::WriteOnly);
    QByteArray  footer;
    QDataStream ds2(&footer, QIODevice::WriteOnly);
    /*  Striping the first six bytes (a 4-byte length put on by qCompress and a
       2-byte zlib header) and the last four bytes (a zlib integrity check).
     */
    compressedData.remove(0, 6);
    compressedData.chop(4);
    if ((long)((message.size() - compressedData.size() - GZIP_HEADER_AND_FOOTER_LENGTH)
               * 100)
            / message.size()
        < PUBNUB_MINIMAL_ACCEPTABLE_COMPRESSION_RATIO) {
        /* With insufficient compression we choose not to pack */
        qDebug() << "pack_message_to_gzip(" << message
                 << "):message wasn't compressed due to low compression ratio.";
        return message;
    }
    /* Prepending a generic 10-byte gzip header (RFC 1952) */
    ds1.setByteOrder(QDataStream::BigEndian);
    ds1 << quint16(0x1f8b) << quint16(0x0800) << quint16(0x0000)
        << quint16(0x0000) << quint16(0x000b);
    /* Appending a four-byte CRC-32 of the uncompressed data
       Appending 4 bytes uncompressed input size modulo 2^32
     */
    ds2.setByteOrder(QDataStream::LittleEndian);
    ds2 << quint32(pbcrc32(message.data(), message.size()))
        << quint32(message.size());
    return header + compressedData + footer;
}

pubnub_res pubnub_qt::publish_via_post_with_gzip(QString const&    channel,
                                                 QByteArray const& message)
{
    QMutexLocker lk(&d_mutex);
    d_message_to_publish = pack_message_to_gzip(message);
    d_publish_method     = (d_message_to_publish.size() != message.size())
                           ? pubnubPublishViaPOSTwithGZIP
                           : pubnubPublishViaPOST;
    return startRequest(pbcc_publish_prep(d_context.data(),
                                          channel.toLatin1().data(),
                                          d_message_to_publish.data(),
                                          true,
                                          false,
                                          NULL,
                                          d_publish_method),
                        PBTT_PUBLISH);
}

pubnub_res pubnub_qt::subscribe(QString const& channel, QString const& channel_group)
{
    KEEP_THREAD_SAFE();
    return startRequest(
        pbcc_subscribe_prep(
            d_context.data(),
            channel.isEmpty() ? 0 : channel.toLatin1().data(),
            channel_group.isEmpty() ? 0 : channel_group.toLatin1().data(),
            0),
        PBTT_SUBSCRIBE);
}


pubnub_res pubnub_qt::leave(QString const& channel, QString const& channel_group)
{
    KEEP_THREAD_SAFE();
    return startRequest(
        pbcc_leave_prep(
            d_context.data(),
            channel.isEmpty() ? 0 : channel.toLatin1().data(),
            channel_group.isEmpty() ? 0 : channel_group.toLatin1().data()),
        PBTT_LEAVE);
}


pubnub_res pubnub_qt::time()
{
    KEEP_THREAD_SAFE();
    return startRequest(pbcc_time_prep(d_context.data()), PBTT_TIME);
}


pubnub_res pubnub_qt::history(QString const& channel, unsigned count, bool include_token)
{
    KEEP_THREAD_SAFE();
    return startRequest(
        pbcc_history_prep(d_context.data(),
                          channel.isEmpty() ? 0 : channel.toLatin1().data(),
                          count,
                          include_token,
                          pbccNotSet,
                          pbccNotSet,
                          0,
                          0),
        PBTT_HISTORY);
}


pubnub_res pubnub_qt::history(QString const& channel,
                              unsigned       count,
                              bool           include_token,
                              QString const& start,
                              bool           reverse,
                              QString const& end,
                              bool           string_token)
{
    KEEP_THREAD_SAFE();
    return startRequest(
        pbcc_history_prep(d_context.data(),
                          channel.isEmpty() ? 0 : channel.toLatin1().data(),
                          count,
                          include_token,
                          string_token ? pbccTrue : pbccFalse,
                          reverse ? pbccTrue : pbccFalse,
                          start.isEmpty() ? 0 : start.toLatin1().data(),
                          end.isEmpty() ? 0 : end.toLatin1().data()),
        PBTT_HISTORY);
}

#if PUBNUB_USE_ADVANCED_HISTORY
QString pubnub_qt::get_error_message()
{
    pubnub_chamebl_t msg;
    if (pbcc_get_error_message(d_context.data(), &msg) != 0) {
        return QString("");
    }
    return QString(QByteArray(msg.ptr, msg.size));    
}


pubnub_res pubnub_qt::message_counts(QString const& channel, QString const& timetoken)
{
    KEEP_THREAD_SAFE();
    return startRequest(
        pbcc_message_counts_prep(d_context.data(),
                                 channel.isEmpty() ? 0 : channel.toLatin1().data(),
                                 timetoken.isEmpty() ? 0 : timetoken.toLatin1().data(),
                                 0),
        PBTT_MESSAGE_COUNTS);
}


pubnub_res pubnub_qt::message_counts(QStringList const& channel, QString const& timetoken)
{
    return message_counts(channel.join(","), timetoken);
}


pubnub_res pubnub_qt::message_counts(QString const& channel,
                                     QStringList const& channel_timetoken)
{
    QString tt_list = channel_timetoken.join(",");
    KEEP_THREAD_SAFE();
    return startRequest(
        pbcc_message_counts_prep(d_context.data(),
                                 channel.isEmpty() ? 0 : channel.toLatin1().data(),
                                 0,
                                 tt_list.isEmpty() ? 0 : tt_list.toLatin1().data()),
        PBTT_MESSAGE_COUNTS);
}


pubnub_res pubnub_qt::message_counts(QStringList const& channel,
                                     QStringList const& channel_timetoken)
{
    return message_counts(channel.join(","), channel_timetoken);
}


pubnub_res pubnub_qt::message_counts(QVector<QPair<QString, QString>> const& channel_timetokens)
{
    QString  ch_list("");
    QString  tt_list("");
    unsigned n = channel_timetokens.isEmpty() ? 0 : channel_timetokens.size(); 
    unsigned i;
    KEEP_THREAD_SAFE();
    for (i = 0; i < n; i++) {
        QString separator(((i+1) < n) ? "," : "");
        ch_list += channel_timetokens[i].first + separator;
        tt_list += channel_timetokens[i].second + separator;
    }
    return startRequest(
        pbcc_message_counts_prep(d_context.data(),
                                 ch_list.isEmpty() ? 0 : ch_list.toLatin1().data(),
                                 0,
                                 tt_list.isEmpty() ? 0 : tt_list.toLatin1().data()),
        PBTT_MESSAGE_COUNTS);
}


QMap<QString, size_t> pubnub_qt::get_channel_message_counts()
{
    QMap<QString, size_t> map;
    QVector<pubnub_chan_msg_count> chan_msg_counters;
    int count = pbcc_get_chan_msg_counts_size(d_context.data());
    if (count <= 0) {
        return map;
    }
    chan_msg_counters = QVector<pubnub_chan_msg_count>(count);
    if (pbcc_get_chan_msg_counts(d_context.data(), (size_t*)&count, &chan_msg_counters[0]) != 0) {
        return map;
    }
    for (int i = 0; i < count; i++) {
        map.insert(QString(QByteArray(chan_msg_counters[i].channel.ptr,
                                      chan_msg_counters[i].channel.size)),
                   chan_msg_counters[i].message_count);
    }
    return map;
}
#endif /* PUBNUB_USE_ADVANCED_HISTORY */

pubnub_res pubnub_qt::here_now(QString const& channel, QString const& channel_group)
{
    KEEP_THREAD_SAFE();
    return startRequest(
        pbcc_here_now_prep(
            d_context.data(),
            channel.isEmpty() ? 0 : channel.toLatin1().data(),
            channel_group.isEmpty() ? 0 : channel_group.toLatin1().data(),
            pbccNotSet,
            pbccNotSet),
        PBTT_HERENOW);
}


pubnub_res pubnub_qt::global_here_now()
{
    KEEP_THREAD_SAFE();
    return startRequest(
        pbcc_here_now_prep(d_context.data(), 0, 0, pbccNotSet, pbccNotSet),
        PBTT_GLOBAL_HERENOW);
}


pubnub_res pubnub_qt::where_now(QString const& uuid)
{
    KEEP_THREAD_SAFE();
    return startRequest(pbcc_where_now_prep(d_context.data(),
                                            uuid.isEmpty()
                                                ? pbcc_uuid_get(d_context.data())
                                                : uuid.toLatin1().data()),
                        PBTT_WHERENOW);
}


pubnub_res pubnub_qt::set_state(QString const& channel,
                                QString const& channel_group,
                                QString const& uuid,
                                QString const& state)
{
    KEEP_THREAD_SAFE();
    return startRequest(
        pbcc_set_state_prep(
            d_context.data(),
            channel.isEmpty() ? 0 : channel.toLatin1().data(),
            channel_group.isEmpty() ? 0 : channel_group.toLatin1().data(),
            uuid.isEmpty() ? pbcc_uuid_get(d_context.data()) : uuid.toLatin1().data(),
            state.toLatin1().data()),
        PBTT_SET_STATE);
}


pubnub_res pubnub_qt::state_get(QString const& channel,
                                QString const& channel_group,
                                QString const& uuid)
{
    KEEP_THREAD_SAFE();
    return startRequest(
        pbcc_state_get_prep(
            d_context.data(),
            channel.isEmpty() ? 0 : channel.toLatin1().data(),
            channel_group.isEmpty() ? 0 : channel_group.toLatin1().data(),
            uuid.isEmpty() ? pbcc_uuid_get(d_context.data())
                           : uuid.toLatin1().data()),
        PBTT_STATE_GET);
}


pubnub_res pubnub_qt::remove_channel_group(QString const& channel_group)
{
    KEEP_THREAD_SAFE();
    return startRequest(
        pbcc_remove_channel_group_prep(
            d_context.data(),
            channel_group.isEmpty() ? 0 : channel_group.toLatin1().data()),
        PBTT_REMOVE_CHANNEL_GROUP);
}


pubnub_res pubnub_qt::remove_channel_from_group(QString const& channel,
                                                QString const& channel_group)
{
    KEEP_THREAD_SAFE();
    return startRequest(
        pbcc_channel_registry_prep(
            d_context.data(),
            channel_group.isEmpty() ? 0 : channel_group.toLatin1().data(),
            "remove",
            channel.isEmpty() ? 0 : channel.toLatin1().data()),
        PBTT_REMOVE_CHANNEL_FROM_GROUP);
}


pubnub_res pubnub_qt::add_channel_to_group(QString const& channel,
                                           QString const& channel_group)
{
    KEEP_THREAD_SAFE();
    return startRequest(
        pbcc_channel_registry_prep(
            d_context.data(),
            channel_group.isEmpty() ? 0 : channel_group.toLatin1().data(),
            "add",
            channel.isEmpty() ? 0 : channel.toLatin1().data()),
        PBTT_ADD_CHANNEL_TO_GROUP);
}


pubnub_res pubnub_qt::list_channel_group(QString const& channel_group)
{
    KEEP_THREAD_SAFE();
    return startRequest(
        pbcc_channel_registry_prep(
            d_context.data(),
            channel_group.isEmpty() ? 0 : channel_group.toLatin1().data(),
            0,
            0),
        PBTT_LIST_CHANNEL_GROUP);
}


int pubnub_qt::last_http_code() const
{
    return d_http_code;
}


QString pubnub_qt::last_publish_result() const
{
    KEEP_THREAD_SAFE();
    if (PUBNUB_DYNAMIC_REPLY_BUFFER && (nullptr == d_context->http_reply)) {
        return "";
    }
    if ((d_trans != PBTT_PUBLISH) || (d_context->http_reply[0] == '\0')) {
        return "";
    }

    char* end;
    strtol(d_context->http_reply + 1, &end, 10);
    return end + 1;
}


pubnub_publish_res pubnub_qt::parse_last_publish_result()
{
    QString toParse = last_publish_result();
    return pubnub_parse_publish_result(toParse.toLatin1());
}


QString pubnub_qt::last_time_token() const
{
    KEEP_THREAD_SAFE();
    return d_context->timetoken;
}


void pubnub_qt::set_ssl_options(ssl_opts options)
{
    QMutexLocker lk(&d_mutex);
    if (options & useSSL) {
        if (d_origin.startsWith("http:")) {
            d_origin.replace(0, 5, "https:");
        }
    }
    else {
        if (d_origin.startsWith("https:")) {
            d_origin.replace(0, 6, "http:");
        }
    }
    d_ssl_opts = options;
}


void pubnub_qt::set_origin(QString const& origin)
{
    QMutexLocker lk(&d_mutex);
    d_origin = origin;
    if (!origin.startsWith("http:") && !origin.startsWith("https:")) {
        d_origin.prepend("http://");
        set_ssl_options(d_ssl_opts);
    }
}


pubnub_res pubnub_qt::finish(QByteArray const& data, int http_code)
{
    pubnub_res pbres = PNR_OK;
    KEEP_THREAD_SAFE();
    if (PUBNUB_DYNAMIC_REPLY_BUFFER) {
        pbcc_realloc_reply_buffer(d_context.data(), data.size());
        memcpy(d_context->http_reply, data.data(), data.size());
        d_context->http_buf_len            = data.size();
        d_context->http_reply[data.size()] = '\0';
    }
    else {
        if ((unsigned)data.size() >= sizeof d_context->http_reply) {
            return PNR_REPLY_TOO_BIG;
        }
        d_context->http_buf_len = data.size();
        memcpy(d_context->http_reply, data.data(), data.size());
        d_context->http_reply[d_context->http_buf_len] = '\0';
    }

    qDebug() << "finish('" << d_context->http_reply << "')";

    switch (d_trans) {
    case PBTT_SUBSCRIBE:
        if (pbcc_parse_subscribe_response(d_context.data()) != 0) {
            pbres = PNR_FORMAT_ERROR;
        }
        break;
    case PBTT_PUBLISH:
        pbres = pbcc_parse_publish_response(d_context.data());
        break;
    case PBTT_TIME:
        if (pbcc_parse_time_response(d_context.data()) != 0) {
            pbres = PNR_FORMAT_ERROR;
        }
        break;
    case PBTT_HISTORY:
        if (pbcc_parse_history_response(d_context.data()) != 0) {
            pbres = PNR_FORMAT_ERROR;
        }
        break;
    case PBTT_LEAVE:
    case PBTT_HERENOW:
    case PBTT_GLOBAL_HERENOW:
    case PBTT_WHERENOW:
    case PBTT_SET_STATE:
    case PBTT_STATE_GET:
        if (pbcc_parse_presence_response(d_context.data()) != 0) {
            pbres = PNR_FORMAT_ERROR;
        }
        break;
    case PBTT_REMOVE_CHANNEL_GROUP:
    case PBTT_REMOVE_CHANNEL_FROM_GROUP:
    case PBTT_ADD_CHANNEL_TO_GROUP:
    case PBTT_LIST_CHANNEL_GROUP:
        pbres = pbcc_parse_channel_registry_response(d_context.data());
        break;
#if PUBNUB_USE_ADVANCED_HISTORY
    case PBTT_MESSAGE_COUNTS:
        pbres = pbcc_parse_message_counts_response(d_context.data());
        break;
#endif
    default:
        break;
    }

    QVariant statusCode =
        d_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    d_http_code = statusCode.isValid() ? statusCode.toInt() : 0;

    if ((PNR_OK == pbres) && (http_code != 0) && (http_code / 100 != 2)) {
        return PNR_HTTP_ERROR;
    }

    return pbres;
}


void pubnub_qt::transactionTimeout()
{
    QMutexLocker lk(&d_mutex);
    if (d_reply) {
        d_transaction_timed_out = true;
        d_reply->abort();
    }
}


void pubnub_qt::httpFinished()
{
    KEEP_THREAD_SAFE();
    d_transactionTimer->stop();

    QNetworkReply::NetworkError error = d_reply->error();
    if (error) {
        qDebug() << "error: " << d_reply->error()
                 << ", string: " << d_reply->errorString();
        d_context->http_buf_len = 0;
        if (PUBNUB_DYNAMIC_REPLY_BUFFER) {
            d_context->http_reply = NULL;
        }
        else {
            d_context->http_reply[0] = '\0';
        }
        switch (error) {
        case QNetworkReply::OperationCanceledError:
            if (d_transaction_timed_out) {
                emit outcome(PNR_TIMEOUT);
            }
            else {
                emit outcome(PNR_CANCELLED);
            }
            return;
        case QNetworkReply::TimeoutError:
            emit outcome(PNR_CONNECTION_TIMEOUT);
            return;
        case QNetworkReply::HostNotFoundError:
            emit outcome(PNR_ADDR_RESOLUTION_FAILED);
            return;
        case QNetworkReply::ConnectionRefusedError:
        case QNetworkReply::ProtocolUnknownError:
            emit outcome(PNR_CONNECT_FAILED);
            return;
        }
    }

    emit outcome(finish(d_reply->readAll(), d_reply->error()));
}


void pubnub_qt::sslErrors(QNetworkReply* reply, const QList<QSslError>& errors)
{
    QString errorString;
    foreach (const QSslError& error, errors) {
        if (!errorString.isEmpty())
            errorString += ", ";
        errorString += error.errorString();
    }

    qDebug() << "SSL error: " << errorString;

    QMutexLocker lk(&d_mutex);
    if (d_ssl_opts & ignoreSecureConnectionRequirement) {
        reply->ignoreSslErrors();
    }
}


#include "core/pubnub_version_internal.h"

#define PUBNUB_SDK_NAME "Qt5"

extern "C" char const* pubnub_sdk_name()
{
    return PUBNUB_SDK_NAME;
}

extern "C" char const* pubnub_uname()
{
    return PUBNUB_SDK_NAME "%2F" PUBNUB_SDK_VERSION;
}

extern "C" char const* pubnub_version()
{
    return PUBNUB_SDK_VERSION;
}
