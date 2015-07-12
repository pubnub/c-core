#include "pnc_ops_callback.h"
#include "pnc_config.h"

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>

static pthread_mutex_t mutw;
static pthread_cond_t condw;

void sample_callback(pubnub_t *pb, enum pubnub_trans trans, enum pubnub_res result)
{
  switch (trans) {
    case PBTT_PUBLISH:
      printf("Published, result: %d\n", result);
      break;
    case PBTT_SUBSCRIBE:
      printf("Published, result: %d\n", result);
      break;
    case PBTT_LEAVE:
      printf("Left, result: %d\n", result);
      break;
    case PBTT_TIME:
      printf("Timed, result: %d\n", result);
      break;
    case PBTT_HISTORY:
      printf("Historied, result: %d\n", result);
      break;
    case PBTT_HISTORYV2:
      printf("Historied v2, result: %d\n", result);
      break;
    default:
      printf("None?! result: %d\n", result);
      break;
  }

  //pthread_cond_signal(&condw);
  pthread_mutex_unlock(&mutw);
  pnc_ops_parse_response2(trans, result, pb);
}

void pnc_ops_init(pubnub_t *pn) {
  pthread_mutex_init(&mutw, NULL);
  pthread_cond_init(&condw, NULL);
  pubnub_register_callback(pn, sample_callback);
}

void pnc_ops_subscribe(pubnub_t *pn, char *channel) {
  // TODO: add separate thread for subscription
  // The first invokation of pubnub_subscribe() will fetch initial timetoken.
  pnc_ops_parse_response("pubnub_subscribe() connect",
      pubnub_subscribe(pn, channel, NULL), pn);

  // Now you connected to your channel.
  // Then you should invoke it again to start listenting for messages.
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
  puts("TBD");
}

void pnc_ops_presence_unsubscribe() {
  puts("TBD");
}

void pnc_ops_disconnect_and_resubscribe() {
  puts("TBD");
}

void pnc_ops_parse_response(const char *method_name, enum pubnub_res res, pubnub_t *pn) {

  if (res != PNR_STARTED) {
    printf("%s returned unexpected: %d\n", method_name, res);
    pubnub_free(pn);
    return;
  }

  pthread_mutex_lock(&mutw);
}

void pnc_ops_parse_response2(enum pubnub_trans method_name, enum pubnub_res res, pubnub_t *pn) {
  const char *msg;
  res = pubnub_last_result(pn);

  if (res == PNR_HTTP_ERROR) {
    printf("%d failed to parse message as string. HTTP code: %d\n",
        method_name, pubnub_last_http_code(pn));
    pubnub_free(pn);
    return;
  }

  if (res == PNR_STARTED) {
    printf("%d returned unexpected: PNR_STARTED(%d)\n", method_name, res);
    pubnub_free(pn);
    return;
  }

  if (PNR_OK == res) {
    printf("%d success!\n", method_name);
    for (;;) {
      msg = pubnub_get(pn);
      if (NULL == msg) {
        break;
      }
      puts(msg);
    }
  } else {
    printf("%d failed!\n", method_name);
  }
}
