/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_qt_gui_sample.h"

extern "C" {
#include "pubnub_helper.h"
}

#include <QApplication>
#include <QLineEdit>
#include <QTextEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QScrollBar>


void pubnub_qt_gui_sample::resetPubnub()
{
    d_pb_publish.reset(new pubnub_qt(d_pubkey->text(), d_keysub->text()));
    connect(d_pb_publish.data(), SIGNAL(outcome(pubnub_res)), this, SLOT(onPublish(pubnub_res)));

    d_pb_subscribe.reset(new pubnub_qt(d_pubkey->text(), d_keysub->text()));
    connect(d_pb_subscribe.data(), SIGNAL(outcome(pubnub_res)), this, SLOT(onSubscribe(pubnub_res)));
    d_pb_subscribe->subscribe(d_channel->text());
}


pubnub_qt_gui_sample::pubnub_qt_gui_sample()
{
    d_pubkey = new QLineEdit("demo");
    connect(d_pubkey, SIGNAL(returnPressed()), this, SLOT(pubkeyChanged()));
    d_keysub = new QLineEdit("demo");
    connect(d_keysub, SIGNAL(returnPressed()), this, SLOT(keysubChanged()));
    d_channel = new QLineEdit("hello_world");
    connect(d_channel, SIGNAL(returnPressed()), this, SLOT(channelChanged()));

    d_message = new QLineEdit("{\"text\": \"Pubnub-Qt\"}");
    connect(d_message, SIGNAL(returnPressed()), this, SLOT(messageChanged()));

    d_console = new QTextEdit;
    d_console->setFocusPolicy(Qt::NoFocus);
    d_console->setReadOnly(true);

    resetPubnub();
 
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(new QLabel("Publish key"));
    layout->addWidget(d_pubkey);
    layout->addWidget(new QLabel("Subscribe key"));
    layout->addWidget(d_keysub);
    layout->addWidget(new QLabel("Channel"));
    layout->addWidget(d_channel);
    layout->addWidget(new QLabel("Message"));
    layout->addWidget(d_message);
    layout->addWidget(new QLabel("Console"));
    layout->addWidget(d_console);

    setLayout(layout);

    setWindowTitle("Pubnub Qt Debug Console");
}


void pubnub_qt_gui_sample::onPublish(pubnub_res result)
{
    QString report =  QString("Publish result: '") + pubnub_res_2_string(result) + "', response: " + d_pb_publish->last_publish_result() + "\n";

    d_console->insertPlainText(report);
    QScrollBar *bar = d_console->verticalScrollBar();
    bar->setValue(bar->maximum());
}


void pubnub_qt_gui_sample::onSubscribe(pubnub_res result)
{
    if (PNR_OK != result) {
        d_console->insertPlainText(QString("Subscribe failed, result: ") + pubnub_res_2_string(result) + '\n');
    }
    else {
        QList<QString> msg = d_pb_subscribe->get_all();
        for (int i = 0; i < msg.size(); ++i) {
            d_console->insertPlainText(msg[i] + '\n');
        }
    }

    QScrollBar *bar = d_console->verticalScrollBar();
    bar->setValue(bar->maximum());

    result = d_pb_subscribe->subscribe(d_channel->text());
    if (result != PNR_STARTED) {
        d_console->insertPlainText(QString("Subscribe failed, result: '") + pubnub_res_2_string(result) + "'\n");
    }
}


void pubnub_qt_gui_sample::pubkeyChanged()
{
    qDebug() << "pubkeyChanged()";
    resetPubnub();
}


void pubnub_qt_gui_sample::keysubChanged()
{
    qDebug() << "keysubChanged()";
    resetPubnub();
}


void pubnub_qt_gui_sample::channelChanged()
{
    qDebug() << "channelChanged()";
    d_pb_subscribe->cancel();
}


void pubnub_qt_gui_sample::messageChanged()
{
    qDebug() << "messageChanged()";
    pubnub_res result = d_pb_publish->publish(d_channel->text(), d_message->text());
    if (result != PNR_STARTED) {
        d_console->insertPlainText(QString("Publish failed, result: '") + pubnub_res_2_string(result) + "'\n");
    }
}


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    pubnub_qt_gui_sample sample;
    
    sample.show();
    
    return app.exec();
}

