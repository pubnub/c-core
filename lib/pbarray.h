/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#ifndef PBARRAY_H
#define PBARRAY_H

/**
 * @file pbarray.h
 *
 * Resizable array type implementation.
 */

#include <stdbool.h>
#include <stdlib.h>


// ----------------------------------
//              Types
// ----------------------------------

typedef enum
{
    /**
     * @brief Array with static length.
     *
     * If element doesn't fit array won't resize.
     */
    PBARRAY_RESIZE_NONE,
    /**
     * @brief Minimalistic array resize strategy.
     *
     * Resize array by one (if element added or removed).
     */
    PBARRAY_RESIZE_CONSERVATIVE,
    /**
     * @brief Aggressive array resize.
     *
     * With each resize, the size of the array will be doubled when a new
     * element won't fit to the size of the array.
     *
     * @note The initial size of the array will be used to identify when the
     *       array should shrink to preserve memory.
     */
    PBARRAY_RESIZE_OPTIMISTIC,
    /**
     * @brief Moderate array resize strategy.
     *
     * With each resize, the size of the array will be increased to half of the
     * initial size when a new element won't fit to the size of the array.
     *
     * @note Half of the initial size of the array will be used to identify when
     *       the array should shrink to preserve memory.
     */
    PBARRAY_RESIZE_BALANCED,
} pbarray_resize_strategy;

/**
 * @brief Type of data stored in an array.
 *
 * Information about array content lets some functions work more efficiently.
 */
typedef enum
{
    /**
     * @brief Array used to store generic data as pointers.
     *
     * @note Content matching will be done using memory addresses.
     */
    PBARRAY_GENERIC_CONTENT_TYPE,
    /**
     * @brief Array used to store string pointers.
     *
     * @note Content matching will be done using `strcmp`.
     */
    PBARRAY_CHAR_CONTENT_TYPE
} pbarray_content_type;

/** Array result codes. */
typedef enum
{
    /**
     * @brief Success.
     *
     * Operation finished successfully.
     */
    PBAR_OK,
    /**
     * @brief Element not found.
     *
     * Referenced element not found in the array.
     */
    PBAR_NOT_FOUND,
    /**
     * @brief Element can't be added or inserted.
     *
     * A new element cannot be inserted into the array as it has been configured
     * with the `PBARRAY_RESIZE_NONE` resize policy to ensure its unalterable
     * capacity.
     */
    PBAR_FIXED_SIZE,
    /**j
     * @brief Ran out of dynamic memory.
     *
     * Operation failed because there were not enough memory to allocate or
     * reallocate structures.
     */
    PBAR_OUT_OF_MEMORY,
    /**
     * @brief Index is out of range.
     *
     * Provided element index is out of array values range (empty or larger than
     * current array capacity).
     */
    PBAR_INDEX_OUT_OF_RANGE,
} pbarray_res;

/**
 * @brief Array element `free` function prototype.
 *
 * Function which will be used when hash set is freed using 'pbarray_free'.
 *
 * @note Element destructor function will `NULL`ify provided pointer.
 */
typedef void (*pbarray_element_free)(void*);

/** Auto-resizable array type definition. */
typedef struct pbarray pbarray_t;


// ----------------------------------
//             Functions
// ----------------------------------

/**
 * @brief Create auto-resizable array.
 * @code
 * // Example: Create a static sized array.
 * pbarray_t* array = pbarray_alloc(1,
 *                                  PBARRAY_RESIZE_NONE,
 *                                  PBARRAY_CHAR_CONTENT_TYPE,
 *                                  free);
 * pbarray_add(array, "hello");
 *  // This call will fail because PBARRAY_RESIZE_NONE strategy has been used.
 * pbarray_add(array, " there!");
 * @endcode
 * @code
 * // Example: Create array with dynamic length.
 * pbarray_t* array = pbarray_alloc(1,
 *                                  PBARRAY_RESIZE_OPTIMISTIC,
 *                                  PBARRAY_CHAR_CONTENT_TYPE,
 *                                  free);
 * pbarray_add(array, "hello");
 * pbarray_add(array, " there!");
 * @endcode
 *
 * @param length          Initial length of the array.
 * @param resize_strategy Strategy which should be used when a new element
 *                        doesn't fit to the size of the array.
 * @param content_type    Type of data which will be stored in array.
 * @param free_fn         Elements `free` function. The value could be set to
 *                        `NULL` and leave memory management to the array user.
 * @return Pointer to the ready to use resizable array or `NULL` in case of
 *         insufficient memory error. The returned pointer must be passed to the
 *         `pbarray_free` to avoid a memory leak.
 *
 * @see pbarray_add
 * @see pbarray_free
 */
pbarray_t* pbarray_alloc(
    size_t length,
    pbarray_resize_strategy resize_strategy,
    pbarray_content_type content_type,
    pbarray_element_free free_fn);

/**
 * @brief Create shallow array copy.
 * @code
 * pbarray_add(array_orig, "hello");
 * pbarray_add(array_orig, " there!");
 * pbarray_t* array_copy = pbarray_copy(array_orig);
 *
 * // Prints: Contains 'hello' in 'array_copy'? true
 * printf("Contains 'hello' in 'array_copy'? %s",
 *        pbarray_contains(array_copy, "hello") ? "true" : "false");
 *
 * // Prints: Contains ' there!' in 'array_copy'? true
 * printf("Contains ' there!' in 'array_copy'? %s",
 *        pbarray_contains(array_copy, " there!") ? "true" : "false");
 * @endcode
 *
 * \b Important: Copies store shared object, which never shouldn't be freed
 * directly to avoid runtime segmentation fault error. This implementation uses
 * reference counting and actual data will be freed only if all arrays freed or
 * entry has been removed from all arrays.<br/>
 * \b Important: Make sure to have element destructor specified for the source
 * array or in case of insufficient memory error already copied entries won't be
 * able to free up used resource.
 *
 * @param array Pointer to the array from which shallow copy should be created.
 * @return Pointer to the `array` copy or `NULL` if case of insufficient
 *         memory error. The returned pointer must be passed to the
 *         `pbarray_free` to avoid a memory leak.
 *
 * @see pbarray_remove
 * @see pbarray_remove_element_at
 * @see pbarray_remove_all
 * @see pbarray_pop_first
 * @see pbarray_pop_last
 * @see pbarray_free
 * @see pbarray_free_with_destructor
 */
pbarray_t* pbarray_copy(pbarray_t* array);

/**
 * @brief Number of elements in array.
 *
 * @param array Pointer to the array, for which number of elements should be
 *              retrieved.
 * @return Number of elements added to the array.
 */
size_t pbarray_count(pbarray_t* array);

/**
 * @brief Check whether element has been added to the array or not.
 *
 * @param array   Pointer to the array, inside which `element` presence should
 *                be checked.
 * @param element Pointer to the element which should be checked.
 * @return `true` in case if `element` already exists in the `array`.
 */
bool pbarray_contains(pbarray_t* array, const void* element);

/**
 * @brief Get array elements.
 * @code
 * // Example: Shallow copy of the elements in array.
 * size_t count;
 * void** elements = pbarray_elements(array, &count);
 * if (NULL == elements) {
 *     // handle insufficient memory error.
 * } else if (0 == count) {
 *     // array is empty.
 * } else {
 *    // work with data.
 * }
 *
 * // Free up when done.
 * if (NULL != elements) { free(elements); }
 * @endcode
 *
 * \b Warning: Returned pointer to the array's element pointers share entries
 * with `array` and they shouldn't be manually freed.<br/>
 * \b Warning: Returned pointer to the array's element pointers, and could be
 * invalid if `array` or all (some) elements are freed before values will be
 * used.
 *
 * @param array        Pointer to the array, from which list of element pointers
 *                     should be retrieved.
 * @param [out] count  Parameter will hold the count of returned elements.
 * @return Pointer to the array's value pointers, or `NULL` in case of
 *         insufficient memory error. The returned pointer must be passed to the
 *         `free` to avoid a memory leak.
 */
const void** pbarray_elements(pbarray_t* array, size_t* count);

/**
 * @brief Add a new element to the end of the array.
 *
 * @param array   Pointer to the array to which new element should be added.
 * @param element Pointer to the element which should be added.
 * @return `PBAR_OK` in case if new element has been added.
 */
pbarray_res pbarray_add(pbarray_t* array, void* element);

/**
 * @brief Insert a new element at a specific location in the array.
 *
 * @note Insert operation is expensive because it requires other elements to
 *       shift their position in the array to create a “gap” for the new element.
 *
 * @param array   Pointer to the array to which new element should be inserted.
 * @param element Pointer to the element which should be inserted.
 * @param idx     Index at which `element` should be inserted. Element at
 *                specified index, and after it will be pushed to the right.
 * @return `PBAR_OK` in case if new element has been inserted.
 */
pbarray_res pbarray_insert_at(pbarray_t* array, void* element, size_t idx);

/**
 * @brief Add elements from another array to the end of the array.
 *
 * @param array       Pointer to the array to which new elements should be added.
 * @param other_array Pointer to the array with elements which should be added.
 * @return `PBAR_OK` in case if new elements has been added.
 */
pbarray_res pbarray_merge(pbarray_t* array, pbarray_t* other_array);

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
 * @param array           Pointer to the array from which element should be
 *                        removed.
 * @param element         Pointer to the element which should be removed or not.
 * @param all_occurrences Whether all `element` occurrences should be removed or
 *                        not.
 * @return `PBAR_OK` in case if element has been removed.
 */
pbarray_res pbarray_remove(
    pbarray_t* array,
    void** element,
    bool all_occurrences);

/**
 * @brief Remove element at specific index from array.
 *
 * \b Important: The element destruct function (if provided during array
 * allocation) will be used for the removed `element` (only if it existed in
 * array).
 *
 * @note Remove operation is expensive because it requires other elements to
 *       shift their position in array to close the “gap”.
 *
 * @param array Pointer to the array from which element should be removed.
 * @param idx   Index at which element for removal is stored in array.
 * @return `PBAR_OK` in case if element has been removed.
 */
pbarray_res pbarray_remove_element_at(pbarray_t* array, size_t idx);

/**
 * @brief Remove all element from array.
 *
 * \b Important: The element destruct function (if provided during array
 * allocation) will be used for the removed `element` (only if it existed in
 * array).
 *
 * @param array Pointer to the array from which elements should be removed.
 * @return `PBAR_OK` in case if all elements has been removed.
 */
pbarray_res pbarray_remove_all(pbarray_t* array);

/**
 * @brief Remove elements from array which is present in both arrays.
 *
 * @param array           Pointer to the array from which element should be
 *                        removed.
 * @param other_array     Pointer to the array with elements which should be
 *                        removed.
 * @param all_occurrences Whether all `element` occurrences from `other_array`
 *                        should be removed from `array` or not.
 */
void pbarray_subtract(
    pbarray_t* array,
    pbarray_t* other_array,
    bool all_occurrences);

/**
 * @brief Get element from `array` at specified index.
 *
 * @param array Pointer to the array from which element should be retrieved.
 * @param idx   Index at which requested element stored in array.
 * @return Pointer to the element from array at specified index or `NULL` if it
 *         is empty or smaller than provided `idx`.
 */
const void* pbarray_element_at(pbarray_t* array, size_t idx);

/**
 * @brief First element in `array`.
 *
 * @param array Pointer to the array from which element should be retrieved.
 * @return Pointer to the first element in array or `NULL` if it is empty.
 */
const void* pbarray_first(pbarray_t* array);

/**
 * @brief Last element in `array`.
 *
 * @param array Pointer to the array from which element should be retrieved.
 * @return Pointer to the last element in array or `NULL` if it is empty.
 */
const void* pbarray_last(pbarray_t* array);

/**
 * @brief Remove first element from `array`.
 *
 * @param array Pointer to the array from which element should be removed.
 * @return Pointer to th first removed element in array or `NULL` if it is
 *         empty. The returned pointer must be freed to avoid a memory leak.
 */
const void* pbarray_pop_first(pbarray_t* array);

/**
 * @brief Remove last element from `array`.
 *
 * @param array Pointer to the array from which element should be removed.
 * @return Pointer to the last removed element in array or `NULL` if it is
 *         empty. The returned pointer must be freed to avoid a memory leak.
 */
const void* pbarray_pop_last(pbarray_t* array);

/**
 * @brief Clean up resources used by `array`.
 *
 * @note User is responsible for reclaiming resources used by the elements if
 *       element destructing function is not provided.
 * @note Function will `NULL`ify provided array pointer.
 *
 * @param array Pointer to the array, which should free up resources used for it
 *              and stored elements.
 */
void pbarray_free(pbarray_t** array);

/**
 * @brief Clean up resources used by `array`.
 *
 * @note User is responsible for reclaiming resources used by the elements if
 *       element destructing function is not provided.
 * @note Function will NULLify provided array pointer.
 *
 * @param array   Pointer to the array, which should free up resources used for
 *                it and stored elements.
 * @param free_fn Elements `free` function. The value could be set to `NULL` and
 *                leave memory management to the array user.
 */
void pbarray_free_with_destructor(
    pbarray_t** array,
    pbarray_element_free free_fn);
#endif // #ifndef PBARRAY_H
