/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_qt.h"

extern "C" {
#include "core/pubnub_version_internal.h"
#include "core/pubnub_ccore_pubsub.h"
#include "core/pubnub_ccore.h"
#include "core/pubnub_assert.h"
#include "lib/pbcrc32.h"
#include "core/pubnub_memory_block.h"
#if PUBNUB_USE_ADVANCED_HISTORY
#include "core/pubnub_advanced_history.h"
#endif
#if PUBNUB_USE_OBJECTS_API
#include "core/pbcc_objects_api.h"
#endif
}

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


static QString GetOsName()
 {
 #if defined(Q_OS_ANDROID)
     return QLatin1String("Android");
 #elif defined(Q_OS_BLACKBERRY)
     return QLatin1String("Blackberry");
 #elif defined(Q_OS_IOS)
     return QLatin1String("iOS");
 #elif defined(Q_OS_MACOS)
     return QLatin1String("macOS");
 #elif defined(Q_OS_TVOS)
     return QLatin1String("tvOS");
 #elif defined(Q_OS_WATCHOS)
     return QLatin1String("watchOS");
 #elif defined(Q_OS_WINCE)
     return QLatin1String("WindowsCE");
 #elif defined(Q_OS_WIN)
     return QLatin1String("Windows");
 #elif defined(Q_OS_CYGWIN)
     return QLatin1String("Cygwin");
 #elif defined(Q_OS_LINUX)
     return QLatin1String("Linux");
 #elif defined(Q_OS_UNIX)
     return QLatin1String("Unix");
 #else
     return QLatin1String("UnknownOS");
 #endif
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
        QString user_agent(GetOsName() +
                           "-Qt" +
                           QT_VERSION_STR +
                           "-PubNub-core/" +
                           PUBNUB_SDK_VERSION);
        req.setRawHeader("User-Agent", user_agent.toLatin1());
        if (!d_use_http_keep_alive) {
            req.setRawHeader("Connection", "Close");
        }
        switch (transaction) {
#if PUBNUB_USE_ACTIONS_API
        case PBTT_REMOVE_ACTION:
             d_reply.reset(d_qnam.deleteResource(req));
             break;
#endif /* PUBNUB_USE_ACTIONS_API */
#if PUBNUB_USE_OBJECTS_API
        case PBTT_DELETE_USER:
        case PBTT_DELETE_SPACE:
             d_reply.reset(d_qnam.deleteResource(req));
             break;
        case PBTT_CREATE_USER:
        case PBTT_UPDATE_USER:
        case PBTT_CREATE_SPACE:
        case PBTT_UPDATE_SPACE:
        case PBTT_JOIN_SPACES:
        case PBTT_UPDATE_MEMBERSHIPS:
        case PBTT_LEAVE_SPACES:
        case PBTT_ADD_MEMBERS:
        case PBTT_UPDATE_MEMBERS:
        case PBTT_REMOVE_MEMBERS:
#endif /* PUBNUB_USE_OBJECTS_API */
#if PUBNUB_USE_ACTIONS_API
        case PBTT_ADD_ACTION:
#endif /* PUBNUB_USE_ACTIONS_API */
        case PBTT_PUBLISH:
            switch (d_method) {
            case pubnubSendViaPOSTwithGZIP:
            case pubnubUsePATCHwithGZIP:
                req.setRawHeader("Content-Encoding", "gzip");
                /* FALLTHRU */
            case pubnubSendViaPOST:
            case pubnubUsePATCH:
                req.setRawHeader("Content-Type", "application/json");
                req.setRawHeader("Content-Length",
                                 QByteArray::number(d_message_to_send.size()));
                d_reply.reset(((pubnubSendViaPOST == d_method) ||
                               (pubnubSendViaPOSTwithGZIP == d_method))
                              ? d_qnam.post(req, d_message_to_send)
                              : d_qnam.sendCustomRequest(req, "PATCH", d_message_to_send));
                break;
            case pubnubSendViaGET:
                d_reply.reset(d_qnam.get(req));
                break;
            default:
                break;
            }
            break;
        default:
            d_reply.reset(d_qnam.get(req));
            break;
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


#if PUBNUB_USE_SUBSCRIBE_V2
v2_message pubnub_qt::get_v2() const
{
    KEEP_THREAD_SAFE();
    return v2_message(pbcc_get_msg_v2(d_context.data()));
}


QVector<v2_message> pubnub_qt::get_all_v2() const
{
    QVector<v2_message> all;
    v2_message msg = get_v2();

    while (!msg.is_empty()) {
        all.append(msg);
        msg = get_v2();
    }
    return all;
}
#endif /* PUBNUB_USE_SUBSCRIBE_V2 */


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
    d_method = pubnubSendViaGET;
    return startRequest(pbcc_publish_prep(d_context.data(),
                                          channel.toLatin1().data(),
                                          message.toLatin1().data(),
                                          true,
                                          false,
                                          NULL,
                                          d_method),
                        PBTT_PUBLISH);
}


pubnub_res pubnub_qt::publish_via_post(QString const&    channel,
                                       QByteArray const& message)
{
    QMutexLocker lk(&d_mutex);
    d_method = pubnubSendViaPOST;
    d_message_to_send = message;
    return startRequest(pbcc_publish_prep(d_context.data(),
                                          channel.toLatin1().data(),
                                          d_message_to_send.data(),
                                          true,
                                          false,
                                          NULL,
                                          d_method),
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
    if ((long)((message.size() - compressedData.size() - GZIP_HEADER_AND_FOOTER_LENGTH) * 100)
        / (long)message.size()
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
    d_message_to_send = pack_message_to_gzip(message);
    d_method = (d_message_to_send.size() != message.size())
               ? pubnubSendViaPOSTwithGZIP
               : pubnubSendViaPOST;
    return startRequest(pbcc_publish_prep(d_context.data(),
                                          channel.toLatin1().data(),
                                          d_message_to_send.data(),
                                          true,
                                          false,
                                          NULL,
                                          d_method),
                        PBTT_PUBLISH);
}


pubnub_res pubnub_qt::signal(QString const& channel, QByteArray const& message)
{
    KEEP_THREAD_SAFE();
    return startRequest(pbcc_signal_prep(d_context.data(),
                                         channel.toLatin1().data(),
                                         message.data()),
                        PBTT_SIGNAL);
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


pubnub_res pubnub_qt::subscribe_v2(QString const& channel, subscribe_v2_options opt)
{
    KEEP_THREAD_SAFE();
    return startRequest(
        pbcc_subscribe_v2_prep(
            d_context.data(),
            channel.isEmpty() ? 0 : channel.toLatin1().data(),
            opt.get_chgroup(),
            opt.get_heartbeat(),
            opt.get_filter_expr()),
        PBTT_SUBSCRIBE_V2);
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

#if PUBNUB_USE_OBJECTS_API
pubnub_res pubnub_qt::get_users(list_options& options)
{
    KEEP_THREAD_SAFE();
    return startRequest(
        pbcc_get_users_prep(d_context.data(),
                            options.include_c_strings_array(), 
                            options.include_count(),
                            options.limit(),
                            options.start(),
                            options.end(),
                            options.count()),
        PBTT_GET_USERS);
}


pubnub_res pubnub_qt::create_user(QByteArray const& user_obj, QStringList& include)
{
    include_options inc(include);
    KEEP_THREAD_SAFE();
    d_message_to_send = pack_message_to_gzip(user_obj);
    d_method = (d_message_to_send.size() != user_obj.size())
               ? pubnubSendViaPOSTwithGZIP
               : pubnubSendViaPOST;
    return startRequest(
        pbcc_create_user_prep(d_context.data(),
                              inc.include_c_strings_array(),
                              inc.include_count(),
                              d_message_to_send.data()),
        PBTT_CREATE_USER);
}


pubnub_res pubnub_qt::get_user(QString const& user_id, QStringList& include)
{
    include_options inc(include);
    KEEP_THREAD_SAFE();
    return startRequest(
        pbcc_get_user_prep(d_context.data(),
                           inc.include_c_strings_array(),
                           inc.include_count(),
                           user_id.toLatin1().data()),
        PBTT_GET_USER);
}

    
pubnub_res pubnub_qt::update_user(QByteArray const& user_obj, QStringList& include)
{
    include_options inc(include);
    pubnub_res rslt;
    pbjson_elem parsed_id;
    KEEP_THREAD_SAFE();
    rslt = pbcc_find_objects_id(d_context.data(),
                                user_obj.data(),
                                &parsed_id,
                                __FILE__,
                                __LINE__);
    if (rslt != PNR_OK) {
        return rslt;
    }
    d_message_to_send = pack_message_to_gzip(user_obj);
    d_method = (d_message_to_send.size() != user_obj.size())
               ? pubnubUsePATCHwithGZIP
               : pubnubUsePATCH;
    return startRequest(
        pbcc_update_user_prep(d_context.data(),
                              inc.include_c_strings_array(),
                              inc.include_count(),
                              d_message_to_send.data(),
                              &parsed_id),
        PBTT_UPDATE_USER);
}


pubnub_res pubnub_qt::delete_user(QString const& user_id)
{
    KEEP_THREAD_SAFE();
    return startRequest(
        pbcc_delete_user_prep(d_context.data(),
                              user_id.toLatin1().data()),
        PBTT_DELETE_USER);
}


pubnub_res pubnub_qt::get_spaces(list_options& options)
{
    KEEP_THREAD_SAFE();
    return startRequest(
        pbcc_get_spaces_prep(d_context.data(),
                             options.include_c_strings_array(), 
                             options.include_count(),
                             options.limit(),
                             options.start(),
                             options.end(),
                             options.count()),
        PBTT_GET_SPACES);
}


pubnub_res pubnub_qt::create_space(QByteArray const& space_obj, QStringList& include)
{
    include_options inc(include);
    KEEP_THREAD_SAFE();
    d_message_to_send = pack_message_to_gzip(space_obj);
    d_method = (d_message_to_send.size() != space_obj.size())
               ? pubnubSendViaPOSTwithGZIP
               : pubnubSendViaPOST;
    return startRequest(
        pbcc_create_space_prep(d_context.data(),
                              inc.include_c_strings_array(),
                              inc.include_count(),
                              d_message_to_send.data()),
        PBTT_CREATE_SPACE);
}


pubnub_res pubnub_qt::get_space(QString const& space_id, QStringList& include)
{
    include_options inc(include);
    KEEP_THREAD_SAFE();
    return startRequest(
        pbcc_get_space_prep(d_context.data(),
                            inc.include_c_strings_array(),
                            inc.include_count(),
                            space_id.toLatin1().data()),
        PBTT_GET_SPACE);
}

    
pubnub_res pubnub_qt::update_space(QByteArray const& space_obj, QStringList& include)
{
    include_options inc(include);
    pubnub_res rslt;
    pbjson_elem parsed_id;
    KEEP_THREAD_SAFE();
    rslt = pbcc_find_objects_id(d_context.data(),
                                space_obj.data(),
                                &parsed_id,
                                __FILE__,
                                __LINE__);
    if (rslt != PNR_OK) {
        return rslt;
    }
    d_message_to_send = pack_message_to_gzip(space_obj);
    d_method = (d_message_to_send.size() != space_obj.size())
               ? pubnubUsePATCHwithGZIP
               : pubnubUsePATCH;
    return startRequest(
        pbcc_update_space_prep(d_context.data(),
                               inc.include_c_strings_array(),
                               inc.include_count(),
                               d_message_to_send.data(),
                               &parsed_id),
        PBTT_UPDATE_SPACE);
}


pubnub_res pubnub_qt::delete_space(QString const& space_id)
{
    KEEP_THREAD_SAFE();
    return startRequest(
        pbcc_delete_space_prep(d_context.data(),
                               space_id.toLatin1().data()),
        PBTT_DELETE_SPACE);
}


pubnub_res pubnub_qt::get_memberships(QString const& user_id, list_options& options)
{
    KEEP_THREAD_SAFE();
    return startRequest(
        pbcc_get_memberships_prep(
            d_context.data(),
            user_id.toLatin1().data(),
            options.include_c_strings_array(),
            options.include_count(),
            options.limit(),
            options.start(),
            options.end(),
            options.count()),
        PBTT_GET_MEMBERSHIPS);
}


pubnub_res pubnub_qt::join_spaces(QString const& user_id,
                                  QByteArray const& update_obj,
                                  QStringList& include)
{
    include_options inc(include);
    QByteArray obj("{\"add\":");
    obj.append(update_obj);
    obj.append("}");
    KEEP_THREAD_SAFE();
    d_message_to_send = pack_message_to_gzip(obj);
    d_method = (d_message_to_send.size() != obj.size())
               ? pubnubUsePATCHwithGZIP
               : pubnubUsePATCH;
    return startRequest(
        pbcc_update_memberships_prep(
            d_context.data(),
            user_id.toLatin1().data(),
            inc.include_c_strings_array(),
            inc.include_count(),
            d_message_to_send.data()),
        PBTT_JOIN_SPACES);
}


pubnub_res pubnub_qt::update_memberships(QString const& user_id,
                                         QByteArray const& update_obj,
                                         QStringList& include)
{
    include_options inc(include);
    QByteArray obj("{\"update\":");
    obj.append(update_obj);
    obj.append("}");
    KEEP_THREAD_SAFE();
    d_message_to_send = pack_message_to_gzip(obj);
    d_method = (d_message_to_send.size() != obj.size())
               ? pubnubUsePATCHwithGZIP
               : pubnubUsePATCH;
    return startRequest(
        pbcc_update_memberships_prep(
            d_context.data(),
            user_id.toLatin1().data(),
            inc.include_c_strings_array(),
            inc.include_count(),
            d_message_to_send.data()),
        PBTT_UPDATE_MEMBERSHIPS);
}


pubnub_res pubnub_qt::leave_spaces(QString const& user_id,
                                   QByteArray const& update_obj,
                                   QStringList& include)
{
    include_options inc(include);
    QByteArray obj("{\"remove\":");
    obj.append(update_obj);
    obj.append("}");
    KEEP_THREAD_SAFE();
    d_message_to_send = pack_message_to_gzip(obj);
    d_method = (d_message_to_send.size() != obj.size())
               ? pubnubUsePATCHwithGZIP
               : pubnubUsePATCH;
    return startRequest(
        pbcc_update_memberships_prep(
            d_context.data(),
            user_id.toLatin1().data(),
            inc.include_c_strings_array(),
            inc.include_count(),
            d_message_to_send.data()),
        PBTT_LEAVE_SPACES);
}


pubnub_res pubnub_qt::get_members(QString const& space_id, list_options& options)
{
    KEEP_THREAD_SAFE();
    return startRequest(
        pbcc_get_members_prep(
            d_context.data(),
            space_id.toLatin1().data(),
            options.include_c_strings_array(),
            options.include_count(),
            options.limit(),
            options.start(),
            options.end(),
            options.count()),
        PBTT_GET_MEMBERS);
}


pubnub_res pubnub_qt::add_members(QString const& space_id,
                                  QByteArray const& update_obj,
                                  QStringList& include)
{
    include_options inc(include);
    QByteArray obj("{\"add\":");
    obj.append(update_obj);
    obj.append("}");
    KEEP_THREAD_SAFE();
    d_message_to_send = pack_message_to_gzip(obj);
    d_method = (d_message_to_send.size() != obj.size())
               ? pubnubUsePATCHwithGZIP
               : pubnubUsePATCH;
    return startRequest(
        pbcc_update_members_prep(
            d_context.data(),
            space_id.toLatin1().data(),
            inc.include_c_strings_array(),
            inc.include_count(),
            d_message_to_send.data()),
        PBTT_ADD_MEMBERS);
}


pubnub_res pubnub_qt::update_members(QString const& space_id,
                                     QByteArray const& update_obj,
                                     QStringList& include)
{
    include_options inc(include);
    QByteArray obj("{\"update\":");
    obj.append(update_obj);
    obj.append("}");
    KEEP_THREAD_SAFE();
    d_message_to_send = pack_message_to_gzip(obj);
    d_method = (d_message_to_send.size() != obj.size())
               ? pubnubUsePATCHwithGZIP
               : pubnubUsePATCH;
    return startRequest(
        pbcc_update_members_prep(
            d_context.data(),
            space_id.toLatin1().data(),
            inc.include_c_strings_array(),
            inc.include_count(),
            d_message_to_send.data()),
        PBTT_UPDATE_MEMBERS);
}


pubnub_res pubnub_qt::remove_members(QString const& space_id,
                                     QByteArray const& update_obj,
                                     QStringList& include)
{
    include_options inc(include);
    QByteArray obj("{\"remove\":");
    obj.append(update_obj);
    obj.append("}");
    KEEP_THREAD_SAFE();
    d_message_to_send = pack_message_to_gzip(obj);
    d_method = (d_message_to_send.size() != obj.size())
               ? pubnubUsePATCHwithGZIP
               : pubnubUsePATCH;
    return startRequest(
        pbcc_update_members_prep(
            d_context.data(),
            space_id.toLatin1().data(),
            inc.include_c_strings_array(),
            inc.include_count(),
            d_message_to_send.data()),
        PBTT_REMOVE_MEMBERS);
}
#endif /* PUBNUB_USE_OBJECTS_API */

#if PUBNUB_USE_ACTIONS_API
pubnub_res pubnub_qt::add_action(QString const& channel,
                                 QString const& message_timetoken,
                                 pubnub_action_type actype,
                                 QString const& value)
{
    enum pubnub_res rslt;
    char obj_buffer[PUBNUB_BUF_MAXLEN];
    char const* val = value.toLatin1().data();
    KEEP_THREAD_SAFE();
    rslt = pbcc_form_the_action_object(d_context.data(),
                                       obj_buffer,
                                       sizeof obj_buffer,
                                       actype,
                                       &val);
    if (rslt != PNR_OK) {
        return rslt;
    }
    QByteArray action_object(val);    
    d_message_to_send = pack_message_to_gzip(action_object);
    d_method = (d_message_to_send.size() != action_object.size())
               ? pubnubSendViaPOSTwithGZIP
               : pubnubSendViaPOST;
    return startRequest(
        pbcc_add_action_prep(d_context.data(),
                             channel.toLatin1().data(),
                             message_timetoken.toLatin1().data(),
                             d_message_to_send.data()),
        PBTT_ADD_ACTION);
}


QString pubnub_qt::get_message_timetoken()
{
    KEEP_THREAD_SAFE();
    if (d_trans != PBTT_ADD_ACTION) {
        return QString();
    }
    pubnub_chamebl_t result = pbcc_get_message_timetoken(d_context.data());
    return QString(QByteArray(result.ptr, result.size));
}


QString pubnub_qt::get_action_timetoken()
{
    KEEP_THREAD_SAFE();
    if (d_trans != PBTT_ADD_ACTION) {
        return QString();
    }
    pubnub_chamebl_t result = pbcc_get_action_timetoken(d_context.data());
    return QString(QByteArray(result.ptr, result.size));
}


pubnub_res pubnub_qt::remove_action(QString const& channel,
                                    QString const& message_timetoken,
                                    QString const& action_timetoken)
{
    KEEP_THREAD_SAFE();
    return startRequest(
        pbcc_remove_action_prep(d_context.data(),
                                channel.toLatin1().data(),
                                message_timetoken.toLatin1().data(),
                                action_timetoken.toLatin1().data()),
        PBTT_REMOVE_ACTION);
}


pubnub_res pubnub_qt::get_actions(QString const& channel,
                                  QString const& start,
                                  QString const& end,
                                  size_t limit)
{
    KEEP_THREAD_SAFE();
    return startRequest(
        pbcc_get_actions_prep(d_context.data(),
                              channel.toLatin1().data(),
                              start.isEmpty() ? NULL : start.toLatin1().data(),
                              end.isEmpty() ? NULL : end.toLatin1().data(),
                              limit),
        PBTT_GET_ACTIONS);
}


pubnub_res pubnub_qt::get_actions_more()
{
    KEEP_THREAD_SAFE();
    return startRequest(pbcc_get_actions_more_prep(d_context.data()), PBTT_GET_ACTIONS);
}


pubnub_res pubnub_qt::history_with_actions(QString const& channel,
                                           QString const& start,
                                           QString const& end,
                                           size_t limit)
{
    KEEP_THREAD_SAFE();
    return startRequest(
        pbcc_history_with_actions_prep(d_context.data(),
                                       channel.toLatin1().data(),
                                       start.isEmpty() ? NULL : start.toLatin1().data(),
                                       end.isEmpty() ? NULL : end.toLatin1().data(),
                                       limit),
        PBTT_HISTORY_WITH_ACTIONS);
}


pubnub_res pubnub_qt::history_with_actions_more()
{
    KEEP_THREAD_SAFE();
    return startRequest(pbcc_get_actions_more_prep(d_context.data()), PBTT_HISTORY_WITH_ACTIONS);
}
#endif /* PUBNUB_USE_ACTIONS_API */

int pubnub_qt::last_http_code() const
{
    KEEP_THREAD_SAFE();
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


int pubnub_qt::set_transaction_timeout(int duration_ms)
{
    if (duration_ms > 0) {
        if (duration_ms < PUBNUB_MIN_TRANSACTION_TIMER) {
            return -1;
        }
        QMutexLocker lk(&d_mutex);
        d_transaction_timeout_duration_ms = duration_ms;
        return 0;
    }

    return -1;
}


int pubnub_qt::transaction_timeout_get()
{
    QMutexLocker lk(&d_mutex);
    return d_transaction_timeout_duration_ms;
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
#if PUBNUB_USE_SUBSCRIBE_V2
    case PBTT_SUBSCRIBE_V2:
        pbres = pbcc_parse_subscribe_v2_response(d_context.data());
        break;
#endif
#if PUBNUB_USE_ADVANCED_HISTORY
    case PBTT_MESSAGE_COUNTS:
        pbres = pbcc_parse_message_counts_response(d_context.data());
        break;
#endif
#if PUBNUB_USE_OBJECTS_API
    case PBTT_GET_USERS:
    case PBTT_CREATE_USER:
    case PBTT_GET_USER:
    case PBTT_UPDATE_USER:
    case PBTT_DELETE_USER:
    case PBTT_GET_SPACES:
    case PBTT_CREATE_SPACE:
    case PBTT_GET_SPACE:
    case PBTT_UPDATE_SPACE:
    case PBTT_DELETE_SPACE:
    case PBTT_GET_MEMBERSHIPS:
    case PBTT_JOIN_SPACES:
    case PBTT_UPDATE_MEMBERSHIPS:
    case PBTT_LEAVE_SPACES:
    case PBTT_GET_MEMBERS:
    case PBTT_ADD_MEMBERS:
    case PBTT_UPDATE_MEMBERS:
    case PBTT_REMOVE_MEMBERS:
        pbres = pbcc_parse_objects_api_response(d_context.data());
        break;
#endif
#if PUBNUB_USE_ACTIONS_API
    case PBTT_ADD_ACTION:
    case PBTT_REMOVE_ACTION:
    case PBTT_GET_ACTIONS:
        pbres = pbcc_parse_actions_api_response(d_context.data());
        break;
    case PBTT_HISTORY_WITH_ACTIONS:
        pbres = pbcc_parse_history_with_actions_response(d_context.data());
        break;
#endif /* PUBNUB_USE_ACTIONS_API */
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
        default:
            break;
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
