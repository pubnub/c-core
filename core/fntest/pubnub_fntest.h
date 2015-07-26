/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_FNTEST
#define	INC_PUBNUB_FNTEST

#include <stdbool.h>


enum PNFNTestResult {
    trFail = -1,
    trIndeterminate,
    trPass
};

struct pnfntst_timer;
typedef struct pnfntst_timer pnfntst_timer_t;

pnfntst_timer_t *pnfntst_alloc_timer(void);
int pnfntst_start_timer(pnfntst_timer_t *t, unsigned ms);
void pnfntst_stop_timer(pnfntst_timer_t *t);
#define pnfntst_restart_timer(t, ms) pnfntst_stop_timer(t), pnfntst_start_timer(t, ms)
int pnfntst_reset_timer(pnfntst_timer_t *t);
bool pnfntst_timer_is_running(pnfntst_timer_t *t);
void pnfntst_free_timer(pnfntst_timer_t *t);


struct pubnub;
typedef struct pubnub pubnub_t;

/** Returns whether the messages specified as strings in the
    variable-arguments (which must end with NULL), have been received
    on the context @p p.
    
 */
bool pnfntst_got_messages(pubnub_t *p, ...);


#include "pubnub_fntest_pal.h"

#endif /* !defined INC_PUBNUB_FNTEST */
