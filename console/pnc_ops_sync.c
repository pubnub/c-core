#include "pnc_ops_sync.h"

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

void pnc_ops_init() {
  // Nothing todo in sync version
}

void pnc_ops_subscribe(pubnub_t *pn_sub) {
  puts("Subscribe will show the first message and return to select operation loop.");

  char channels_string[PNC_SUBSCRIBE_CHANNELS_LIMIT * PNC_CHANNEL_NAME_SIZE];
  char channel_groups_string[PNC_SUBSCRIBE_CHANNELS_LIMIT * PNC_CHANNEL_NAME_SIZE];
  pnc_subscribe_list_channels(channels_string);
  pnc_subscribe_list_channel_groups(channel_groups_string);

  int i;
  enum pubnub_res res;

  for (i = 0; i < 2; i++) {
    puts("Subscribe loop...");

    if (strlen(channels_string) == 0 && strlen(channel_groups_string) == 0) {
      puts("You need add some channels or channel groups first. Ignoring");
    } else if (strlen(channel_groups_string) == 0) {
      res = pubnub_subscribe(pn_sub, channels_string, NULL);
    } else if (strlen(channels_string) == 0) {
      res = pubnub_subscribe(pn_sub, NULL, channel_groups_string);
    } else {
      res = pubnub_subscribe(pn_sub, channels_string, channel_groups_string);
    }

    pnc_ops_parse_response("pubnub_subscribe()", res, pn_sub);
  }
}

void pnc_ops_unsubscribe(pubnub_t *pn) {
  puts("Not implemented in sync version");
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
    puts("\n****************************************");
    printf("Result for %s: success!\n", method_name);
    puts("****************************************");
    for (;;) {
      msg = pubnub_get(pn);
      if (NULL == msg) {
        break;
      }
      puts(msg);
    }
    puts("****************************************");
  } else {
    printf("%s failed! res code is %d\n", method_name, res);
  }
}
