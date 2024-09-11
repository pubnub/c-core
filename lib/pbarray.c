/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#include "pbarray.h"

#include <stdio.h>
#include <string.h>
#include "core/pubnub_mutex.h"
#include "lib/pbref_counter.h"


// ----------------------------------
//              Types
// ----------------------------------

typedef struct pbarray_entry {
    // Pointer to the current entry value.
    const void* value;
    // Object references counter.
    pbref_counter_t* counter;
} pbarray_entry_t;

// Auto-resizable array definition.
struct pbarray {
    // Array resize strategy.
    pbarray_resize_strategy resize_strategy;
    // Type of data which will be stored in array.
    pbarray_content_type content_type;
    // Stored elements `free` function.
    pbarray_element_free free;
    /**
     * @brief Initial length of array.
     *
     * Reference length which will be used by new size computation function.
     */
    size_t initial_length;
    // Pointer to the pointers to the entries (`pbarray_entry_t`).
    void** elements;
    // Current array capacity.
    size_t length;
    // Number of elements stored in an array.
    size_t count;
    // Shared resources access lock.
    pubnub_mutex_t mutw;
};


// ----------------------------------
//        Function prototypes
// ----------------------------------

/**
 * @brief Remove element from array.
 *
 * \b Important: The element destruct function (if provided during array
 * allocation) will be used for the removed `element` (only if it existed in
 * array).
 *
 * @note Remove operation is expensive because it requires other elements to
 *       shift their position in array to close the “gap”.
 *
 * @param array           Pointer to the array from which element should be
 *                        removed.
 * @param element         Pointer to the element which should be removed.
 * @param all_occurrences Whether all `element` occurrences should be removed or
 *                        not.
 * @return `PBAR_OK` in case if new element has been removed.
 */
static pbarray_res _pbarray_remove(
    pbarray_t*  array,
    const void* element,
    bool        all_occurrences);

/**
 * @brief Compute size, which depends on whether the array increases or shrinks.
 *
 * @param array    Pointer to the array for which new size should be computed.
 * @param increase Whether array should be increased in size or not.
 * @return New array size.
 */
static size_t _pbarray_target_length(const pbarray_t* array, bool increase);

/**
 * @brief Resize array to fit new elements or shrink to preserve memory.
 *
 * @param array    Pointer to the array which should be resized if it will be
 *                 required.
 * @param increase Whether array should be increased in size or not.
 * @return `PBAR_OK` in case if array resized or there were no need in resize.
 */
static pbarray_res _pbarray_resize(pbarray_t* array, bool increase);

/**
 * @brief Check whether elements are equal.
 *
 * @param array    Pointer to the array inside which `element1` is stored.
 * @param element1 Pointer to the element from the `array` which should be
 *                 compared to another.
 * @param element2 Pointer to the element which should be compared to the other
 *                 from array.
 * @return `true` in case if two elements are equal.
 */
static bool _pbarray_element_is_equal(
    const pbarray_t* array,
    const void*      element1,
    const void*      element2);

/**
 * @brief Create an array entry object.
 *
 * @param value   Pointer to the value which should be safely stored in an
 *                array.
 * @param counter Pointer to the `value` reference counter (if entry for a copy
 *                from another array).
 * @return Pointer to the ready to use array entry object or `NULL` in case of
 *         insufficient memory error. The returned pointer must be passed to the
 *         `_pbarray_entry_free` to avoid a memory leak.
 */
static pbarray_entry_t* _pbarray_entry_alloc(
    const void*      value,
    pbref_counter_t* counter);

/**
 * @brief Add entry with user-provided value at specific index in array.
 *
 * @param array Pointer to the array which will store the entry.
 * @param index Index into which `entry` should be placed.
 * @param entry Pointer to the entry which holds user-provided value.
 * @return Entry addition operation result.
 */
static pbarray_res _pbarray_add_entry_at(
    pbarray_t*       array,
    size_t           index,
    pbarray_entry_t* entry);

/**
 * @brief Remove entry by index from array.
 *
 * \b Important: The entry value destruct function (if provided during array
 * allocation) will be used for the removed `element` (only if it existed in
 * array).
 *
 * @note Remove operation is expensive because it requires other elements to
 *       shift their position in array to close the “gap”.
 *
 * @param array      Pointer to the array from which entry should be removed.
 * @param index      Index of the `entry` which should be removed.
 * @param free_entry Whether entry value destructor should be used or not.
 * @return Entry removal operation result.
 */
static pbarray_res _pbarray_remove_entry_at(
    pbarray_t* array,
    size_t     index,
    bool       free_entry);

/**
 * @brief Clean up resources used by entry object.
 *
 * @param array      Pointer to the array which stored 'entry'.
 * @param entry      Pointer to the entry which should be to free up resources.
 * @param free_entry Whether entry value destructor should be used or not.
 */
static void _pbarray_entry_free(
    const pbarray_t* array,
    pbarray_entry_t* entry,
    bool             free_entry);


// ----------------------------------
//             Functions
// ----------------------------------

pbarray_t* pbarray_alloc(
    const size_t                  length,
    const pbarray_resize_strategy resize_strategy,
    const pbarray_content_type    content_type,
    const pbarray_element_free    free_fn)
{
    pbarray_t* array = malloc(sizeof(pbarray_t));
    if (NULL == array) { return NULL; }

    array->elements = malloc(length * sizeof(pbarray_entry_t*));
    if (NULL == array->elements) {
        free(array);
        return NULL;
    }

    pubnub_mutex_init(array->mutw);
    array->resize_strategy = resize_strategy;
    array->content_type    = content_type;
    array->initial_length  = length;
    array->length          = length;
    array->free            = free_fn;
    array->count           = 0;

    return array;
}

pbarray_t* pbarray_copy(const pbarray_t* array)
{
    pubnub_mutex_lock(array->mutw);
    const size_t count = array->count;
    if (0 == count) {
        pubnub_mutex_unlock(array->mutw);
        return NULL;
    }

    pbarray_t* copy = pbarray_alloc(array->length,
                                    array->resize_strategy,
                                    array->content_type,
                                    array->free);
    if (NULL == copy) {
        pubnub_mutex_unlock(array->mutw);
        return NULL;
    }
    copy->count = count;

    for (size_t i = 0; i < count; ++i) {
        const pbarray_entry_t* entry        = array->elements[i];
        pbarray_entry_t*       entry_to_add = _pbarray_entry_alloc(
            entry->value,
            entry->counter);

        if (NULL == entry_to_add) {
            pubnub_mutex_unlock(array->mutw);
            pbarray_free(copy);
            return NULL;
        }

        _pbarray_add_entry_at(copy, i, entry_to_add);
    }
    pubnub_mutex_unlock(array->mutw);

    return copy;
}

size_t pbarray_count(const pbarray_t* array)
{
    pubnub_mutex_lock(array->mutw);
    const size_t count = array->count;
    pubnub_mutex_unlock(array->mutw);

    return count;
}

bool pbarray_contains(const pbarray_t* array, const void* element)
{
    pubnub_mutex_lock(array->mutw);
    for (int i = 0; i < array->count; ++i) {
        const pbarray_entry_t* entry = array->elements[i];

        if (_pbarray_element_is_equal(array, entry->value, element)) {
            pubnub_mutex_unlock(array->mutw);
            return true;
        }
    }
    pubnub_mutex_unlock(array->mutw);

    return false;
}

const void** pbarray_elements(const pbarray_t* array, size_t* count)
{
    pubnub_mutex_lock(array->mutw);
    const size_t cnt      = array->count;
    const void** elements = malloc((0 == cnt ? 1 : cnt) * sizeof(void*));

    if (NULL != count) { *count = cnt; }
    if (0 == cnt || NULL == elements) {
        pubnub_mutex_unlock(array->mutw);
        return elements;
    }

    for (size_t i = 0; i < cnt; ++i) {
        elements[i] = (void*)((pbarray_entry_t*)array->elements[i])->value;
    }
    pubnub_mutex_unlock(array->mutw);

    return elements;
}

pbarray_res pbarray_add(pbarray_t* array, const void* element)
{
    pubnub_mutex_lock(array->mutw);
    pbarray_entry_t* entry = _pbarray_entry_alloc(element, NULL);
    if (NULL == entry) {
        pubnub_mutex_unlock(array->mutw);
        return PBAR_OUT_OF_MEMORY;
    }

    const pbarray_res result =
        _pbarray_add_entry_at(array, array->count, entry);
    if (PBAR_OK != result) { _pbarray_entry_free(array, entry, false); }
    pubnub_mutex_unlock(array->mutw);

    return result;
}

pbarray_res pbarray_insert(
    pbarray_t*   array,
    const void*  element,
    const size_t idx)
{
    pubnub_mutex_lock(array->mutw);
    pbarray_entry_t* entry = _pbarray_entry_alloc(element, NULL);
    if (NULL == entry) {
        pubnub_mutex_unlock(array->mutw);
        return PBAR_OUT_OF_MEMORY;
    }

    const pbarray_res result = _pbarray_add_entry_at(array, idx, entry);
    if (PBAR_OK != result) { _pbarray_entry_free(array, entry, false); }
    pubnub_mutex_unlock(array->mutw);

    return result;
}

pbarray_res pbarray_merge(pbarray_t* array, const pbarray_t* other_array)
{
    if (NULL == other_array) { return PBAR_OK; }

    pubnub_mutex_lock(array->mutw);
    pbarray_res result = PBAR_OK;

    for (size_t i = 0; i < other_array->count; ++i) {
        const pbarray_entry_t* entry        = other_array->elements[i];
        pbarray_entry_t*       entry_to_add = _pbarray_entry_alloc(
            entry->value,
            entry->counter);

        if (NULL == entry_to_add) { result = PBAR_OUT_OF_MEMORY; }
        else { result = _pbarray_add_entry_at(array, i, entry_to_add); }
        if (PBAR_OK != result) { break; }
    }
    pubnub_mutex_unlock(array->mutw);

    return result;
}

pbarray_res pbarray_remove(
    pbarray_t*  array,
    const void* element,
    const bool  all_occurrences)
{
    pubnub_mutex_lock(array->mutw);
    const pbarray_res result = _pbarray_remove(array, element, all_occurrences);
    pubnub_mutex_unlock(array->mutw);

    return result;
}

pbarray_res pbarray_remove_element_at(pbarray_t* array, const size_t idx)
{
    pubnub_mutex_lock(array->mutw);
    if (0 == array->count || idx >= array->count) {
        pubnub_mutex_unlock(array->mutw);
        return idx >= array->count ? PBAR_INDEX_OUT_OF_RANGE : PBAR_OK;
    }

    _pbarray_remove_entry_at(array, idx, true);
    pubnub_mutex_unlock(array->mutw);

    return PBAR_OK;
}

pbarray_res pbarray_remove(pbarray_t* array)
{
    pubnub_mutex_lock(array->mutw);
    for (int i = 0; i < array->count - 1; ++i) {
        _pbarray_entry_free(array, array->elements[i], true);
    }
    array->count = 0;
    pubnub_mutex_unlock(array->mutw);

    return PBAR_OK;
}

void pbarray_subtract(
    pbarray_t*       array,
    const pbarray_t* other_array,
    const bool       all_occurrences)
{
    if (NULL == other_array) { return; }

    pubnub_mutex_lock(array->mutw);
    for (size_t i = 0; i < other_array->count; ++i) {
        const pbarray_entry_t* other_entry = other_array->elements[i];

        for (int j = 0; j < array->count;) {
            const pbarray_entry_t* entry = array->elements[j];

            if (_pbarray_element_is_equal(array,
                                          entry->value,
                                          other_entry->value)) {
                _pbarray_remove_entry_at(array, j, true);
                if (!all_occurrences) { break; }
            }
            else { j++; }
        }
    }
    pubnub_mutex_unlock(array->mutw);
}

const void* pbarray_element_at(const pbarray_t* array, const size_t idx)
{
    pubnub_mutex_lock(array->mutw);
    if (0 == array->count || idx >= array->count) {
        pubnub_mutex_unlock(array->mutw);
        return NULL;
    }

    const pbarray_entry_t* entry = array->elements[idx];
    pubnub_mutex_unlock(array->mutw);

    return entry->value;
}

const void* pbarray_first(const pbarray_t* array)
{
    pubnub_mutex_lock(array->mutw);
    const size_t           count = array->count;
    const pbarray_entry_t* entry = array->elements[0];
    pubnub_mutex_unlock(array->mutw);

    return 0 == count ? NULL : entry->value;
}

const void* pbarray_last(const pbarray_t* array)
{
    pubnub_mutex_lock(array->mutw);
    const size_t           count = array->count;
    const pbarray_entry_t* entry = array->elements[count - 1];
    pubnub_mutex_unlock(array->mutw);
    return 0 == count ? NULL : entry->value;
}

const void* pbarray_pop_first(pbarray_t* array)
{
    pubnub_mutex_lock(array->mutw);
    if (0 == array->count) {
        pubnub_mutex_unlock(array->mutw);
        return NULL;
    }

    const pbarray_entry_t* entry = array->elements[0];
    const void* value = entry->value;
    _pbarray_remove_entry_at(array, 0, false);
    pubnub_mutex_unlock(array->mutw);

    return value;
}

const void* pbarray_pop_last(pbarray_t* array)
{
    pubnub_mutex_lock(array->mutw);
    if (0 == array->count) {
        pubnub_mutex_unlock(array->mutw);
        return NULL;
    }

    const pbarray_entry_t* entry = array->elements[array->count - 1];
    const void* value = entry->value;
    _pbarray_remove_entry_at(array, array->count - 1, false);
    pubnub_mutex_unlock(array->mutw);

    return value;
}

void pbarray_free(pbarray_t* array)
{
    pbarray_free_with_destructor(array, array->free);
}

void pbarray_free_with_destructor(
    pbarray_t*                 array,
    const pbarray_element_free free_fn)
{
    pubnub_mutex_lock(array->mutw);
    if (NULL != free_fn) { array->free = free_fn; }
    for (int i = 0; i < array->count; ++i) {
        _pbarray_entry_free(array, array->elements[i], true);
    }
    pubnub_mutex_unlock(array->mutw);
    pubnub_mutex_destroy(array->mutw);
    free(array);
}

pbarray_res _pbarray_remove(
    pbarray_t*  array,
    const void* element,
    const bool  all_occurrences)
{
    if (NULL == element || 0 == array->count) { return PBAR_NOT_FOUND; }

    bool found = false;

    for (int i = 0; i < array->count;) {
        const pbarray_entry_t* entry = array->elements[i];

        if (_pbarray_element_is_equal(array, entry->value, element)) {
            _pbarray_remove_entry_at(array, i, true);
            found = true;
            if (!all_occurrences) { break; }
        }
        else { i++; }
    }

    _pbarray_resize(array, false);

    return found ? PBAR_OK : PBAR_NOT_FOUND;
}

size_t _pbarray_target_length(const pbarray_t* array, const bool increase)
{
    int resize_len = 0;
    if (array->resize_strategy == PBARRAY_RESIZE_CONSERVATIVE) {
        resize_len = 1;
    }
    else if (array->resize_strategy == PBARRAY_RESIZE_OPTIMISTIC) {
        resize_len = increase ? array->length : array->initial_length;
    }
    else if (array->resize_strategy == PBARRAY_RESIZE_BALANCED) {
        resize_len = array->initial_length * .5;
    }

    if (!increase) {
        if (array->length - array->count < resize_len && (
                array->resize_strategy == PBARRAY_RESIZE_OPTIMISTIC ||
                array->resize_strategy == PBARRAY_RESIZE_BALANCED)) {
            resize_len = 0;
        }
    }
    else {
        if (array->count < array->length) { resize_len = 0; }
        else
            if (array->resize_strategy == PBARRAY_RESIZE_NONE) { return 0; }
    }

    return array->length + resize_len * (increase ? 1 : -1);
}

pbarray_res _pbarray_resize(pbarray_t* array, const bool increase)
{
    const size_t length = _pbarray_target_length(array, increase);

    if (0 == length) { return PBAR_FIXED_SIZE; }
    if (length != array->length) {
        void** elements = realloc(array->elements,
                                  length * sizeof(pbarray_entry_t*));
        if (NULL == elements) { return PBAR_OUT_OF_MEMORY; }

        array->elements = elements;
        array->length   = length;
    }

    return PBAR_OK;
}

bool _pbarray_element_is_equal(
    const pbarray_t* array,
    const void*      element1,
    const void*      element2)
{
    if (array->content_type == PBARRAY_CHAR_CONTENT_TYPE) {
        if (strcmp(element1, element2) == 0) { return true; }
    }
    else
        if (element1 == element2) { return true; }

    return false;
}

pbarray_entry_t* _pbarray_entry_alloc(
    const void*      value,
    pbref_counter_t* counter)
{
    pbarray_entry_t* entry = malloc(sizeof(pbarray_entry_t));
    if (NULL == entry) { return NULL; }

    if (NULL == counter) {
        entry->counter = pbref_counter_alloc();
        entry->value   = value;
    }
    else { entry->counter = counter; }

    return entry;
}

pbarray_res _pbarray_add_entry_at(
    pbarray_t*       array,
    const size_t     index,
    pbarray_entry_t* entry)
{
    if (index > array->length) { return PBAR_INDEX_OUT_OF_RANGE; }

    const pbarray_res resize_result = _pbarray_resize(array, true);
    if (PBAR_OK != resize_result) { return resize_result; }

    // Check whether addition is done not to the end of the array to shift
    // other entries position.
    if (array->count != index) {
        for (int i = array->count; i > index; --i) {
            array->elements[i] = array->elements[i - 1];
        }
    }

    pbref_counter_increment(entry->counter);
    array->elements[index] = (void*)entry;
    array->count++;

    return PBAR_OK;
}

pbarray_res _pbarray_remove_entry_at(
    pbarray_t*   array,
    const size_t index,
    const bool   free_entry)
{
    pbarray_entry_t* entry = array->elements[index];
    if (NULL == entry) { return PBAR_NOT_FOUND; }

    _pbarray_entry_free(array, entry, free_entry);

    for (int j = index; j < array->count - 1; ++j) {
        array->elements[j] = array->elements[j + 1];
    }
    array->count--;

    return PBAR_OK;
}

void _pbarray_entry_free(
    const pbarray_t* array,
    pbarray_entry_t* entry,
    const bool       free_entry)
{
    if (NULL == entry) { return; }

    if (0 == pbref_counter_free(entry->counter)) {
        if (free_entry && NULL != array->free)
            array->free((void*)entry->value);
    }
    free(entry);
}