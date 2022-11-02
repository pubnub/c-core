#include <FreeRTOS.h>
#include "task.h"
#include "queue.h"

#include "FreeRTOS_IP.h"

#include "pubnub_callback.h"
#include "pubnub_helper.h"
#include "pubnub_timers.h"


static void PubnubTask(void *pvParameters);



void StartPubnubTask(uint16_t usTaskStackSize, UBaseType_t uxTaskPriority)
{
	xTaskCreate(PubnubTask, "Pubnub Callback", usTaskStackSize, NULL, uxTaskPriority, NULL);
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

enum NSampleState {
    sastInit,
    sastPublish,
    sastFirstSubscribe,
    sastSubscribe,
    sastTime
};

struct SampleData {
    enum NSampleState state;
};
static struct SampleData m_data;


static void PubnubCallbackSample(pubnub_t *pbp, enum pubnub_trans trans, enum pubnub_res res, void *user_data)
{
    char const *const chan = "hello_world";
	char const *msg;
    struct SampleData *pData = user_data;

    switch (pData->state) {
    case sastInit:
        PUBNUB_ASSERT_OPT(pbp == NULL);
	    pbp = pubnub_alloc();

	    if (NULL == pbp) {
		    FreeRTOS_printf(("Failed to allocate Pubnub context!\n"));
		    return ;
	    }
	    pubnub_init(pbp, "demo", "demo");

        pubnub_set_transaction_timeout(pbp, PUBNUB_DEFAULT_NON_SUBSCRIBE_TIMEOUT);

        res = pubnub_register_callback(pbp, PubnubCallbackSample, pData);
        if (PNR_OK != res) {
		    FreeRTOS_printf(("Failed to register callback, error: %d\n", res));
		    pubnub_free(pbp);
		    return ;
        }

	    generate_uuid(pbp);
    	pubnub_set_auth(pbp, "danaske");

	    FreeRTOS_printf(("Publishing...\n"));
	    res = pubnub_publish(pbp, chan, "\"Hello world from FreeRTOS callback!\"");
	    if (res != PNR_STARTED) {
		    FreeRTOS_printf(("pubnub_publish() returned unexpected: %d\n", res));
		    pubnub_free(pbp);
		    return ;
	    }
        pData->state = sastPublish;
        break;
    case sastPublish:
        PUBNUB_ASSERT_OPT(PBTT_PUBLISH == trans);
	    if (res == PNR_STARTED) {
		    FreeRTOS_printf(("Publish outcome unexpected: PNR_STARTED(%d)\n", res));
		    pubnub_free(pbp);
            pData->state = sastInit;
		    return ;
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
            pData->state = sastInit;
		    return ;
	    }
        pData->state = sastFirstSubscribe;
        break;
    case sastFirstSubscribe:
        PUBNUB_ASSERT_OPT(PBTT_SUBSCRIBE == trans);
	    if (res == PNR_STARTED) {
		    FreeRTOS_printf(("Subscribe outcome unexpected: PNR_STARTED(%d)\n", res));
		    pubnub_free(pbp);
            pData->state = sastInit;
		    return ;
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
            pData->state = sastInit;
		    return ;
	    }

        pData->state = sastSubscribe;
        break;
    case sastSubscribe:
        PUBNUB_ASSERT_OPT(PBTT_SUBSCRIBE == trans);
	    if (res == PNR_STARTED) {
		    FreeRTOS_printf(("Subscribe outcome unexpected: PNR_STARTED(%d)\n", res));
		    pubnub_free(pbp);
            pData->state = sastInit;
		    return ;
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
            pData->state = sastInit;
		    return ;
	    }

        pData->state = sastTime;
        break;
    case sastTime:
        PUBNUB_ASSERT_OPT(PBTT_TIME == trans);
	    if (res == PNR_STARTED) {
		    FreeRTOS_printf(("Time outcome unexpected: PNR_STARTED(%d)\n", res));
		    pubnub_free(pbp);
            pData->state = sastInit;
		    return ;
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
        FreeRTOS_printf(("Pubnub FreeRTOS callback demo over.\n"));
        pData->state = sastInit;
        break;
    }
}


static void PubnubTask(void *pvParameters)
{
	PUBNUB_UNUSED(pvParameters);
	for( ;; )
	{
		if (m_data.state == sastInit) {
            PubnubCallbackSample(NULL, PBTT_TIME, PNR_OK, &m_data);
        }
		vTaskDelay(pdMS_TO_TICKS(5000));
	}
}
