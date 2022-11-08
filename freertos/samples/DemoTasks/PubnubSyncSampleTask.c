#include <FreeRTOS.h>
#include "task.h"
#include "queue.h"

#include "FreeRTOS_IP.h"

#include "pubnub_sync.h"
#include "pubnub_helper.h"



static void PubnubTask(void *pvParameters);



void StartPubnubTask(uint16_t usTaskStackSize, UBaseType_t uxTaskPriority)
{
	xTaskCreate(PubnubTask, "Pubnub Sync", usTaskStackSize, NULL, uxTaskPriority, NULL);
}


static void generate_uuid(pubnub_t *pbp)
{
    char const *uuid_default = "zeka-peka-iz-jendeka";
    struct Pubnub_UUID uuid;
    static struct Pubnub_UUID_String str_uuid;

    if (0 != pubnub_generate_uuid_v4_random(&uuid)) {
        pubnub_set_user_id(pbp, uuid_default);
    }
    else {
        str_uuid = pubnub_uuid_to_string(&uuid);
        pubnub_set_user_id(pbp, str_uuid.uuid);
        FreeRTOS_printf(("Generated UUID: %s\n", str_uuid.uuid));
    }
}


static int PubnubSyncSample(char const *chan)
{
	char const *msg;
	enum pubnub_res res;
	pubnub_t *pbp = pubnub_alloc();

	if (NULL == pbp) {
		FreeRTOS_printf(("Failed to allocate Pubnub context!\n"));
		return -1;
	}
	pubnub_init(pbp, "demo", "demo");

	generate_uuid(pbp);

	pubnub_set_auth(pbp, "danaske");

	FreeRTOS_printf(("Publishing...\n"));
	res = pubnub_publish(pbp, chan, "\"Hello world from FreeRTOS sync!\"");
	if (res != PNR_STARTED) {
		FreeRTOS_printf(("pubnub_publish() returned unexpected: %d\n", res));
		pubnub_free(pbp);
		return -1;
	}
	res = pubnub_await(pbp);
	if (res == PNR_STARTED) {
		FreeRTOS_printf(("pubnub_await() returned unexpected: PNR_STARTED(%d)\n", res));
		pubnub_free(pbp);
		return -1;
	}
	if (PNR_OK == res) {
		FreeRTOS_printf(("Published! Response from Pubnub: %s\n", pubnub_last_publish_result(pbp)));
	}
	else if (PNR_PUBLISH_FAILED == res) {
		FreeRTOS_printf(("Published failed on Pubnub, description: %s\n", pubnub_last_publish_result(pbp)));
	}
	else {
		FreeRTOS_printf(("Publishing failed with code: %d\n", res));
	}

	FreeRTOS_printf(("Subscribing...\n"));
	res = pubnub_subscribe(pbp, chan, NULL);
	if (res != PNR_STARTED) {
		FreeRTOS_printf(("pubnub_subscribe() returned unexpected: %d\n", res));
		pubnub_free(pbp);
		return -1;
	}
	res = pubnub_await(pbp);
	if (res == PNR_STARTED) {
		FreeRTOS_printf(("pubnub_await() returned unexpected: PNR_STARTED(%d)\n", res));
		pubnub_free(pbp);
		return -1;
	}

	if (PNR_OK == res) {
		FreeRTOS_printf(("Subscribed!\n"));
	}
	else {
		FreeRTOS_printf(("Subscribing failed with code: %d\n", res));
	}

	res = pubnub_subscribe(pbp, chan, NULL);
	if (res != PNR_STARTED) {
		FreeRTOS_printf(("pubnub_subscribe() returned unexpected: %d\n", res));
		pubnub_free(pbp);
		return -1;
	}
	res = pubnub_await(pbp);
	if (res == PNR_STARTED) {
		FreeRTOS_printf(("pubnub_await() returned unexpected: PNR_STARTED(%d)\n", res));
		pubnub_free(pbp);
		return -1;
	}

	if (PNR_OK == res) {
		FreeRTOS_printf(("Subscribed! Got messages:\n"));
		for (;;) {
			msg = pubnub_get(pbp);
			if (NULL == msg) {
				break;
			}
			FreeRTOS_printf(("%s\n", msg));
		}
	}
	else {
		FreeRTOS_printf(("Subscribing failed with code: %d (%s)\n", res, pubnub_res_2_string(res)));
	}

	FreeRTOS_printf(("Getting time...\n"));
	res = pubnub_time(pbp);
	if (res != PNR_STARTED) {
		FreeRTOS_printf(("pubnub_time() returned unexpected: %d\n", res));
		pubnub_free(pbp);
		return -1;
	}
	res = pubnub_await(pbp);
	if (res == PNR_STARTED) {
		FreeRTOS_printf(("pubnub_await() returned unexpected: PNR_STARTED(%d)\n", res));
		pubnub_free(pbp);
		return -1;
	}

	if (PNR_OK == res) {
		FreeRTOS_printf(("Gotten time: %s; last time token=%s\n", pubnub_get(pbp), pubnub_last_time_token(pbp)));
	}
	else {
		FreeRTOS_printf(("Getting time failed with code: %d\n", res));
	}

   /* We're done */
    if (pubnub_free(pbp) != 0) {
        FreeRTOS_printf(("Failed to free the Pubnub context\n"));
    }

    FreeRTOS_printf(("Pubnub FreeRTOS sync demo over.\n"));

    return 0;
}


static void PubnubTask(void *pvParameters)
{
	PUBNUB_UNUSED(pvParameters);
	for( ;; )
	{
		PubnubSyncSample("hello_world");
		vTaskDelay(pdMS_TO_TICKS(10000));
	}
}
