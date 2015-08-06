#include "pubnub_callback.h"
#include "pnc_ops_callback.h"

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>

static pthread_mutex_t mutw;
static pthread_mutex_t mutw_sub;
static pthread_cond_t condw;
static pthread_cond_t condw_sub;

static bool loop_enabled = true;
pthread_t sub_thread;

void pnc_ops_init(pubnub_t *pn, pubnub_t *pn_sub) {
  pthread_mutex_init(&mutw, NULL);
  pthread_mutex_init(&mutw_sub, NULL);

  pubnub_register_callback(pn, ops_callback);
  pubnub_register_callback(pn_sub, subscribe_callback);
}

void ops_callback(pubnub_t *pn, enum pubnub_trans trans, enum pubnub_res result) {
  puts("\n");

  switch (trans) {
    case PBTT_PUBLISH:
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

  pnc_ops_parse_callback(trans, result, pn);
  pthread_cond_signal(&condw);
  pthread_mutex_unlock(&mutw);
}

void subscribe_callback(pubnub_t *pn_sub, enum pubnub_trans trans, enum pubnub_res result) {
  if (trans != PBTT_SUBSCRIBE) {
      printf("Non-subsribe response on subscribe instance: %d\n", result);
      return;
  }

  pnc_ops_parse_callback(trans, result, pn_sub);
  pthread_cond_signal(&condw_sub);
  pthread_mutex_unlock(&mutw_sub);
}

void pnc_ops_subscribe(pubnub_t *pn_sub) {
  pthread_create(&sub_thread, NULL, (void *) pnc_ops_subscribe_thr, (void *) pn_sub);
}

void pnc_ops_subscribe_thr(void *pn_sub_addr) {
  puts("Subscribe thread created");

  pubnub_t *pn_sub = (pubnub_t *) pn_sub_addr;
  enum pubnub_res res;

  char channels_string[PNC_SUBSCRIBE_CHANNELS_LIMIT * PNC_CHANNEL_NAME_SIZE];
  char channel_groups_string[PNC_SUBSCRIBE_CHANNELS_LIMIT * PNC_CHANNEL_NAME_SIZE];

  while (loop_enabled) {
    puts("Subscribe loop...");

    pnc_subscribe_list_channels(channels_string);
    pnc_subscribe_list_channel_groups(channel_groups_string);

    pthread_mutex_lock(&mutw_sub);

    if (strlen(channels_string) == 0 && strlen(channel_groups_string) == 0) {
      puts("You need add some channels or channel groups first. Ignoring");
    } else if (strlen(channel_groups_string) == 0) {
      res = pubnub_subscribe(pn_sub, channels_string, NULL);
    } else if (strlen(channels_string) == 0) {
      res = pubnub_subscribe(pn_sub, NULL, channel_groups_string);
    } else {
      res = pubnub_subscribe(pn_sub, channels_string, channel_groups_string);
    }

    if (res != PNR_STARTED) {
      printf("%s returned unexpected: %d\n", "pubnub_subscribe()", res);
      break;
    }

    pthread_cond_wait(&condw_sub, &mutw_sub);
  }

  pthread_exit(NULL);
}

void pnc_ops_unsubscribe(pubnub_t *pn_sub) {
  loop_enabled = false;
  puts("Subscription loop is disabled and will be stopped after the next message received");
  // pthread_cancel(sub_thread);
}

void pnc_ops_parse_response(const char *method_name, enum pubnub_res res, pubnub_t *pn) {
  if (res != PNR_STARTED) {
    printf("%s returned unexpected: %d\n", method_name, res);
    return;
  }

  pthread_mutex_lock(&mutw);
  pthread_cond_wait(&condw, &mutw);
}

void pnc_ops_parse_callback(enum pubnub_trans method_name, enum pubnub_res res, pubnub_t *pn) {
  const char *msg;
  res = pubnub_last_result(pn);

  if (res == PNR_HTTP_ERROR) {
    printf("%d failed to parse message as string. HTTP code: %d\n",
        method_name, pubnub_last_http_code(pn));
    return;
  }

  if (res == PNR_STARTED) {
    printf("%d returned unexpected: PNR_STARTED(%d)\n", method_name, res);
    return;
  }

  if (PNR_OK == res) {
    puts("***************************");
    printf("Result for %d: success!\n", method_name);
    puts("***************************");
    for (;;) {
      msg = pubnub_get(pn);
      if (NULL == msg) {
        break;
      }
      puts(msg);
    }
    puts("***************************");
  } else {
    printf("%d failed!\n", method_name);
  }
}
