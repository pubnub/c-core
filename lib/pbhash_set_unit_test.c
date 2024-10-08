/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cgreen/include/cgreen/constraint_syntax_helpers.h"
#include "cgreen/include/cgreen/constraint.h"
#include "cgreen/include/cgreen/assertions.h"
#include "cgreen/include/cgreen/filename.h"
#include "cgreen/include/cgreen/cgreen.h"

#include "lib/pbhash_set.h"


// ----------------------------------
//              Statics
// ----------------------------------

static int           elementFreeCounter = 0;
static pbhash_set_t* set                = NULL;

typedef struct {
    char* name;
    int   age;
} user_t;

// ----------------------------------
//              Helpers
// ----------------------------------

user_t* createUser(char* name, const int age)
{
    user_t* user = malloc(sizeof(user_t));
    user->name   = name;
    user->age    = age;

    return user;
}

void freeElement(void* __unused element)
{
    elementFreeCounter++;
}


// ----------------------------------
//            Tests setup
// ----------------------------------

Describe(pbhash_set);

BeforeEach(pbhash_set)
{
    set                = NULL;
    elementFreeCounter = 0;
}

AfterEach(pbhash_set)
{
    if (NULL != set) { pbhash_set_free(&set); }
}


// ----------------------------------
//              Tests
// ----------------------------------

Ensure(pbhash_set, should_allocate)
{
    set = pbhash_set_alloc(10, PBHASH_SET_GENERIC_CONTENT_TYPE, NULL);

    assert_that(set, is_non_null);
}

Ensure(pbhash_set, should_add_unique_strings)
{
    set = pbhash_set_alloc(10, PBHASH_SET_CHAR_CONTENT_TYPE, NULL);

    assert_that(pbhash_set_add(set, "hello once", NULL), is_equal_to(PBHSR_OK));
    assert_that(pbhash_set_add(set, "hello twice", NULL),
                is_equal_to(PBHSR_OK));
}

Ensure(pbhash_set, should_not_add_duplicate_strings)
{
    char* value = "hello once";
    set = pbhash_set_alloc(10, PBHASH_SET_CHAR_CONTENT_TYPE, NULL);
    char* once = malloc((strlen(value) + 1) * sizeof(char));
    strcpy(once, value);

    assert_that(pbhash_set_add(set, value, NULL), is_equal_to(PBHSR_OK));
    // String with same value already exists.
    assert_that(pbhash_set_add(set, once, NULL),
                is_equal_to(PBHSR_VALUE_EXISTS));

    free(once);
}

Ensure(pbhash_set, should_not_add_exact_duplicate_strings)
{
    char* value = "hello once";
    set = pbhash_set_alloc(10, PBHASH_SET_CHAR_CONTENT_TYPE, NULL);

    assert_that(pbhash_set_add(set, value, NULL), is_equal_to(PBHSR_OK));
    // String is the same by value and address. Value may require manual memory
    // management.
    assert_that(pbhash_set_add(set, value, NULL),
                is_equal_to(PBHSR_EXACT_MATCH_EXISTS));
}

Ensure(pbhash_set, should_add_unique_data)
{
    set = pbhash_set_alloc(10, PBHASH_SET_GENERIC_CONTENT_TYPE, NULL);

    assert_that(pbhash_set_add(set, "hello once", NULL), is_equal_to(PBHSR_OK));
    assert_that(pbhash_set_add(set, "hello twice", NULL),
                is_equal_to(PBHSR_OK));
}

Ensure(pbhash_set, should_add_duplicate_data_with_different_address)
{
    const char* value = "hello once";
    set = pbhash_set_alloc(10, PBHASH_SET_GENERIC_CONTENT_TYPE, NULL);
    char* once1 = malloc((strlen(value) + 1) * sizeof(char));
    strcpy(once1, value);
    char* once2 = malloc((strlen(value) + 1) * sizeof(char));
    strcpy(once2, value);

    assert_that(pbhash_set_add(set, once1, NULL), is_equal_to(PBHSR_OK));
    assert_that(pbhash_set_add(set, once2, NULL), is_equal_to(PBHSR_OK));

    free(once1);
    free(once2);
}

Ensure(pbhash_set, should_not_add_duplicate_data)
{
    const char* value = "hello once";
    set = pbhash_set_alloc(10, PBHASH_SET_GENERIC_CONTENT_TYPE, NULL);
    char* once = malloc((strlen(value) + 1) * sizeof(char));
    strcpy(once, value);
    pbhash_set_add(set, once, NULL);

    // String with same value already exists.
    assert_that(pbhash_set_add(set, once, NULL),
                is_equal_to(PBHSR_EXACT_MATCH_EXISTS));

    free(once);
}

Ensure(pbhash_set, should_add_structured_data_with_key)
{
    set          = pbhash_set_alloc(10, PBHASH_SET_GENERIC_CONTENT_TYPE, NULL);
    user_t* user = createUser("Bob", 28);

    assert_that(pbhash_set_add(set, user->name, user), is_equal_to(PBHSR_OK));
    size_t     count    = 0;
    const char** elements = (const char**)pbhash_set_elements(set, &count);
    assert_that(elements, is_not_null);
    assert_that(elements[0], is_equal_to(user));

    free(user);
}

Ensure(pbhash_set, should_not_add_duplicate_structured_data_with_key)
{
    set          = pbhash_set_alloc(10, PBHASH_SET_GENERIC_CONTENT_TYPE, NULL);
    user_t* user = createUser("Bob", 28);

    assert_that(pbhash_set_add(set, user->name, user), is_equal_to(PBHSR_OK));
    assert_that(pbhash_set_add(set, user->name, user),
                is_equal_to(PBHSR_EXACT_MATCH_EXISTS));

    free(user);
}

Ensure(pbhash_set, should_remove_string)
{
    char* removed_str = "hello twice";
    set = pbhash_set_alloc(10, PBHASH_SET_CHAR_CONTENT_TYPE, freeElement);
    pbhash_set_add(set, "hello once", NULL);
    pbhash_set_add(set, "hello twice", NULL);

    assert_that(pbhash_set_remove(set, (void**)&removed_str, NULL), is_equal_to(PBHSR_OK));
    assert_that(elementFreeCounter, is_equal_to(1));
}

Ensure(pbhash_set, should_not_remove_string_when_not_found)
{
    char* removed_str = "hello twice";
    set = pbhash_set_alloc(10, PBHASH_SET_CHAR_CONTENT_TYPE, freeElement);
    pbhash_set_add(set, "hello once", NULL);

    assert_that(pbhash_set_remove(set, (void**)&removed_str, NULL),
                is_equal_to(PBHSR_NOT_FOUND));
    assert_that(elementFreeCounter, is_equal_to(0));
}

Ensure(pbhash_set, should_union_only_unique_elements)
{
    pbhash_set_t* set1 = pbhash_set_alloc(10,
                                          PBHASH_SET_CHAR_CONTENT_TYPE,
                                          NULL);
    pbhash_set_add(set1, "hello once", NULL);
    pbhash_set_t* set2 = pbhash_set_alloc(10,
                                          PBHASH_SET_CHAR_CONTENT_TYPE,
                                          NULL);
    pbhash_set_add(set2, "hello once", NULL);
    pbhash_set_add(set2, "hello twice", NULL);

    assert_that(pbhash_set_union(set1, set2, NULL), is_equal_to(PBHSR_OK));
    assert_that(pbhash_set_contains(set1, "hello twice"), is_true);

    pbhash_set_free(&set1);
    pbhash_set_free(&set2);
}

Ensure(pbhash_set, should_not_free_shared_data_after_union_when_one_set_free)
{
    pbhash_set_t* set1 = pbhash_set_alloc(10,
                                          PBHASH_SET_CHAR_CONTENT_TYPE,
                                          freeElement);
    pbhash_set_add(set1, "hello once", NULL);
    pbhash_set_t* set2 = pbhash_set_alloc(10,
                                          PBHASH_SET_CHAR_CONTENT_TYPE,
                                          freeElement);
    pbhash_set_add(set2, "hello twice", NULL);
    pbhash_set_union(set1, set2, NULL);

    pbhash_set_free(&set2);
    assert_that(elementFreeCounter, is_equal_to(0));

    pbhash_set_free(&set1);
}

Ensure(pbhash_set, should_free_shared_data_shared_with_union_when_all_set_free)
{
    pbhash_set_t* set1 = pbhash_set_alloc(10,
                                          PBHASH_SET_CHAR_CONTENT_TYPE,
                                          freeElement);
    pbhash_set_add(set1, "hello once", NULL);
    pbhash_set_t* set2 = pbhash_set_alloc(10,
                                          PBHASH_SET_CHAR_CONTENT_TYPE,
                                          freeElement);
    pbhash_set_add(set2, "hello twice", NULL);
    pbhash_set_union(set1, set2, NULL);

    pbhash_set_free(&set2);
    assert_that(elementFreeCounter, is_equal_to(0));
    pbhash_set_free(&set1);
    assert_that(elementFreeCounter, is_equal_to(2));
}

Ensure(pbhash_set, should_subtract_elements)
{
    pbhash_set_t* set1 = pbhash_set_alloc(10,
                                          PBHASH_SET_CHAR_CONTENT_TYPE,
                                          freeElement);
    pbhash_set_add(set1, "hello once", NULL);
    pbhash_set_t* set2 = pbhash_set_alloc(10,
                                          PBHASH_SET_CHAR_CONTENT_TYPE,
                                          freeElement);
    pbhash_set_add(set2, "hello twice", NULL);
    pbhash_set_union(set1, set2, NULL);

    assert_that(pbhash_set_subtract(set1, set2), is_equal_to(PBHSR_OK));
    assert_that(pbhash_set_contains(set1, "hello twice"), is_false);
    pbhash_set_free(&set2);
    assert_that(elementFreeCounter, is_equal_to(1));

    pbhash_set_free(&set1);
}

Ensure(pbhash_set, should_contain_data_by_value)
{
    char* value = "hello once";
    set = pbhash_set_alloc(10, PBHASH_SET_CHAR_CONTENT_TYPE, freeElement);
    char* once = malloc((strlen(value) + 1) * sizeof(char));
    strcpy(once, value);
    pbhash_set_add(set, value, NULL);

    assert_that(pbhash_set_contains(set, once), is_true);

    free(once);
}

Ensure(pbhash_set, should_not_contain_data_by_address)
{
    char* value = "hello once";
    set = pbhash_set_alloc(10, PBHASH_SET_GENERIC_CONTENT_TYPE, freeElement);
    char* once = malloc((strlen(value) + 1) * sizeof(char));
    strcpy(once, value);
    pbhash_set_add(set, value, NULL);

    assert_that(pbhash_set_contains(set, once), is_false);

    free(once);
}

Ensure(pbhash_set, should_return_added_elements)
{
    char* value = "hello once";
    set = pbhash_set_alloc(10, PBHASH_SET_GENERIC_CONTENT_TYPE, NULL);
    char* once = malloc((strlen(value) + 1) * sizeof(char));
    strcpy(once, value);
    pbhash_set_add(set, value, NULL);
    pbhash_set_add(set, "hello twice", NULL);

    size_t     count    = 0;
    const char** elements = (const char**)pbhash_set_elements(set, &count);
    assert_that(elements, is_not_null);
    assert_that(count, is_equal_to(2));
    int found_one = 0;
    int found_two = 0;
    int found_three = 0;
    for (size_t i = 0; i < count; i++) {
        const char*element = elements[i];
        if (0 == strcmp(element, "hello once")) { found_one = 1; }
        if (0 == strcmp(element, "hello twice")) { found_two = 1; }
        if (0 == strcmp(element, once)) { found_three = 1; }
    }
    assert_that(found_one, is_equal_to(1));
    assert_that(found_two, is_equal_to(1));
    assert_that(found_three, is_equal_to(1));
    free(once);

}

Ensure(pbhash_set, should_free)
{
    set = pbhash_set_alloc(10, PBHASH_SET_CHAR_CONTENT_TYPE, freeElement);
    pbhash_set_add(set, "hello once", NULL);
    pbhash_set_add(set, "hello twice", NULL);

    pbhash_set_free(&set);

    assert_that(elementFreeCounter, is_equal_to(2));

    set = NULL;
}