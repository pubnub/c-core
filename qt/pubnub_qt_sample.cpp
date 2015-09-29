/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_qt_sample.h"

extern "C" {
#include "pubnub_helper.h"
}

#include <QCoreApplication>
#include <QTimer>



const QString chann = "hello_world";


bool pubnub_qt_sample::reconnect(char const *from, char const *to)
{
    disconnect(&d_pb, SIGNAL(outcome(pubnub_res)), this, from);
    return connect(&d_pb, SIGNAL(outcome(pubnub_res)), this, to);
}


void pubnub_qt_sample::onPublish(pubnub_res result)
{
    d_out << "onPublish! Result: '" << pubnub_res_2_string(result) << "', Response: " << d_pb.last_publish_result() << "\n";
    
    reconnect(SLOT(onPublish(pubnub_res)), SLOT(onConnect(pubnub_res)));
    result = d_pb.subscribe(chann);
    if (result != PNR_STARTED) {
        d_out << "Subscribe failed, result: '"<< pubnub_res_2_string(result) << "'\n";
        QCoreApplication::instance()->quit();
    }
}


void pubnub_qt_sample::onConnect(pubnub_res result)
{
    d_out << "onConnect! Result: '" << pubnub_res_2_string(result) << "'\n";
    reconnect(SLOT(onConnect(pubnub_res)), SLOT(onSubscribe(pubnub_res)));
    result = d_pb.subscribe(chann);
    if (result != PNR_STARTED) {
        d_out << "Subscribe failed, result: '"<< pubnub_res_2_string(result) << "'\n";
        QCoreApplication::instance()->quit();
    }
}


void pubnub_qt_sample::onSubscribe(pubnub_res result)
{
    d_out << "onSubscribe! Result: '" << pubnub_res_2_string(result) << "' Messages:\n";
    QList<QString> msg = d_pb.get_all();
    for (int i = 0; i < msg.size(); ++i) {
        d_out << msg[i].toLatin1().data() << '\n';
    }

    reconnect(SLOT(onSubscribe(pubnub_res)), SLOT(onTime(pubnub_res)));
    result = d_pb.time();
    if (result != PNR_STARTED) {
        d_out << "Time failed, result: '"<< pubnub_res_2_string(result) << "'\n";
        QCoreApplication::instance()->quit();
    }
}

void pubnub_qt_sample::onTime(pubnub_res result)
{
    d_out << "onTime! Result: '" << pubnub_res_2_string(result) << "' Time:" << d_pb.get() << '\n';

    reconnect(SLOT(onTime(pubnub_res)), SLOT(onHistory(pubnub_res)));
    result = d_pb.history(chann);
    if (result != PNR_STARTED) {
        d_out << "History failed, result: '"<< pubnub_res_2_string(result) << "'\n";
        QCoreApplication::instance()->quit();
    }
}


void pubnub_qt_sample::onHistory(pubnub_res result)
{
    d_out << "onHistory! Result: '" << pubnub_res_2_string(result) << "' Messages:\n";
    QList<QString> msg = d_pb.get_all();
    for (int i = 0; i < msg.size(); ++i) {
        d_out << msg[i].toLatin1().data() << '\n';
    }

    reconnect(SLOT(onHistory(pubnub_res)), SLOT(onHereNow(pubnub_res)));
    result = d_pb.here_now(chann);
    if (result != PNR_STARTED) {
        d_out << "Hhere now failed, result: '"<< pubnub_res_2_string(result) << "'\n";
        QCoreApplication::instance()->quit();
    }
}

void pubnub_qt_sample::onHereNow(pubnub_res result)
{
   d_out << "onHereNow! Result: '" << pubnub_res_2_string(result) << "' Response:\n" << d_pb.get() << "\n";
 
    reconnect(SLOT(onHereNow(pubnub_res)), SLOT(onWhereNow(pubnub_res)));
    result = d_pb.where_now();
    if (result != PNR_STARTED) {
        d_out << "Where now failed, result: '"<< pubnub_res_2_string(result) << "'\n";
        QCoreApplication::instance()->quit();
    }
}


void pubnub_qt_sample::onWhereNow(pubnub_res result)
{
    d_out << "onWhereNow! Result: '" << pubnub_res_2_string(result) << "' Response:\n" << d_pb.get() << "\n";

    reconnect(SLOT(onWhereNow(pubnub_res)), SLOT(onSetState(pubnub_res)));
    result = d_pb.set_state(chann, "", d_pb.uuid(), "{\"a\":1}");
    if (result != PNR_STARTED) {
        d_out << "Set state failed, result: '"<< pubnub_res_2_string(result) << "'\n";
        QCoreApplication::instance()->quit();
    }
}

void pubnub_qt_sample::onSetState(pubnub_res result)
{
    d_out << "onSetState! Result: '" << pubnub_res_2_string(result) << "' Response:\n" << d_pb.get() << "\n";

    reconnect(SLOT(onSetState(pubnub_res)), SLOT(onStateGet(pubnub_res)));
    result = d_pb.state_get(chann);
    if (result != PNR_STARTED) {
        d_out << "State get failed, result: '"<< pubnub_res_2_string(result) << "'\n";
        QCoreApplication::instance()->quit();
    }
}

void pubnub_qt_sample::onStateGet(pubnub_res result)
{
    d_out << "onStateGet! Result: '" << pubnub_res_2_string(result) << "' Response:\n" << d_pb.get() << "\n";
    QCoreApplication::instance()->quit();
}


void pubnub_qt_sample::execute()
{
    qDebug() << "execute()";
    
//    d_pb.set_ssl_options(0);
    d_pb.set_uuid_v4_random();

    connect(&d_pb, SIGNAL(outcome(pubnub_res)), this, SLOT(onPublish(pubnub_res)));
    pubnub_res result = d_pb.publish(chann, "\"Hello world from Qt!\"");
    if (result != PNR_STARTED) {
        d_out << "Subscribe failed, result: '"<< pubnub_res_2_string(result) << "'\n";
        QCoreApplication::instance()->quit();
    }
}


int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    pubnub_qt_sample sample;
    
    QTimer::singleShot(0, &sample, SLOT(execute()));
    
    return app.exec();
}

