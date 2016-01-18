#include "pubnub.hpp"

#include <QCoreApplication>

extern "C" {
#include "pubnub_helper.h"
}

using namespace pubnub;


futres::futres(context &ctx, pubnub_res initial) :
  d_ctx(ctx), d_result(initial), d_triggered(false) 
{
  connect(&ctx.d_pbqt, SIGNAL(outcome(pubnub_res)), this, SLOT(onOutcome(pubnub_res)));
}


#if __cplusplus >= 201103L
futres::futres(futres &&x) 
  : d_ctx(x.d_ctx)
  , d_result(x.d_result) 
  , d_triggered(x.d_triggered)
{
  disconnect(&d_ctx.d_pbqt, SIGNAL(outcome(pubnub_res)), &x, SLOT(onOutcome(pubnub_res)));
  connect(&d_ctx.d_pbqt, SIGNAL(outcome(pubnub_res)), this, SLOT(onOutcome(pubnub_res)));
}
#else
futres::futres(futres const &x)
  : d_ctx(x.d_ctx)
  , d_result(x.d_result) 
  , d_triggered(false)
{
  connect(&d_ctx.d_pbqt, SIGNAL(outcome(pubnub_res)), this, SLOT(onOutcome(pubnub_res)));
}
#endif


pubnub_res futres::last_result()
{
  QCoreApplication::processEvents();
  return d_result; 
}


void futres::start_await()
{
}


pubnub_res futres::end_await() 
{
  while (!d_triggered) {
    QCoreApplication::processEvents();
  }
  d_triggered = false;
  return d_result;
}


context::context(std::string pubkey, std::string subkey) :
  d_pbqt(QString::fromStdString(pubkey), QString::fromStdString(subkey))
{
}


std::ostream& operator<<(std::ostream&out, pubnub_res e)
{
  return out << (int)e << '(' << pubnub_res_2_string(e) << ')';
}


QDebug operator<<(QDebug qdbg, pubnub_res e)
{
  QDebugStateSaver saver(qdbg);
  qdbg.nospace() << (int)e << '(' << pubnub_res_2_string(e) << ')';
  return qdbg;
}
