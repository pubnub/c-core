/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cgreen/include/cgreen/constraint_syntax_helpers.h"
#include "cgreen/include/cgreen/constraint.h"
#include "cgreen/include/cgreen/assertions.h"
#include "cgreen/include/cgreen/filename.h"
#include "cgreen/include/cgreen/cgreen.h"

#include "lib/pbarray.h"


// ----------------------------------
//              Statics
// ----------------------------------

static int        elementFreeCounter = 0;
static pbarray_t* array              = NULL;


// ----------------------------------
//              Helpers
// ----------------------------------

void freeElement(void* __unused element)
{
    elementFreeCounter++;
}


// ----------------------------------
//            Tests setup
// ----------------------------------

Describe(pbarray);

BeforeEach(pbarray)
{
    array              = NULL;
    elementFreeCounter = 0;
}

AfterEach(pbarray)
{
    if (NULL != array) { pbarray_free(array); }
}


// ----------------------------------
//              Tests
// ----------------------------------

Ensure(pbarray, should_allocate)
{
    array = pbarray_alloc(2,
                          PBARRAY_RESIZE_NONE,
                          PBARRAY_GENERIC_CONTENT_TYPE,
                          NULL);

    assert_that(pbarray_count(array), is_equal_to(0));
    assert_that(array, is_non_null);
}

Ensure(pbarray, should_contain_string_element)
{
    array = pbarray_alloc(2,
                          PBARRAY_RESIZE_BALANCED,
                          PBARRAY_CHAR_CONTENT_TYPE,
                          NULL);
    pbarray_add(array, "hello once");
    pbarray_add(array, "hello twice");
    pbarray_add(array, "hello again");

    assert_that(pbarray_contains(array, "hello twice"), is_true);
    assert_that(pbarray_contains(array, "hello impostor"), is_false);
}

Ensure(pbarray, should_contain_data)
{
    array = pbarray_alloc(2,
                          PBARRAY_RESIZE_BALANCED,
                          PBARRAY_GENERIC_CONTENT_TYPE,
                          NULL);
    char* once = malloc((strlen("hello once") + 1) * sizeof(char));
    strcpy(once, "hello once");
    char* twice = malloc((strlen("hello twice") + 1) * sizeof(char));
    strcpy(twice, "hello twice");
    pbarray_add(array, once);
    pbarray_add(array, twice);
    pbarray_add(array, "hello again");

    assert_that(pbarray_contains(array, "hello once"), is_false);
    assert_that(pbarray_contains(array, twice), is_true);

    free(once);
    free(twice);
}

Ensure(pbarray, should_return_elements)
{
    array = pbarray_alloc(2,
                          PBARRAY_RESIZE_BALANCED,
                          PBARRAY_GENERIC_CONTENT_TYPE,
                          NULL);
    pbarray_add(array, "hello once");
    pbarray_add(array, "hello twice");


    size_t count;
    const void** elements = pbarray_elements(array, &count);
    assert_that(count, is_equal_to(2));
    assert_that(elements[0], is_equal_to_string("hello once"));
    assert_that(elements[1], is_equal_to_string("hello twice"));
}

Ensure(pbarray, should_not_return_elements_when_empty)
{
    array = pbarray_alloc(2,
                          PBARRAY_RESIZE_BALANCED,
                          PBARRAY_GENERIC_CONTENT_TYPE,
                          NULL);


    size_t       count;
    const void** elements = pbarray_elements(array, &count);
    assert_that(count, is_equal_to(0));
    assert_that(elements, is_not_null);
    free(elements);
}

Ensure(pbarray, should_add_element)
{
    array = pbarray_alloc(1,
                          PBARRAY_RESIZE_NONE,
                          PBARRAY_GENERIC_CONTENT_TYPE,
                          NULL);

    assert_that(pbarray_add(array, "hello"), is_equal_to(PBAR_OK));
    assert_that(pbarray_count(array), is_equal_to(1));
}

Ensure(pbarray, should_add_duplicate_element)
{
    array = pbarray_alloc(2,
                          PBARRAY_RESIZE_NONE,
                          PBARRAY_GENERIC_CONTENT_TYPE,
                          NULL);
    pbarray_add(array, "hello");
    pbarray_add(array, "hello");

    assert_that(pbarray_count(array), is_equal_to(2));
    assert_that(pbarray_first(array), is_equal_to(pbarray_last(array)));
}

Ensure(pbarray, should_resize_when_add_two_elements)
{
    array = pbarray_alloc(1,
                          PBARRAY_RESIZE_BALANCED,
                          PBARRAY_GENERIC_CONTENT_TYPE,
                          NULL);

    assert_that(pbarray_add(array, "hello once"), is_equal_to(PBAR_OK));
    assert_that(pbarray_add(array, "hello twice"), is_equal_to(PBAR_OK));
    assert_that(pbarray_count(array), is_equal_to(2));
}

Ensure(pbarray, should_not_resize_when_add_two_elements_and_resize_policy_NONE)
{
    array = pbarray_alloc(1,
                          PBARRAY_RESIZE_NONE,
                          PBARRAY_GENERIC_CONTENT_TYPE,
                          NULL);
    pbarray_add(array, "hello once");

    assert_that(pbarray_add(array, "hello twice"),
                is_equal_to(PBAR_FIXED_SIZE));
    assert_that(pbarray_count(array), is_equal_to(1));
}

Ensure(pbarray, should_insert_element_at_index_when_index_in_range)
{
    array = pbarray_alloc(4,
                          PBARRAY_RESIZE_NONE,
                          PBARRAY_GENERIC_CONTENT_TYPE,
                          NULL);
    pbarray_add(array, "hello once");
    pbarray_add(array, "hello twice");
    pbarray_add(array, "hello again");

    assert_that(pbarray_insert(array, "hello impostor", 1),
                is_equal_to(PBAR_OK));
    assert_that(pbarray_count(array), is_equal_to(4));
    assert_that(pbarray_element_at(array, 0), is_equal_to_string("hello once"));
    assert_that(pbarray_element_at(array, 1),
                is_equal_to_string("hello impostor"));
    assert_that(pbarray_element_at(array, 2),
                is_equal_to_string("hello twice"));
}

Ensure(pbarray, should_insert_element_at_index_when_index_equal_length)
{
    array = pbarray_alloc(2,
                          PBARRAY_RESIZE_BALANCED,
                          PBARRAY_GENERIC_CONTENT_TYPE,
                          NULL);
    pbarray_add(array, "hello once");
    pbarray_add(array, "hello twice");

    assert_that(pbarray_insert(array, "hello again", pbarray_count(array)),
                is_equal_to(PBAR_OK));
    assert_that(pbarray_count(array), is_equal_to(3));
    assert_that(pbarray_element_at(array, 2),
                is_equal_to_string("hello again"));
}

Ensure(pbarray, should_not_insert_element_at_index_when_index_out_of_range)
{
    array = pbarray_alloc(2,
                          PBARRAY_RESIZE_BALANCED,
                          PBARRAY_GENERIC_CONTENT_TYPE,
                          NULL);
    pbarray_add(array, "hello once");
    pbarray_add(array, "hello twice");

    assert_that(pbarray_insert(array, "hello again", 4),
                is_equal_to(PBAR_INDEX_OUT_OF_RANGE));
    assert_that(pbarray_count(array), is_equal_to(2));
}

Ensure(pbarray, should_remove_first_string_occurrence)
{
    array = pbarray_alloc(2,
                          PBARRAY_RESIZE_BALANCED,
                          PBARRAY_CHAR_CONTENT_TYPE,
                          NULL);
    pbarray_add(array, "hello once");
    pbarray_add(array, "hello twice");
    pbarray_add(array, "hello once");

    assert_that(pbarray_remove(array, "hello once", false),
                is_equal_to(PBAR_OK));
    assert_that(pbarray_count(array), is_equal_to(2));
    assert_that(pbarray_element_at(array, 0),
                is_equal_to_string("hello twice"));
    assert_that(pbarray_element_at(array, 1), is_equal_to_string("hello once"));
}

Ensure(pbarray, should_remove_all_string_occurrences)
{
    array = pbarray_alloc(2,
                          PBARRAY_RESIZE_BALANCED,
                          PBARRAY_CHAR_CONTENT_TYPE,
                          NULL);
    pbarray_add(array, "hello once");
    pbarray_add(array, "hello twice");
    pbarray_add(array, "hello once");

    assert_that(pbarray_remove(array, "hello once", true),
                is_equal_to(PBAR_OK));
    assert_that(pbarray_count(array), is_equal_to(1));
    assert_that(pbarray_element_at(array, 0),
                is_equal_to_string("hello twice"));
}

Ensure(pbarray, should_not_remove_when_string_not_found)
{
    array = pbarray_alloc(2,
                          PBARRAY_RESIZE_BALANCED,
                          PBARRAY_CHAR_CONTENT_TYPE,
                          freeElement);
    pbarray_add(array, "hello once");
    pbarray_add(array, "hello twice");

    assert_that(pbarray_remove(array, "hello impostor", true),
                is_equal_to(PBAR_NOT_FOUND));
    assert_that(pbarray_count(array), is_equal_to(2));
    assert_that(elementFreeCounter, is_equal_to(0));
}

Ensure(pbarray, should_not_remove_when_data_NULL)
{
    array = pbarray_alloc(2,
                          PBARRAY_RESIZE_BALANCED,
                          PBARRAY_GENERIC_CONTENT_TYPE,
                          freeElement);
    pbarray_add(array, "hello once");
    pbarray_add(array, "hello twice");

    assert_that(pbarray_remove(array, NULL, true), is_equal_to(PBAR_NOT_FOUND));
    assert_that(pbarray_count(array), is_equal_to(2));
    assert_that(elementFreeCounter, is_equal_to(0));
}

Ensure(pbarray, should_remove_first_data_occurrence)
{
    array = pbarray_alloc(2,
                          PBARRAY_RESIZE_BALANCED,
                          PBARRAY_GENERIC_CONTENT_TYPE,
                          NULL);
    pbarray_add(array, "hello once");
    pbarray_add(array, "hello twice");
    pbarray_add(array, "hello once");

    assert_that(pbarray_remove(array, "hello once", false),
                is_equal_to(PBAR_OK));
    assert_that(pbarray_count(array), is_equal_to(2));
    assert_that(pbarray_element_at(array, 0),
                is_equal_to_string("hello twice"));
    assert_that(pbarray_element_at(array, 1), is_equal_to_string("hello once"));
}

Ensure(pbarray, should_remove_all_data_occurrences)
{
    array = pbarray_alloc(2,
                          PBARRAY_RESIZE_BALANCED,
                          PBARRAY_GENERIC_CONTENT_TYPE,
                          NULL);
    char* once = malloc((strlen("hello once") + 1) * sizeof(char));
    strcpy(once, "hello once");
    pbarray_add(array, "hello once");
    pbarray_add(array, "hello twice");
    pbarray_add(array, "hello once");
    pbarray_add(array, once);

    assert_that(pbarray_remove(array, "hello once", true),
                is_equal_to(PBAR_OK));
    assert_that(pbarray_count(array), is_equal_to(2));
    assert_that(pbarray_element_at(array, 0),
                is_equal_to_string("hello twice"));
    assert_that(pbarray_element_at(array, 1), is_equal_to_string("hello once"));

    free(once);
}

Ensure(pbarray, should_not_remove_when_data_not_found)
{
    array = pbarray_alloc(2,
                          PBARRAY_RESIZE_BALANCED,
                          PBARRAY_GENERIC_CONTENT_TYPE,
                          freeElement);
    char* once = malloc((strlen("hello once") + 1) * sizeof(char));
    strcpy(once, "hello once");
    pbarray_add(array, "hello once");
    pbarray_add(array, "hello twice");

    assert_that(pbarray_remove(array, once, true), is_equal_to(PBAR_NOT_FOUND));
    assert_that(pbarray_count(array), is_equal_to(2));
    assert_that(elementFreeCounter, is_equal_to(0));

    free(once);
}

Ensure(pbarray, should_return_element_at_index)
{
    array = pbarray_alloc(3,
                          PBARRAY_RESIZE_BALANCED,
                          PBARRAY_GENERIC_CONTENT_TYPE,
                          NULL);
    pbarray_add(array, "hello once");
    pbarray_add(array, "hello twice");
    pbarray_add(array, "hello again");

    assert_that(pbarray_element_at(array, 1),
                is_equal_to_string("hello twice"));
    assert_that(pbarray_element_at(array, 2),
                is_not_equal_to_string("hello once"));
}

Ensure(pbarray, should_not_return_element_at_index_when_empty)
{
    array = pbarray_alloc(3,
                          PBARRAY_RESIZE_BALANCED,
                          PBARRAY_GENERIC_CONTENT_TYPE,
                          NULL);

    assert_that(pbarray_element_at(array, 0), is_null);
}

Ensure(pbarray, should_not_return_element_at_index_when_out_of_range)
{
    array = pbarray_alloc(3,
                          PBARRAY_RESIZE_BALANCED,
                          PBARRAY_GENERIC_CONTENT_TYPE,
                          NULL);
    pbarray_add(array, "hello once");

    assert_that(pbarray_element_at(array, 1), is_null);
}

Ensure(pbarray, should_return_first_element)
{
    array = pbarray_alloc(3,
                          PBARRAY_RESIZE_BALANCED,
                          PBARRAY_GENERIC_CONTENT_TYPE,
                          NULL);
    pbarray_add(array, "hello once");
    pbarray_add(array, "hello twice");
    pbarray_add(array, "hello again");

    assert_that(pbarray_first(array), is_equal_to_string("hello once"));
}

Ensure(pbarray, should_not_return_first_element_when_empty)
{
    array = pbarray_alloc(3,
                          PBARRAY_RESIZE_BALANCED,
                          PBARRAY_GENERIC_CONTENT_TYPE,
                          NULL);

    assert_that(pbarray_first(array), is_null);
}

Ensure(pbarray, should_return_last_element)
{
    array = pbarray_alloc(3,
                          PBARRAY_RESIZE_BALANCED,
                          PBARRAY_GENERIC_CONTENT_TYPE,
                          NULL);
    pbarray_add(array, "hello once");
    pbarray_add(array, "hello twice");
    pbarray_add(array, "hello again");

    assert_that(pbarray_last(array), is_equal_to_string("hello again"));
}

Ensure(pbarray, should_not_return_last_element_when_empty)
{
    array = pbarray_alloc(3,
                          PBARRAY_RESIZE_BALANCED,
                          PBARRAY_GENERIC_CONTENT_TYPE,
                          NULL);

    assert_that(pbarray_last(array), is_null);
}

Ensure(pbarray, should_remove_and_return_first_element)
{
    array = pbarray_alloc(3,
                          PBARRAY_RESIZE_BALANCED,
                          PBARRAY_GENERIC_CONTENT_TYPE,
                          freeElement);
    pbarray_add(array, "hello once");
    pbarray_add(array, "hello twice");
    pbarray_add(array, "hello again");

    assert_that(pbarray_pop_first(array), is_equal_to_string("hello once"));
    assert_that(pbarray_count(array), is_equal_to(2));
    // We return element to the called and it responsible for resources free.
    assert_that(elementFreeCounter, is_equal_to(0));
}

Ensure(pbarray, should_not_remove_and_return_first_element_when_empty)
{
    array = pbarray_alloc(3,
                          PBARRAY_RESIZE_BALANCED,
                          PBARRAY_GENERIC_CONTENT_TYPE,
                          freeElement);

    assert_that(pbarray_pop_first(array), is_null);
    assert_that(elementFreeCounter, is_equal_to(0));
}

Ensure(pbarray, should_remove_and_return_last_element)
{
    array = pbarray_alloc(3,
                          PBARRAY_RESIZE_BALANCED,
                          PBARRAY_GENERIC_CONTENT_TYPE,
                          freeElement);
    pbarray_add(array, "hello once");
    pbarray_add(array, "hello twice");
    pbarray_add(array, "hello again");

    assert_that(pbarray_pop_last(array), is_equal_to_string("hello again"));
    assert_that(pbarray_count(array), is_equal_to(2));
    // We return element to the called and it responsible for resources free.
    assert_that(elementFreeCounter, is_equal_to(0));
}

Ensure(pbarray, should_not_remove_and_return_last_element_when_empty)
{
    array = pbarray_alloc(3,
                          PBARRAY_RESIZE_BALANCED,
                          PBARRAY_GENERIC_CONTENT_TYPE,
                          freeElement);

    assert_that(pbarray_pop_last(array), is_null);
    assert_that(elementFreeCounter, is_equal_to(0));
}

Ensure(pbarray, should_free)
{
    array = pbarray_alloc(3,
                          PBARRAY_RESIZE_BALANCED,
                          PBARRAY_GENERIC_CONTENT_TYPE,
                          freeElement);
    pbarray_add(array, "hello once");
    pbarray_add(array, "hello twice");
    pbarray_add(array, "hello again");

    const int count = pbarray_count(array);
    pbarray_free(array);

    assert_that(elementFreeCounter, is_equal_to(count));

    array = NULL;
}