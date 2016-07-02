/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_callback.h"

#include "pubnub_helper.h"
#include "pubnub_mutex.h"

#include "pnc_ops_callback.h"

#if defined _WIN32
#include <windows.h>
#include <process.h>
#else
#include <pthread.h>
#endif

#include <stdio.h>
#include <stdbool.h>
#include <string.h>


struct UserData {
#if defined _WIN32
    HANDLE condw;
#else
    pthread_mutex_t mutw;
    bool triggered;
    pthread_cond_t condw;
#endif
    pubnub_t *pb;
};

static struct UserData m_user_data;
static struct UserData m_user_data_sub;


#if defined _WIN32
uintptr_t sub_thread;
#else
pthread_t sub_thread;
#endif
static pubnub_mutex_t m_loop_enabled_mutex;
static bool m_loop_enabled = false;



static void callback_signal(struct UserData *pUserData)
{
#if defined _WIN32
    SetEvent(pUserData->condw);
#else
    pthread_mutex_lock(&pUserData->mutw);
    pUserData->triggered = true;
    pthread_cond_signal(&pUserData->condw);
    pthread_mutex_unlock(&pUserData->mutw);
#endif
}


static void ops_callback(pubnub_t *pn, enum pubnub_trans trans, enum pubnub_res result, void *user_data)
{
    struct UserData *pUserData = (struct UserData*)user_data;

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
    default:
        printf("None?! result: %d\n", result);
        break;
    }
    
    pnc_ops_parse_callback(trans, result, pn);

    callback_signal(pUserData);
}


static void subscribe_callback(pubnub_t *pn_sub, enum pubnub_trans trans, enum pubnub_res result, void *user_data)
{
    struct UserData *pUserData = (struct UserData*)user_data;

    if (trans != PBTT_SUBSCRIBE) {
        printf("Non-subsribe response on subscribe instance: %d\n", result);
        return;
    }
    
    pnc_ops_parse_callback(trans, result, pn_sub);

    callback_signal(pUserData);
}


static void InitUserData(struct UserData *pUserData, pubnub_t *pb)
{
#if defined _WIN32
    pUserData->condw = CreateEvent(NULL, TRUE, FALSE, NULL);
#else
    pthread_mutex_init(&pUserData->mutw, NULL);
    pthread_cond_init(&pUserData->condw, NULL);
#endif
    pUserData->pb = pb;
}


static void await(struct UserData *pUserData)
{
#if defined _WIN32	
    ResetEvent(pUserData->condw);
    WaitForSingleObject(pUserData->condw, INFINITE);
#else
    pthread_mutex_lock(&pUserData->mutw);
    pUserData->triggered = false;
    while (!pUserData->triggered) {
        pthread_cond_wait(&pUserData->condw, &pUserData->mutw);
    }
    pthread_mutex_unlock(&pUserData->mutw);
#endif
}


void pnc_ops_init(pubnub_t *pn, pubnub_t *pn_sub)
{
    pubnub_mutex_init(m_loop_enabled_mutex);

    InitUserData(&m_user_data, pn);
    InitUserData(&m_user_data_sub, pn_sub);
    
    pubnub_register_callback(pn, ops_callback, &m_user_data);
    pubnub_register_callback(pn_sub, subscribe_callback, &m_user_data_sub);
}


static
#if defined _WIN32
    void
#else
    void* 
#endif
pnc_ops_subscribe_thr(void *pn_sub_addr)
{
    pubnub_t *pn_sub = (pubnub_t *)pn_sub_addr;
    enum pubnub_res res;
    
    char channels_string[PNC_SUBSCRIBE_CHANNELS_LIMIT * PNC_CHANNEL_NAME_SIZE];
    char channel_groups_string[PNC_SUBSCRIBE_CHANNELS_LIMIT * PNC_CHANNEL_NAME_SIZE];

    puts("\nSubscribe thread created");
    pubnub_mutex_lock(m_loop_enabled_mutex);
    m_loop_enabled = true;
    pubnub_mutex_unlock(m_loop_enabled_mutex);

    for (;;) {
        bool loop_enabled;

        pubnub_mutex_lock(m_loop_enabled_mutex);
        loop_enabled = m_loop_enabled;
        pubnub_mutex_unlock(m_loop_enabled_mutex);
        
        if (!loop_enabled) {
            break;
        }

        puts("Subscribe loop...");
        
        pnc_subscribe_list_channels(channels_string, sizeof channels_string);
        pnc_subscribe_list_channel_groups(channel_groups_string, sizeof channel_groups_string);
        
        if (strlen(channels_string) == 0 && strlen(channel_groups_string) == 0) {
            puts("You need add some channels or channel groups first. Ignoring");
            break;
        } 
        else if (strlen(channel_groups_string) == 0) {
            res = pubnub_subscribe(pn_sub, channels_string, NULL);
        } 
        else if (strlen(channels_string) == 0) {
            res = pubnub_subscribe(pn_sub, NULL, channel_groups_string);
        } 
        else {
            res = pubnub_subscribe(pn_sub, channels_string, channel_groups_string);
        }

        if (res != PNR_STARTED) {
            printf("pubnub_subscribe() returned unexpected %d: %s\n", res, pubnub_res_2_string(res));
            break;
        }
        
        await(&m_user_data_sub);
    }
    
#if !defined _WIN32
    return NULL;
#endif
}


void pnc_ops_subscribe(pubnub_t *pn_sub)
{
#if defined _WIN32
    sub_thread = _beginthread(pnc_ops_subscribe_thr, 0, pn_sub);
#else
    pthread_create(&sub_thread, NULL, pnc_ops_subscribe_thr, pn_sub);
#endif
}


void pnc_ops_unsubscribe(pubnub_t *pn_sub)
{
    pubnub_mutex_lock(m_loop_enabled_mutex);
    if (m_loop_enabled) {
        m_loop_enabled = false;
        puts("Subscription loop is disabled and will be stopped after the next message received.");
    } 
    else {
        puts("Subscription loop is already disabled.");
    }
    pubnub_mutex_unlock(m_loop_enabled_mutex);
}


void pnc_ops_parse_response(const char *method_name, enum pubnub_res res, pubnub_t *pn)
{
    struct UserData *pUserData = pubnub_get_user_data(pn);

    if (res != PNR_STARTED) {
        printf("%s returned unexpected %d: %s\n", method_name, res, pubnub_res_2_string(res));
        return;
    }
    
    await(pUserData);
}


void pnc_ops_parse_callback(enum pubnub_trans method_name, enum pubnub_res res, pubnub_t *pn)
{
    if (res == PNR_HTTP_ERROR) {
        printf("%d HTTP error / code: %d\n",
               method_name, pubnub_last_http_code(pn));
        return;
    }
    
    if (res == PNR_STARTED) {
        printf("%d returned unexpected: PNR_STARTED(%d)\n", method_name, res);
        return;
    }
    
    if (PNR_OK == res) {
        const char *msg;
        
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
    } 
    else {
        printf("%d failed, error code %d: %s !\n", method_name, res, pubnub_res_2_string(res));
    }
}
