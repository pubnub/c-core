/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_qt.h"

#include <QTextStream>


class pubnub_qt_sample : public QObject {
    Q_OBJECT
  
public:
    pubnub_qt_sample() 
        : d_pb("demo", "demo")
        , d_out(stdout)
    {}
    
public slots:
    void execute();
    
private slots:
    void onPublish(pubnub_res result);
    void onConnect(pubnub_res result);
    void onSubscribe(pubnub_res result);
    void onTime(pubnub_res result);
    void onHistory(pubnub_res result);
    void onHereNow(pubnub_res result);
    void onWhereNow(pubnub_res result);
    void onSetState(pubnub_res result);
    void onStateGet(pubnub_res result);
            
private:
    bool reconnect(char const *from, char const *to);
    pubnub_qt d_pb;
    QTextStream d_out;
};
