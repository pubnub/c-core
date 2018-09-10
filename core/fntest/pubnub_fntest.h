/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_FNTEST
#define	INC_PUBNUB_FNTEST

#include <stdbool.h>


enum PNFNTestResult { trFail = -1, trIndeterminate, trPass };

struct PNTestParameters {
    char const* pubkey;
    char const* keysub;
    char const* origin;
    bool        candochangroup;
};


struct pnfntst_timer;
typedef struct pnfntst_timer pnfntst_timer_t;

pnfntst_timer_t *pnfntst_alloc_timer(void);
int pnfntst_start_timer(pnfntst_timer_t *t, unsigned ms);
void pnfntst_stop_timer(pnfntst_timer_t *t);
#define pnfntst_restart_timer(t, ms)                                           \
    pnfntst_stop_timer(t), pnfntst_start_timer(t, ms)
int pnfntst_reset_timer(pnfntst_timer_t *t);
bool pnfntst_timer_is_running(pnfntst_timer_t *t);
void pnfntst_free_timer(void* t);


struct PNTestParameters const* pnfntst_params(void);

int pnfntst_set_params(struct PNTestParameters const* p);

#include "core/pubnub_api_types.h"

/** Returns whether the messages specified as strings in the
    variable-arguments (which must end with NULL), have been received
    on the context @p p.
    
 */
bool pnfntst_got_messages(pubnub_t *p, ...);


/** Returns whether the @p message specified as a string is the next
    in the buffer of received messages in the the context @p p and if
    it was received on the given @p channel (also a string).  Don't
    use this function after a subscribe to a single channel, because
    in that case there is no channel information in the message
    buffer, and it will fail, even if the messages was really received
    from the channel at hand.
 */
bool pnfntst_got_message_on_channel(pubnub_t*   p,
                                    char const* message,
                                    char const* channel);


bool pnfntst_subscribe_and_check(pubnub_t*   p,
                                 char const* chan,
                                 char const* chgroup,
                                 unsigned    ms,
                                 ...);

void pnfntst_free(void* p);


/** Helper function, calls pubnub_alloc() and, if it succeeds, then calls
    pubnub_init() to set the pub/sub keys and then sets the origin, all
    according to test run parameters.
*/
pubnub_t* pnfntst_create_ctx(void);


/** Will use random number generation to "mix" the given string and
    generate a random string to be used as a name in a test.  Mostly
    to be used to generate unique names for channels and channel
    groups, so that concurrent tests don't mess up each other.

    Since random number generators (RNGs) are not fully reliable, we
    put the given string "in the mix", which will usually be the test
    name, making it higly likely that the generated name would be
    different for different tests, even if RNG generates the same
    values.

    The result is allocated by this function and should be free()d
    (unless it is NULL, of course).
 */
char* pnfntst_make_name(char const* s);


#include "fntest/pubnub_fntest_pal.h"

/** Declare a test that needs support for channel groups API (add,
    remove, list: channel groups). IOW, can only be run on an account
    that has this API enabled.
*/
#define TEST_DEF_NEED_CHGROUP(tst)                                             \
    TEST_DEF(tst)                                                              \
    {                                                                          \
        struct PNTestParameters const* ptstp_ = pnfntst_params();              \
        if (!ptstp_->candochangroup) {                                         \
            TEST_INDETERMINATE;                                                \
        }                                                                      \
    }


#endif /* !defined INC_PUBNUB_FNTEST */
