/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_FNTEST
#define	INC_PUBNUB_FNTEST

#include <stdbool.h>


enum PNFNTestResult {
    trFail = -1,
    trIndeterminate,
    trPass
};

extern char const*  g_pubkey;
extern char const*  g_keysub;
extern char const*  g_origin;


struct pnfntst_timer;
typedef struct pnfntst_timer pnfntst_timer_t;

pnfntst_timer_t *pnfntst_alloc_timer(void);
int pnfntst_start_timer(pnfntst_timer_t *t, unsigned ms);
void pnfntst_stop_timer(pnfntst_timer_t *t);
#define pnfntst_restart_timer(t, ms) pnfntst_stop_timer(t), pnfntst_start_timer(t, ms)
int pnfntst_reset_timer(pnfntst_timer_t *t);
bool pnfntst_timer_is_running(pnfntst_timer_t *t);
void pnfntst_free_timer(pnfntst_timer_t *t);


#include "pubnub_api_types.h"

/** Returns whether the messages specified as strings in the
    variable-arguments (which must end with NULL), have been received
    on the context @p p.
    
 */
bool pnfntst_got_messages(pubnub_t *p, ...);


/** Returns whether the @p message specified as a string is the
    next in the buffer of received messages in the the context @p p
    and if it was received on the given @p channel (also a string).
    Don't use this function after a subscribe to a single channel,
    because in that case there is no channel information in the
    message buffer, and it will fail, even if the messages was
    really received from the channel at hand.
    
 */
bool pnfntst_got_message_on_channel(pubnub_t *p, char const *message, char const *channel);


bool pnfntst_subscribe_and_check(pubnub_t *p, char const *chan, char const*chgroup, unsigned ms, ...);

#include "pubnub_fntest_pal.h"

#endif /* !defined INC_PUBNUB_FNTEST */
