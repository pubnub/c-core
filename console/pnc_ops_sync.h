#include "pubnub_posix_sync.h"
#include "pnc_config.h"

void pnc_ops_subscribe(pubnub_t *pn, char *channel);
void pnc_ops_cg_subscribe(pubnub_t *pn, char *channel_group);
void pnc_ops_publish(pubnub_t *pn, char *channel, bool store);
void pnc_ops_presence(pubnub_t *pn, char *channel);
void pnc_ops_history(pubnub_t *pn, char *channel, int count, bool includeToken);
void pnc_ops_here_now(pubnub_t *pn, char *channel);
void pnc_ops_unsubscribe();
void pnc_ops_presence_unsubscribe();
void pnc_ops_time(pubnub_t *pn);
void pnc_ops_parse_response(const char *method_name, enum pubnub_res res, pubnub_t *pn);
void pnc_ops_disconnect_and_resubscribe();
