/*******************************************************************************
  MPLAB Harmony Application Source File
  
  Company:
    Microchip Technology Inc.
  
  File Name:
    app.c

  Summary:
    This file contains the source code for the MPLAB Harmony application.

  Description:
    This file contains the source code for the MPLAB Harmony application.  It 
    implements the logic of the application's state machine and it may call 
    API routines of other MPLAB Harmony modules in the system, such as drivers,
    system services, and middleware.  However, it does not call any of the
    system interfaces (such as the "Initialize" and "Tasks" functions) of any of
    the modules in the system or make any assumptions about when those functions
    are called.  That is the responsibility of the configuration-specific system
    files.
 *******************************************************************************/

// DOM-IGNORE-BEGIN
/*******************************************************************************
Copyright (c) 2013-2014 released Microchip Technology Inc.  All rights reserved.

Microchip licenses to you the right to use, modify, copy and distribute
Software only when embedded on a Microchip microcontroller or digital signal
controller that is integrated into your product or third party product
(pursuant to the sublicense terms in the accompanying license agreement).

You should refer to the license agreement accompanying this Software for
additional information regarding your rights and obligations.

SOFTWARE AND DOCUMENTATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF
MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE.
IN NO EVENT SHALL MICROCHIP OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER
CONTRACT, NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR
OTHER LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE OR
CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT OF
SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
(INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.
 *******************************************************************************/
// DOM-IGNORE-END


// *****************************************************************************
// *****************************************************************************
// Section: Included Files 
// *****************************************************************************
// *****************************************************************************

#include "app.h"
#include "app_commands.h"
#include "config.h"
//#include <cyassl/ssl.h>
#include <tcpip/src/hash_fnv.h>

#include "pubnub_callback.h"
#include "pubnub_log.h"
#include "pubnub_timers.h"
#include "pubnub_helper.h"


// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************

// *****************************************************************************
/* Application Data

  Summary:
    Holds application data

  Description:
    This structure holds the application's data.

  Remarks:
    This structure should be initialized by the APP_Initialize function.
    
    Application strings and buffers are be defined outside this structure.
*/

APP_DATA appData;


void APP_Initialize ( void )
{
    memset(&appData, 0, sizeof(appData));
    /* Place the App state machine in its initial state. */
    appData.state = APP_STATE_INIT;
    APP_Commands_Init();
}


static void generate_uuid(pubnub_t *pbp)
{
    char const *uuid_default = "zeka-peka-iz-jendeka";
    struct Pubnub_UUID uuid;
    static struct Pubnub_UUID_String str_uuid;

    if (0 != pubnub_generate_uuid_v4_random(&uuid)) {
        pubnub_set_uuid(pbp, uuid_default);
    }
    else {
        str_uuid = pubnub_uuid_to_string(&uuid);
        pubnub_set_uuid(pbp, str_uuid.uuid);
        PUBNUB_LOG_INFO("Generated UUID: %s\n", str_uuid.uuid);
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
        if (pbp == NULL) {
            pbp = pubnub_alloc();

            if (NULL == pbp) {
                PUBNUB_LOG_INFO("Failed to allocate Pubnub context!\r\n");
                return ;
            }
            pubnub_init(pbp, "demo", "demo");

            pubnub_set_transaction_timeout(pbp, PUBNUB_DEFAULT_NON_SUBSCRIBE_TIMEOUT);

            res = pubnub_register_callback(pbp, PubnubCallbackSample, pData);
            if (PNR_OK != res) {
                PUBNUB_LOG_INFO("Failed to register callback, error: %d\r\n", res);
                pubnub_free(pbp);
                return ;
            }

            generate_uuid(pbp);
            pubnub_set_auth(pbp, "danaske");

            PUBNUB_LOG_INFO("Publishing...\r\n");
            res = pubnub_publish(pbp, chan, "\"Hello world from MPLAB Harmony callback!\"");
            if (res != PNR_STARTED) {
                PUBNUB_LOG_INFO("pubnub_publish() returned unexpected: %d\r\n", res);
                pubnub_free(pbp);
                return ;
            }
            pData->state = sastPublish;
        }
        else {
            PUBNUB_LOG_INFO("Received unexpected outcome: %d\r\n", res);
            pubnub_free(pbp);
            return;
        }
        break;
    case sastPublish:
        PUBNUB_ASSERT_OPT(PBTT_PUBLISH == trans);
	    if (res == PNR_STARTED) {
		    PUBNUB_LOG_INFO("Publish outcome unexpected: PNR_STARTED(%d)\r\n", res);
		    pubnub_free(pbp);
            pData->state = sastInit;
		    return ;
	    }
	    if (PNR_OK == res) {
		    PUBNUB_LOG_INFO("Published! Response from Pubnub: %s\r\n", pubnub_last_publish_result(pbp));
	    }
	    else if (PNR_PUBLISH_FAILED == res) {
		    PUBNUB_LOG_INFO("Published failed on Pubnub, description: %s\r\n", pubnub_last_publish_result(pbp));
	    }
	    else {
		    PUBNUB_LOG_INFO("Publishing failed with code: %d\r\n", res);
            res = pubnub_publish(pbp, chan, "\"Hello world from MPLAB Harmony callback!\"");
            if (res != PNR_STARTED) {
                PUBNUB_LOG_INFO("pubnub_publish() returned unexpected: %d\r\n", res);
                pubnub_free(pbp);
                pData->state = sastInit;
                return ;
            }
            pData->state = sastPublish;
            return;
	    }

	    PUBNUB_LOG_INFO("Subscribing...\r\n");
	    res = pubnub_subscribe(pbp, chan, NULL);
	    if (res != PNR_STARTED) {
		    PUBNUB_LOG_INFO("pubnub_subscribe() returned unexpected: %d\r\n", res);
		    pubnub_free(pbp);
            pData->state = sastInit;
		    return ;
	    }
        pData->state = sastFirstSubscribe;
        break;
    case sastFirstSubscribe:
        PUBNUB_ASSERT_OPT(PBTT_SUBSCRIBE == trans);
	    if (res == PNR_STARTED) {
		    PUBNUB_LOG_INFO("Subscribe outcome unexpected: PNR_STARTED(%d)\r\n", res);
		    pubnub_free(pbp);
            pData->state = sastInit;
		    return ;
	    }

	    if (PNR_OK == res) {
		    PUBNUB_LOG_INFO("Subscribed!\r\n");
	    }
	    else {
		    PUBNUB_LOG_INFO("Subscribing failed with code: %d\r\n", res);
	    }

	    res = pubnub_subscribe(pbp, chan, NULL);
	    if (res != PNR_STARTED) {
		    PUBNUB_LOG_INFO("pubnub_subscribe() returned unexpected: %d\r\n", res);
		    pubnub_free(pbp);
            pData->state = sastInit;
		    return ;
	    }

        pData->state = sastSubscribe;
        break;
    case sastSubscribe:
        PUBNUB_ASSERT_OPT(PBTT_SUBSCRIBE == trans);
	    if (res == PNR_STARTED) {
		    PUBNUB_LOG_INFO("Subscribe outcome unexpected: PNR_STARTED(%d)\r\n", res);
		    pubnub_free(pbp);
            pData->state = sastInit;
		    return ;
	    }
	    if (PNR_OK == res) {
		    PUBNUB_LOG_INFO("Subscribed! Got messages:\r\n");
		    for (;;) {
			    msg = pubnub_get(pbp);
			    if (NULL == msg) {
				    break;
			    }
			    PUBNUB_LOG_INFO("%s\r\n", msg);
		    }
	    }
	    else {
		    PUBNUB_LOG_INFO("Subscribing failed with code: %d (%s)\r\n", res, pubnub_res_2_string(res));
	    }

	    PUBNUB_LOG_INFO("Getting time...\r\n");
	    res = pubnub_time(pbp);
	    if (res != PNR_STARTED) {
		    PUBNUB_LOG_INFO("pubnub_time() returned unexpected: %d\r\n", res);
		    pubnub_free(pbp);
            pData->state = sastInit;
		    return ;
	    }

        pData->state = sastTime;
        break;
    case sastTime:
        PUBNUB_ASSERT_OPT(PBTT_TIME == trans);
	    if (res == PNR_STARTED) {
		    PUBNUB_LOG_INFO("Time outcome unexpected: PNR_STARTED(%d)\r\n", res);
		    pubnub_free(pbp);
            pData->state = sastInit;
		    return ;
	    }

	    if (PNR_OK == res) {
		    PUBNUB_LOG_INFO("Gotten time: %s; last time token=%s\r\n", pubnub_get(pbp), pubnub_last_time_token(pbp));
	    }
	    else {
		    PUBNUB_LOG_INFO("Getting time failed with code: %d\r\n", res);
	    }

       /* We're done */
        if (pubnub_free(pbp) != 0) {
            PUBNUB_LOG_INFO("Failed to free the Pubnub context\r\n");
        }
        PUBNUB_LOG_INFO("Pubnub FreeRTOS callback demo over.\r\n");
        pData->state = sastInit;
        break;
    }
}


/******************************************************************************
  Function:
    void APP_Tasks ( void )

  Remarks:
    See prototype in app.h.
 */
char networkBuffer[256];

void APP_Tasks ( void )
{
    /* Check the application's current state. */
    switch ( appData.state )
    {
        /* Application's initial state. */
        case APP_STATE_INIT:
        {
            appData.state = APP_TCPIP_WAIT_FOR_TCPIP_INIT;
            SYS_CONSOLE_MESSAGE(" APP: Initialization\r\n");
            break;
        }

        case APP_TCPIP_WAIT_FOR_TCPIP_INIT:
        {
            SYS_STATUS tcpipStat;
            tcpipStat = TCPIP_STACK_Status(sysObj.tcpip);
            const char          *netName, *netBiosName;
            int                 i, nNets;
            TCPIP_NET_HANDLE    netH;
            if(tcpipStat < 0)
            {   // some error occurred
                SYS_CONSOLE_MESSAGE(" APP: TCP/IP stack initialization failed!\r\n");
                appData.state = APP_TCPIP_ERROR;
            }
            else if(tcpipStat == SYS_STATUS_READY)
            {
                // now that the stack is ready we can check the
                // available interfaces
                nNets = TCPIP_STACK_NumberOfNetworksGet();
                for(i = 0; i < nNets; i++)
                {

                    netH = TCPIP_STACK_IndexToNet(i);
                    netName = TCPIP_STACK_NetNameGet(netH);
                    netBiosName = TCPIP_STACK_NetBIOSName(netH);

#if defined(TCPIP_STACK_USE_NBNS)
                    SYS_CONSOLE_PRINT("    Interface %s on host %s - NBNS enabled\r\n", netName, netBiosName);
#else
                    SYS_CONSOLE_PRINT("    Interface %s on host %s - NBNS disabled\r\n", netName, netBiosName);
#endif  // defined(TCPIP_STACK_USE_NBNS)

                }
                appData.state = APP_TCPIP_WAIT_FOR_IP;

            }
            break;
        }
        case APP_TCPIP_WAIT_FOR_IP:
        {
            int                 i, nNets;
            TCPIP_NET_HANDLE    netH;
            IPV4_ADDR           ipAddr;
            // if the IP address of an interface has changed
            // display the new value on the system console
            nNets = TCPIP_STACK_NumberOfNetworksGet();

            for (i = 0; i < nNets; i++)
            {
                netH = TCPIP_STACK_IndexToNet(i);
                ipAddr.Val = TCPIP_STACK_NetAddress(netH);
                if(0 != ipAddr.Val)
                {
                    SYS_CONSOLE_MESSAGE(TCPIP_STACK_NetNameGet(netH));
                    SYS_CONSOLE_MESSAGE(" IP Address: ");
                    SYS_CONSOLE_PRINT("%d.%d.%d.%d \r\n", ipAddr.v[0], ipAddr.v[1], ipAddr.v[2], ipAddr.v[3]);
                    if (ipAddr.v[0] != 0 && ipAddr.v[0] != 169) // Wait for a Valid IP
                    {
                        appData.state = APP_TCPIP_RUNNING_PUBNUB_DEMO;
                        SYS_CONSOLE_MESSAGE("Starting demo\r\n");
                        PubnubCallbackSample(NULL, PBTT_PUBLISH, PNR_OK, &m_data);
                    }
                }
            }
            break;
        }
        case APP_TCPIP_RUNNING_PUBNUB_DEMO:
            pubnub_task();
            break;

        default:
        {
            /* TODO: Handle error in application's state machine. */
            break;
        }
    }
}
 

/*******************************************************************************
 End of File
 */

