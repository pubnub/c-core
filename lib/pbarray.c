/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pbarray.h"

#include <stdio.h>
#include <string.h>

#include "core/pubnub_mutex.h"
#include "lib/pbref_counter.h"


// ----------------------------------
//              Types
// ----------------------------------

/**
 * @brief Single array entry definition.
 *
 * To avoid potential segmentation failure on a secondary attempt to free
 * resources used by the entry, non-raw pointers have been chosen.
 */
typedef struct pbarray_entry {
    /** Pointer to the current entry value. */
    void* value;
    /** Object references counter. */
    pbref_counter_t* counter;
} pbarray_entry_t;

/** Auto-resizable array definition. */
struct pbarray {
    /** Array resize strategy. */
    pbarray_resize_strategy resize_strategy;
    /** Type of data which will be stored in array. */
    pbarray_content_type content_type;
    /** Stored elements `free` function. */
    pbarray_element_free free;
    /**
     * @brief Initial length of array.
     *
     * Reference length which will be used by new size computation function.
     */
    size_t initial_length;
    /**
     * @brief Pointer to the pointers to the entries.
     *
     * @see pbarray_entry_t
     */
    void** elements;
    /** Current array capacity. */
    size_t length;
    /** Number of elements stored in an array. */
    size_t count;
    /** Shared resources access lock. */
    pubnub_mutex_t mutw;
};


// ----------------------------------
//        Function prototypes
// ----------------------------------

/**
 * @brief Remove element from array.
 *
 * \b Important: If provided during array allocation, the element destruct
 * function will be used for the matched element only if there are no other
 * references to it (not shared with other arrays after `pbarray_merge`).<br/>
 * \b Important: The `element` pointer will be NULLified if the memory address
 * and value of the `element` match the elements that have been freed
 * (no references).
 *
 * @note Remove operation is expensive because it requires other elements to
 *       shift their position in array to close the “gap”.
 *
 * @param array                Pointer to the array from which element should be
 *                             removed.
 * @param element              Pointer to the element which should be removed.
 * @param all_occurrences      Whether all `element` occurrences should be
 *                             removed or not.
 * @param [in,out] freed_value Whether entry value actually has been freed
 *                             (no references) or not.
 * @return `PBAR_OK` in case if new element has been removed.
 */
static pbarray_res pbarray_remove_(
    pbarray_t* array,
    void**     element,
    bool       all_occurrences,
    bool*      freed_value);

/**
 * @brief Compute size, which depends on whether the array increases or shrinks.
 *
 * @param array    Pointer to the array for which new size should be computed.
 * @param increase Whether array should be increased in size or not.
 * @return New array size.
 */
static size_t pbarray_target_length_(const pbarray_t* array, bool increase);

/**
 * @brief Resize array to fit new elements or shrink to preserve memory.
 *
 * @param array    Pointer to the array which should be resized if it will be
 *                 required.
 * @param increase Whether array should be increased in size or not.
 * @return `PBAR_OK` in case if array resized or there were no need in resize.
 */
static pbarray_res pbarray_resize_(pbarray_t* array, bool increase);

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
static bool pbarray_element_is_equal_(
    const pbarray_t* array,
    const void*      element1,
    const void*      element2);

/**
 * @brief Create an array entry object.
 *
 * @param value Pointer to the value which should be safely stored in an array.
 * @return Pointer to the ready to use array entry object or `NULL` in case of
 *         insufficient memory error. The returned pointer must be passed to the
 *         `_pbarray_entry_free` to avoid a memory leak.
 */
static pbarray_entry_t* pbarray_entry_alloc_(void* value);

/**
 * @brief Add entry with user-provided value at specific index in array.
 *
 * @param array Pointer to the array which will store the entry.
 * @param index Index into which `entry` should be placed.
 * @param entry Pointer to the entry which holds user-provided value.
 * @param clone Whether `entry` clone should be added into array instead.
 * @return Entry addition operation result.
 */
static pbarray_res pbarray_add_entry_at_(
    pbarray_t*       array,
    size_t           index,
    pbarray_entry_t* entry,
    bool             clone);

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
 * @param array                Pointer to the array from which entry should be
 *                             removed.
 * @param index                Index of the `entry` which should be removed.
 * @param free_value           Whether entry value destructor should be used or
 *                             not.
 * @param [in,out] freed_value Whether entry value actually has been freed
 *                             (no references) or not.
 * @return Entry removal operation result.
 */
static pbarray_res pbarray_remove_entry_at_(
    pbarray_t* array,
    size_t     index,
    bool       free_value,
    bool*      freed_value);

/**
 * @brief Clean up resources used by entry object.
 *
 * @param array      Pointer to the array which stored 'entry'.
 * @param entry      Pointer to the entry which should be to free up resources.
 * @param free_value Whether entry value destructor should be used or not.
 * @return `true` if entry value actually has been freed.
 */
static bool pbarray_entry_free_(
    const pbarray_t* array,
    pbarray_entry_t* entry,
    bool             free_value);


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

pbarray_t* pbarray_copy(pbarray_t* array)
{
    if (NULL == array->free) { return NULL; }

    pubnub_mutex_lock(array->mutw);
    pbarray_t* copy = pbarray_alloc(array->length,
                                    array->resize_strategy,
                                    array->content_type,
                                    array->free);
    if (NULL == copy) {
        pubnub_mutex_unlock(array->mutw);
        return NULL;
    }

    for (size_t i = 0; i < array->count; ++i) {
        pbarray_entry_t* entry = array->elements[i];

        if (PBAR_OK != pbarray_add_entry_at_(copy, i, entry, true)) {
            pbarray_free(&copy);
            break;
        }
    }
    pubnub_mutex_unlock(array->mutw);

    return copy;
}

size_t pbarray_count(pbarray_t* array)
{
    if (NULL == array) { return 0; }

    pubnub_mutex_lock(array->mutw);
    const size_t count = array->count;
    pubnub_mutex_unlock(array->mutw);

    return count;
}

bool pbarray_contains(pbarray_t* array, const void* element)
{
    pubnub_mutex_lock(array->mutw);
    for (int i = 0; i < array->count; ++i) {
        const pbarray_entry_t* entry = array->elements[i];

        if (pbarray_element_is_equal_(array, entry->value, element)) {
            pubnub_mutex_unlock(array->mutw);
            return true;
        }
    }
    pubnub_mutex_unlock(array->mutw);

    return false;
}

const void** pbarray_elements(pbarray_t* array, size_t* count)
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
        elements[i] = ((pbarray_entry_t*)array->elements[i])->value;
    }
    pubnub_mutex_unlock(array->mutw);

    return elements;
}

pbarray_res pbarray_add(pbarray_t* array, void* element)
{
    pubnub_mutex_lock(array->mutw);
    pbarray_entry_t* entry = pbarray_entry_alloc_(element);
    if (NULL == entry) {
        pubnub_mutex_unlock(array->mutw);
        return PBAR_OUT_OF_MEMORY;
    }

    const pbarray_res result =
        pbarray_add_entry_at_(array, array->count, entry, false);
    if (PBAR_OK != result) { pbarray_entry_free_(array, entry, false); }
    pubnub_mutex_unlock(array->mutw);

    return result;
}

pbarray_res pbarray_insert_at(
    pbarray_t*   array,
    void*        element,
    const size_t idx)
{
    pubnub_mutex_lock(array->mutw);
    pbarray_entry_t* entry = pbarray_entry_alloc_(element);
    if (NULL == entry) {
        pubnub_mutex_unlock(array->mutw);
        return PBAR_OUT_OF_MEMORY;
    }

    const pbarray_res result = pbarray_add_entry_at_(array, idx, entry, false);
    if (PBAR_OK != result) { pbarray_entry_free_(array, entry, false); }
    pubnub_mutex_unlock(array->mutw);

    return result;
}

pbarray_res pbarray_merge(pbarray_t* array, pbarray_t* other_array)
{
    if (NULL == other_array) { return PBAR_OK; }

    pubnub_mutex_lock(array->mutw);
    pbarray_res result = PBAR_OK;

    pubnub_mutex_lock(other_array->mutw);
    for (size_t i = 0; i < other_array->count; ++i) {
        pbarray_entry_t* entry = other_array->elements[i];
        result = pbarray_add_entry_at_(array, array->count, entry, true);

        if (PBAR_OK != result) { break; }
    }
    pubnub_mutex_unlock(other_array->mutw);
    pubnub_mutex_unlock(array->mutw);

    return result;
}

pbarray_res pbarray_remove(
    pbarray_t* array,
    void**     element,
    const bool all_occurrences)
{
    pubnub_mutex_lock(array->mutw);
    bool              freed  = false;
    const pbarray_res result =
        pbarray_remove_(array, element, all_occurrences, &freed);

    /** Nullify element pointer if memory actually has been freed. */
    if (freed && NULL != element && NULL != *element) { *element = NULL; }
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

    pbarray_remove_entry_at_(array, idx, true, NULL);
    pubnub_mutex_unlock(array->mutw);

    return PBAR_OK;
}

pbarray_res pbarray_remove_all(pbarray_t* array)
{
    pubnub_mutex_lock(array->mutw);
    for (int i = 0; i < array->count; ++i) {
        pbarray_entry_free_(array, array->elements[i], true);
    }
    array->count = 0;
    pbarray_resize_(array, false);
    pubnub_mutex_unlock(array->mutw);

    return PBAR_OK;
}

void pbarray_subtract(
    pbarray_t* array,
    pbarray_t* other_array,
    const bool all_occurrences)
{
    if (NULL == other_array) { return; }

    pubnub_mutex_lock(array->mutw);
    pubnub_mutex_lock(other_array->mutw);
    for (size_t i = 0; i < other_array->count; ++i) {
        const pbarray_entry_t* other_entry = other_array->elements[i];

        for (int j = 0; j < array->count;) {
            const pbarray_entry_t* entry = array->elements[j];

            if (pbarray_element_is_equal_(array,
                                          entry->value,
                                          other_entry->value)) {
                pbarray_remove_entry_at_(array, j, true, NULL);
                if (!all_occurrences) { break; }
            }
            else { j++; }
        }
    }
    pbarray_resize_(array, false);
    pubnub_mutex_unlock(other_array->mutw);
    pubnub_mutex_unlock(array->mutw);
}

const void* pbarray_element_at(pbarray_t* array, const size_t idx)
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

const void* pbarray_first(pbarray_t* array)
{
    pubnub_mutex_lock(array->mutw);
    const size_t           count = array->count;
    const pbarray_entry_t* entry = array->elements[0];
    pubnub_mutex_unlock(array->mutw);

    return 0 == count ? NULL : entry->value;
}

const void* pbarray_last(pbarray_t* array)
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
    const void*            value = entry->value;
    pbarray_remove_entry_at_(array, 0, false, NULL);
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
    const void*            value = entry->value;
    pbarray_remove_entry_at_(array, array->count - 1, false, NULL);
    pubnub_mutex_unlock(array->mutw);

    return value;
}

void pbarray_free(pbarray_t** array)
{
    pbarray_free_with_destructor(array, (*array)->free);
}

void pbarray_free_with_destructor(
    pbarray_t**                array,
    const pbarray_element_free free_fn)
{
    pubnub_mutex_lock((*array)->mutw);
    if (NULL != free_fn) { (*array)->free = free_fn; }
    for (int i = 0; i < (*array)->count; ++i) {
        pbarray_entry_free_(*array, (*array)->elements[i], true);
    }
    pubnub_mutex_unlock((*array)->mutw);
    pubnub_mutex_destroy((*array)->mutw);
    free(*array);
    *array = NULL;
}

pbarray_res pbarray_remove_(
    pbarray_t* array,
    void**     element,
    const bool all_occurrences,
    bool*      freed_value)
{
    if (NULL == element || NULL == *element || 0 == array->count)
        return PBAR_NOT_FOUND;

    bool freed_match = false;
    bool found       = false;
    /** Whether single entry value is complete match of the element or not. */
    bool complete_match = false;
    /** Whether single entry value freed or not. */
    bool freed = false;

    for (int i = 0; i < array->count;) {
        const pbarray_entry_t* entry = array->elements[i];

        if (pbarray_element_is_equal_(array, entry->value, *element)) {
            complete_match = entry->value == *element;
            found          = true;
            pbarray_remove_entry_at_(array, i, true, &freed);

            if (complete_match && !freed_match) { freed_match = freed; }
            if (!all_occurrences) { break; }
        }
        else { i++; }
    }
    pbarray_resize_(array, false);

    /** Nullify element pointer if memory actually has been freed. */
    if (freed_match) { *element = NULL; }
    if (NULL != freed_value) { *freed_value = freed_match; }

    return found ? PBAR_OK : PBAR_NOT_FOUND;
}

size_t pbarray_target_length_(const pbarray_t* array, const bool increase)
{
    size_t resize_len = 0;
    if (array->resize_strategy == PBARRAY_RESIZE_CONSERVATIVE) {
        resize_len = 1;
    }
    else if (array->resize_strategy == PBARRAY_RESIZE_OPTIMISTIC) {
        resize_len = increase ? array->length : array->initial_length;
    }
    else if (array->resize_strategy == PBARRAY_RESIZE_BALANCED) {
        resize_len = array->initial_length / 2;
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

pbarray_res pbarray_resize_(pbarray_t* array, const bool increase)
{
    const size_t length = pbarray_target_length_(array, increase);

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

bool pbarray_element_is_equal_(
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

pbarray_entry_t* pbarray_entry_alloc_(void* value)
{
    pbarray_entry_t* entry = malloc(sizeof(pbarray_entry_t));
    if (NULL == entry) { return NULL; }

    entry->counter = pbref_counter_alloc();
    entry->value   = value;

    return entry;
}

pbarray_res pbarray_add_entry_at_(
    pbarray_t*       array,
    const size_t     index,
    pbarray_entry_t* entry,
    const bool       clone)
{
    if (index > array->length) { return PBAR_INDEX_OUT_OF_RANGE; }

    const pbarray_res resize_result = pbarray_resize_(array, true);
    if (PBAR_OK != resize_result) { return resize_result; }

    /**
     * Check whether addition is done not to the end of the array to shift
     * other entries position.
     */
    if (array->count != index) {
        for (size_t i = array->count; i > index; --i) {
            array->elements[i] = array->elements[i - 1];
        }
    }
    /** For safe sharing we need to increase counter. */
    if (clone) { pbref_counter_increment(entry->counter); }

    array->elements[index] = (void*)entry;
    array->count++;

    return PBAR_OK;
}

pbarray_res pbarray_remove_entry_at_(
    pbarray_t*   array,
    const size_t index,
    const bool   free_value,
    bool*        freed_value)
{
    pbarray_entry_t* entry = array->elements[index];
    if (NULL == entry) { return PBAR_NOT_FOUND; }

    const bool freed = pbarray_entry_free_(array, entry, free_value);
    if (NULL != freed_value) { *freed_value = freed; }

    for (size_t j = index; j < array->count - 1; ++j) {
        array->elements[j] = array->elements[j + 1];
    }
    array->count--;

    return PBAR_OK;
}

bool pbarray_entry_free_(
    const pbarray_t* array,
    pbarray_entry_t* entry,
    const bool       free_value)
{
    if (NULL == entry) { return true; }

    bool freed = false;

    if (0 == pbref_counter_free(entry->counter)) {
        if (free_value && NULL != array->free) {
            array->free(entry->value);
            freed = true;
        }
        free(entry);
    }

    return freed;
}