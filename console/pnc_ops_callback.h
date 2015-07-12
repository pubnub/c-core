#include "pubnub_posix_callback.h"

void pnc_ops_init();
void pnc_ops_subscribe(pubnub_t *pn, char *channel);
void pnc_ops_cg_subscribe(pubnub_t *pn, char *channel_group);
void pnc_ops_presence(pubnub_t *pn, char *channel);
void pnc_ops_unsubscribe();
void pnc_ops_presence_unsubscribe();
void pnc_ops_disconnect_and_resubscribe();
void pnc_ops_parse_response(const char *method_name, enum pubnub_res res, pubnub_t *pn);
void pnc_ops_parse_response2(enum pubnub_trans method_name, enum pubnub_res res, pubnub_t *pn);
