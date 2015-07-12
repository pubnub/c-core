#include "pnc_ops_sync.h"

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <json/json.h>

void pnc_ops_init() {
  // Nothing todo in sync version
}

void pnc_ops_subscribe(pubnub_t *pn, char *channel) {
  // TODO: add check for connected
  puts("Subscription is sync and will return to operations loop after the first message received");
  // The first invokation of pubnub_subscribe() will fetch initial timetoken.
  pnc_ops_parse_response("pubnub_subscribe() connect",
      pubnub_subscribe(pn, channel, NULL), pn);

  // Now you connected to your channel.
  // To start listening for messages you should invoke it again
  pnc_ops_parse_response("pubnub_subscribe() message",
      pubnub_subscribe(pn, channel, NULL), pn);
}

void pnc_ops_cg_subscribe(pubnub_t *pn, char *channel_group) {
  puts("Subscription is sync and will return to operations loop after the first message received");
  // The first invokation of pubnub_subscribe() will fetch initial timetoken.
  pnc_ops_parse_response("pubnub_subscribe() connect",
      pubnub_subscribe(pn, NULL, channel_group), pn);

  // Now you connected to your channel group.
  // To start listening for messages you should invoke it again
  pnc_ops_parse_response("pubnub_subscribe() message",
      pubnub_subscribe(pn, NULL, channel_group), pn);
}

void pnc_ops_presence(pubnub_t *pn, char *channel) {
  char presence_channel[PNC_CHANNEL_NAME_SIZE + strlen(PNC_PRESENCE_SUFFIX)];

  strcpy(presence_channel, channel);
  strcat(presence_channel, PNC_PRESENCE_SUFFIX);

  puts("Presence is sync and will return to operations loop after the first message received");
  printf("Subscribing to %s\n", presence_channel);
  // The first invokation of pubnub_subscribe() will fetch initial timetoken.
  pnc_ops_parse_response("pubnub_subscribe() connect",
      pubnub_subscribe(pn, presence_channel, NULL), pn);

  // Now you connected to your presence channel.
  // To start listening for events you should invoke it again
  pnc_ops_parse_response("pubnub_subscribe() message",
      pubnub_subscribe(pn, presence_channel, NULL), pn);
}

void pnc_ops_unsubscribe() {
  puts("ERROR: pnc_ops_unsubscribe() is not implemented in sync version of console");
}

void pnc_ops_presence_unsubscribe() {
  puts("ERROR: pnc_ops_presence_unsubscribe() is not implemented in sync version of console");
}

void pnc_ops_disconnect_and_resubscribe() {
  puts("ERROR: pnc_ops_disconnect_and_resubscribe() is not implemented in sync version of console");
}

void pnc_ops_parse_response(const char *method_name, enum pubnub_res res, pubnub_t *pn) {
  const char *msg;

  if (res != PNR_STARTED) {
    printf("%s another request is already strated: %d\n", method_name, res);
    return;
  }

  res = pubnub_await(pn);

  if (res == PNR_HTTP_ERROR) {
    printf("%s HTTP error. Response code: %d\n",
        method_name, pubnub_last_http_code(pn));
    return;
  }

  if (res == PNR_STARTED) {
    printf("%s returned unexpected: PNR_STARTED(%d)\n", method_name, res);
    return;
  }

  if (PNR_OK == res) {
    printf("%s success!\n", method_name);
    for (;;) {
      msg = pubnub_get(pn);
      if (NULL == msg) {
        break;
      }
      puts(msg);
    }
  } else {
    printf("%s failed! res code is %d\n", method_name, res);
  }
}
