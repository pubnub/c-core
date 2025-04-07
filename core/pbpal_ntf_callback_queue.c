/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"

#include "pbpal_ntf_callback_queue.h"

#include "pubnub_assert.h"
#include <stdio.h>

#include <emscripten.h>


void pbpal_ntf_callback_queue_init(struct pbpal_ntf_callback_queue* queue)
{
    pubnub_mutex_init(queue->monitor);
    queue->size = sizeof queue->apb / sizeof queue->apb[0];
    queue->head = queue->tail = 0;
}


void pbpal_ntf_callback_queue_deinit(struct pbpal_ntf_callback_queue* queue)
{
    pubnub_mutex_destroy(queue->monitor);
    queue->head = queue->tail = 0;
}


int pbpal_ntf_callback_enqueue_for_processing(struct pbpal_ntf_callback_queue* queue,
                                              pubnub_t* pb)
{
    int    result;
    size_t next_head;

    PUBNUB_ASSERT_OPT(queue != NULL);
    PUBNUB_ASSERT_OPT(pb != NULL);

    pubnub_mutex_lock(queue->monitor);
    next_head = queue->head + 1;
    if (next_head == queue->size) {
        next_head = 0;
    }
    if (next_head != queue->tail) {
        queue->apb[queue->head] = pb;
        queue->head             = next_head;
        result                  = +1;
    }
    else {
        result = -1;
    }
    pubnub_mutex_unlock(queue->monitor);

    return result;
}


int pbpal_ntf_callback_requeue_for_processing(struct pbpal_ntf_callback_queue* queue,
                                              pubnub_t* pb)
{
    bool   found = false;
    size_t i;

    PUBNUB_ASSERT_OPT(pb != NULL);

    pubnub_mutex_lock(queue->monitor);
    for (i = queue->tail; i != queue->head;
         i = (((i + 1) == queue->size) ? 0 : i + 1)) {
        if (queue->apb[i] == pb) {
            found = true;
            break;
        }
    }
    pubnub_mutex_unlock(queue->monitor);

    return !found ? pbpal_ntf_callback_enqueue_for_processing(queue, pb) : 0;
}


void pbpal_ntf_callback_remove_from_queue(struct pbpal_ntf_callback_queue* queue,
                                          pubnub_t*                        pb)
{
    size_t i;

    PUBNUB_ASSERT_OPT(queue != NULL);
    PUBNUB_ASSERT_OPT(pb != NULL);

    pubnub_mutex_lock(queue->monitor);
    for (i = queue->tail; i != queue->head;
         i = (((i + 1) == queue->size) ? 0 : i + 1)) {
        if (queue->apb[i] == pb) {
            queue->apb[i] = NULL;
            break;
        }
    }
    pubnub_mutex_unlock(queue->monitor);
}

EM_ASYNC_JS(void, send_fetch_request_cb, (const char* url, const char* method, const char* headers, const char* body, int timeout), {
    let timeoutId;
    const controller = new AbortController();
    
    console.log('send_fetch_request');
    console.log(UTF8ToString(url));
    console.log(UTF8ToString(method));
    console.log(UTF8ToString(headers));
    console.log(UTF8ToString(body));
   
    const requestTimeout = new Promise((_, reject) => {
        timeoutId = setTimeout(() => {
            clearTimeout(timeoutId);
            reject(new Error('Request timeout')); 
            controller.abort('Cancel because of timeout');
        }, timeout * 1000);
    });

    const request = new Request(UTF8ToString(url), {
        method: UTF8ToString(method),
        headers: JSON.parse(UTF8ToString(headers)),
        redirect: 'follow',
        body: UTF8ToString(body)
    });

    await Promise.race([
        fetch(request, {
            signal: controller.signal,
            credentials: 'omit',
            cache: 'no-cache'
        }).then((response) => {
            if (timeoutId) clearTimeout(timeoutId);
            return response;
        }),
        requestTimeout
    ]).then(response => {
        // Handle response
        console.log('Fetch completed:', response);
    }).catch(error => {
        console.error('Fetch error:', error);
    });
});

void pbpal_ntf_callback_process_queue(struct pbpal_ntf_callback_queue* queue)
{
    pubnub_mutex_lock(queue->monitor);
    while (queue->head != queue->tail) {
        pubnub_t* pbp = queue->apb[queue->tail++];
        if (queue->tail == queue->size) {
            queue->tail = 0;
        }
        if (pbp != NULL) {
            pubnub_mutex_unlock(queue->monitor);
            pubnub_mutex_lock(pbp->monitor);
            if (pbp->state == PBS_NULL) {
                pubnub_mutex_unlock(pbp->monitor);
                pballoc_free_at_last(pbp);
            }
            else {
                // Get URL, method, headers and body from pb context
                const char* url = strcat("http://", strcat(pbp->origin, pbp->core.http_buf));
                const char* headers = "{}";

                int i = 0;
                for (;;) {
                    if (pbp->core.http_buf[i] == '{'  || i > 16000) {
                        break;
                    }
                    i++;
                }
                const char* method = i > 16000 ? "GET" : "POST";

                const char* body = pbp->core.http_buf + i;

                printf("url: %s\n", url);
                printf("body: %s\n", body);

                // Call the send_fetch_request function with the parameters
                send_fetch_request_cb(url, method, headers, body, 10000);
                
                pbp->state = PBS_IDLE;
                pbp->core.http_buf_len = 0;
                pbp->core.http_reply = "";
                pbp->core.http_content_len = 0;
                pbp->core.http_buf_len = 0;
                pbp->core.http_reply = "";
                pbp->core.http_content_len = 0;
                pbp->core.http_buf_len = 0;

                //pbnc_fsm(pbp);
                pubnub_mutex_unlock(pbp->monitor);
            }
            pubnub_mutex_lock(queue->monitor);
        }
    }
    pubnub_mutex_unlock(queue->monitor);
}
