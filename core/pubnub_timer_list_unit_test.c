/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "cgreen/cgreen.h"
#include "cgreen/mocks.h"

#include "pubnub_timer_list.h"
#include "pubnub_timers.h"
#include "pubnub_alloc.h"

#include "pubnub_ccore.h"


#include <stdlib.h>
#include <string.h>
#include <setjmp.h>


/* A less chatty cgreen :) */

#define attest assert_that
#define equals is_equal_to
#define streqs is_equal_to_string
#define differs is_not_equal_to
#define strdifs is_not_equal_to_string
#define ptreqs(val) is_equal_to_contents_of(&(val), sizeof(val))
#define ptrdifs(val) is_not_equal_to_contents_of(&(val), sizeof(val))
#define sets(par, val) will_set_contents_of_parameter(par, &(val), sizeof(val))
#define sets_ex will_set_contents_of_parameter
#define returns will_return


void pbcc_deinit(struct pbcc_context *p)
{
}


Describe(pubnub_timer_list);

pubnub_t *m_list;


BeforeEach(pubnub_timer_list) {
    m_list = NULL;
}


AfterEach(pubnub_timer_list) {
}


Ensure(pubnub_timer_list, enqueue_when_empty) {
    pubnub_t *expired;
    pubnub_t *pbp = pubnub_alloc();

    attest(pbp, differs(NULL));
    pubnub_timer_list_init(pbp);
    attest(pubnub_set_transaction_timeout(pbp, 1000), equals(0));
    m_list = pubnub_timer_list_add(m_list, pbp);
    attest(m_list, equals(pbp));
    expired = pubnub_timer_list_as_time_goes_by(&m_list, 1000);
    attest(m_list, equals(NULL));
    attest(expired, equals(pbp));
    attest(pubnub_timer_list_next(expired), equals(NULL));
    attest(pubnub_timer_list_previous(expired), equals(NULL));

    pubnub_free(pbp);
}

Ensure(pubnub_timer_list, dequeue_when_only_one) {
    pubnub_t *pbp = pubnub_alloc();

    attest(pbp, differs(NULL));
    pubnub_timer_list_init(pbp);
    attest(pubnub_set_transaction_timeout(pbp, 1000), equals(0));
    m_list = pubnub_timer_list_add(m_list, pbp);
    attest(m_list, equals(pbp));

    m_list = pubnub_timer_list_remove(m_list, pbp);
    attest(m_list, equals(NULL));
    attest(pubnub_timer_list_next(pbp), equals(NULL));
    attest(pubnub_timer_list_previous(pbp), equals(NULL));

    pubnub_free(pbp);
}



Ensure(pubnub_timer_list, enqueue_one_after_one) {
    pubnub_t *expired;
    pubnub_t *pbp = pubnub_alloc();
    pubnub_t *pbp_two = pubnub_alloc();

    attest(pbp, differs(NULL));
    pubnub_timer_list_init(pbp);
    attest(pubnub_set_transaction_timeout(pbp, 1000), equals(0));
    m_list = pubnub_timer_list_add(m_list, pbp);
    attest(m_list, equals(pbp));

    attest(pbp_two, differs(NULL));
    pubnub_timer_list_init(pbp_two);
    attest(pubnub_set_transaction_timeout(pbp_two, 2000), equals(0));
    m_list = pubnub_timer_list_add(m_list, pbp_two);
    attest(m_list, equals(pbp));

    expired = pubnub_timer_list_as_time_goes_by(&m_list, 2000);
    attest(expired, equals(pbp));
    attest(m_list, equals(NULL));
    attest(pubnub_timer_list_next(expired), equals(pbp_two));
    attest(pubnub_timer_list_previous(expired), equals(NULL));
    attest(pubnub_timer_list_next(pbp_two), equals(NULL));
    attest(pubnub_timer_list_previous(pbp_two), equals(pbp));

    pubnub_free(pbp_two);
    pubnub_free(pbp);
}


Ensure(pubnub_timer_list, enqueue_one_before_one) {
    pubnub_t *expired;
    pubnub_t *pbp = pubnub_alloc();
    pubnub_t *pbp_two = pubnub_alloc();

    attest(pbp, differs(NULL));
    pubnub_timer_list_init(pbp);
    attest(pubnub_set_transaction_timeout(pbp, 1000), equals(0));
    m_list = pubnub_timer_list_add(m_list, pbp);
    attest(m_list, equals(pbp));

    attest(pbp_two, differs(NULL));
    pubnub_timer_list_init(pbp_two);
    attest(pubnub_set_transaction_timeout(pbp_two, 200), equals(0));
    m_list = pubnub_timer_list_add(m_list, pbp_two);
    attest(m_list, equals(pbp_two));

    expired = pubnub_timer_list_as_time_goes_by(&m_list, 1000);
    attest(expired, equals(pbp_two));
    attest(m_list, equals(NULL));
    attest(pubnub_timer_list_next(expired), equals(pbp));
    attest(pubnub_timer_list_previous(expired), equals(NULL));
    attest(pubnub_timer_list_next(pbp), equals(NULL));
    attest(pubnub_timer_list_previous(pbp), equals(pbp_two));

    pubnub_free(pbp_two);
    pubnub_free(pbp);
}


Ensure(pubnub_timer_list, enqueue_in_the_middle) {
    pubnub_t *expired;
    pubnub_t *pbp = pubnub_alloc();
    pubnub_t *pbp_two = pubnub_alloc();
    pubnub_t *pbp_three = pubnub_alloc();

    attest(pbp, differs(NULL));
    pubnub_timer_list_init(pbp);
    attest(pubnub_set_transaction_timeout(pbp, 1000), equals(0));
    m_list = pubnub_timer_list_add(m_list, pbp);
    attest(m_list, equals(pbp));

    attest(pbp_two, differs(NULL));
    pubnub_timer_list_init(pbp_two);
    attest(pubnub_set_transaction_timeout(pbp_two, 200), equals(0));
    m_list = pubnub_timer_list_add(m_list, pbp_two);
    attest(m_list, equals(pbp_two));

    attest(pbp_three, differs(NULL));
    pubnub_timer_list_init(pbp_three);
    attest(pubnub_set_transaction_timeout(pbp_three, 500), equals(0));
    m_list = pubnub_timer_list_add(m_list, pbp_three);
    attest(m_list, equals(pbp_two));

    expired = pubnub_timer_list_as_time_goes_by(&m_list, 1000);
    attest(expired, equals(pbp_two));
    attest(m_list, equals(NULL));
    attest(pubnub_timer_list_next(expired), equals(pbp_three));
    attest(pubnub_timer_list_previous(expired), equals(NULL));
    attest(pubnub_timer_list_next(pbp_three), equals(pbp));
    attest(pubnub_timer_list_previous(pbp_three), equals(pbp_two));
    attest(pubnub_timer_list_next(pbp), equals(NULL));
    attest(pubnub_timer_list_previous(pbp), equals(pbp_three));

    pubnub_free(pbp_three);
    pubnub_free(pbp_two);
    pubnub_free(pbp);
}


Ensure(pubnub_timer_list, dequeue_last) {
    pubnub_t *pbp = pubnub_alloc();
    pubnub_t *pbp_two = pubnub_alloc();

    attest(pbp, differs(NULL));
    pubnub_timer_list_init(pbp);
    attest(pubnub_set_transaction_timeout(pbp, 1000), equals(0));
    m_list = pubnub_timer_list_add(m_list, pbp);
    attest(m_list, equals(pbp));

    attest(pbp_two, differs(NULL));
    pubnub_timer_list_init(pbp_two);
    attest(pubnub_set_transaction_timeout(pbp_two, 2000), equals(0));
    m_list = pubnub_timer_list_add(m_list, pbp_two);
    attest(m_list, equals(pbp));

    m_list = pubnub_timer_list_remove(m_list, pbp_two);
    attest(m_list, equals(pbp));
    attest(pubnub_timer_list_next(pbp), equals(NULL));
    attest(pubnub_timer_list_previous(pbp), equals(NULL));
    attest(pubnub_timer_list_next(pbp_two), equals(NULL));
    attest(pubnub_timer_list_previous(pbp_two), equals(NULL));

    pubnub_free(pbp_two);
    pubnub_free(pbp);
}


Ensure(pubnub_timer_list, dequeue_first) {
    pubnub_t *pbp = pubnub_alloc();
    pubnub_t *pbp_two = pubnub_alloc();

    attest(pbp, differs(NULL));
    pubnub_timer_list_init(pbp);
    attest(pubnub_set_transaction_timeout(pbp, 1000), equals(0));
    m_list = pubnub_timer_list_add(m_list, pbp);
    attest(m_list, equals(pbp));

    attest(pbp_two, differs(NULL));
    pubnub_timer_list_init(pbp_two);
    attest(pubnub_set_transaction_timeout(pbp_two, 2000), equals(0));
    m_list = pubnub_timer_list_add(m_list, pbp_two);
    attest(m_list, equals(pbp));

    m_list = pubnub_timer_list_remove(m_list, pbp);
    attest(m_list, equals(pbp_two));
    attest(pubnub_timer_list_next(pbp), equals(NULL));
    attest(pubnub_timer_list_previous(pbp), equals(NULL));
    attest(pubnub_timer_list_next(pbp_two), equals(NULL));
    attest(pubnub_timer_list_previous(pbp_two), equals(NULL));

    pubnub_free(pbp_two);
    pubnub_free(pbp);
}


Ensure(pubnub_timer_list, dequeue_from_the_middle) {
    pubnub_t *pbp = pubnub_alloc();
    pubnub_t *pbp_two = pubnub_alloc();
    pubnub_t *pbp_three = pubnub_alloc();

    attest(pbp, differs(NULL));
    pubnub_timer_list_init(pbp);
    attest(pubnub_set_transaction_timeout(pbp, 1000), equals(0));
    m_list = pubnub_timer_list_add(m_list, pbp);
    attest(m_list, equals(pbp));

    attest(pbp_two, differs(NULL));
    pubnub_timer_list_init(pbp_two);
    attest(pubnub_set_transaction_timeout(pbp_two, 200), equals(0));
    m_list = pubnub_timer_list_add(m_list, pbp_two);
    attest(m_list, equals(pbp_two));

    attest(pbp_three, differs(NULL));
    pubnub_timer_list_init(pbp_three);
    attest(pubnub_set_transaction_timeout(pbp_three, 500), equals(0));
    m_list = pubnub_timer_list_add(m_list, pbp_three);
    attest(m_list, equals(pbp_two));

    m_list = pubnub_timer_list_remove(m_list, pbp_three);
    attest(m_list, equals(pbp_two));
    attest(pubnub_timer_list_next(pbp), equals(NULL));
    attest(pubnub_timer_list_previous(pbp), equals(pbp_two));
    attest(pubnub_timer_list_next(pbp_two), equals(pbp));
    attest(pubnub_timer_list_previous(pbp_two), equals(NULL));
    attest(pubnub_timer_list_next(pbp_three), equals(NULL));
    attest(pubnub_timer_list_previous(pbp_three), equals(NULL));

    pubnub_free(pbp_three);
    pubnub_free(pbp_two);
    pubnub_free(pbp);
}


Ensure(pubnub_timer_list, little_time_passed) {
    pubnub_t *pbp = pubnub_alloc();
    pubnub_t *pbp_two = pubnub_alloc();

    attest(pbp, differs(NULL));
    pubnub_timer_list_init(pbp);
    attest(pubnub_set_transaction_timeout(pbp, 1000), equals(0));
    m_list = pubnub_timer_list_add(m_list, pbp);
    attest(m_list, equals(pbp));

    attest(pbp_two, differs(NULL));
    pubnub_timer_list_init(pbp_two);
    attest(pubnub_set_transaction_timeout(pbp_two, 2000), equals(0));
    m_list = pubnub_timer_list_add(m_list, pbp_two);
    attest(m_list, equals(pbp));

    attest(pubnub_timer_list_as_time_goes_by(&m_list, 10), equals(NULL));
    attest(m_list, equals(pbp));

    pubnub_free(pbp_two);
    pubnub_free(pbp);
}


Ensure(pubnub_timer_list, expire_first) {
    pubnub_t *expired;
    pubnub_t *pbp = pubnub_alloc();
    pubnub_t *pbp_two = pubnub_alloc();

    attest(pbp, differs(NULL));
    pubnub_timer_list_init(pbp);
    attest(pubnub_set_transaction_timeout(pbp, 1000), equals(0));
    m_list = pubnub_timer_list_add(m_list, pbp);
    attest(m_list, equals(pbp));

    attest(pbp_two, differs(NULL));
    pubnub_timer_list_init(pbp_two);
    attest(pubnub_set_transaction_timeout(pbp_two, 2000), equals(0));
    m_list = pubnub_timer_list_add(m_list, pbp_two);
    attest(m_list, equals(pbp));

    expired = pubnub_timer_list_as_time_goes_by(&m_list, 1100);
    attest(expired, equals(pbp));
    attest(m_list, equals(pbp_two));
    attest(pubnub_timer_list_next(expired), equals(NULL));
    attest(pubnub_timer_list_previous(expired), equals(NULL));
    attest(pubnub_timer_list_next(pbp_two), equals(NULL));
    attest(pubnub_timer_list_previous(pbp_two), equals(NULL));

    pubnub_free(pbp_two);
    pubnub_free(pbp);
}


Ensure(pubnub_timer_list, expire_when_empty) {
    pubnub_t *list = NULL;
    attest(NULL == pubnub_timer_list_as_time_goes_by(&list, 1));
}


Ensure(pubnub_timer_list, expire_in_the_middle) {
    pubnub_t *expired;
    pubnub_t *pbp = pubnub_alloc();
    pubnub_t *pbp_two = pubnub_alloc();
    pubnub_t *pbp_three = pubnub_alloc();

    attest(pbp, differs(NULL));
    pubnub_timer_list_init(pbp);
    attest(pubnub_set_transaction_timeout(pbp, 1000), equals(0));
    m_list = pubnub_timer_list_add(m_list, pbp);
    attest(m_list, equals(pbp));

    attest(pbp_two, differs(NULL));
    pubnub_timer_list_init(pbp_two);
    attest(pubnub_set_transaction_timeout(pbp_two, 2000), equals(0));
    m_list = pubnub_timer_list_add(m_list, pbp_two);
    attest(m_list, equals(pbp));

    attest(pbp_three, differs(NULL));
    pubnub_timer_list_init(pbp_three);
    attest(pubnub_set_transaction_timeout(pbp_three, 3000), equals(0));
    m_list = pubnub_timer_list_add(m_list, pbp_three);
    attest(m_list, equals(pbp));

    expired = pubnub_timer_list_as_time_goes_by(&m_list, 2200);
    attest(expired, equals(pbp));
    attest(m_list, equals(pbp_three));
    attest(pubnub_timer_list_next(expired), equals(pbp_two));
    attest(pubnub_timer_list_previous(expired), equals(NULL));
    attest(pubnub_timer_list_next(pbp_two), equals(NULL));
    attest(pubnub_timer_list_previous(pbp_two), equals(pbp));
    attest(pubnub_timer_list_next(pbp_three), equals(NULL));
    attest(pubnub_timer_list_previous(pbp_three), equals(NULL));

    pubnub_free(pbp_three);
    pubnub_free(pbp_two);
    pubnub_free(pbp);
}
