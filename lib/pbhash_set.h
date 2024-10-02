/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#ifndef PBHASH_SET_H
#define PBHASH_SET_H

/**
 * @file pbhash_set.h
 *
 * Hash set implementation for unique
 */

#include <stdbool.h>
#include <stdlib.h>


// ----------------------------------
//              Types
// ----------------------------------

/**
 * @brief Type of data stored in hash.
 *
 * The type of data affects hash computation algorithm.
 */
typedef enum
{
    /**
     * @brief Hash set used for generic data storage.
     *
     * Hash for generic data based on pointer value module from buckets length.
     */
    PBHASH_SET_GENERIC_CONTENT_TYPE,
    /**
     * @brief Hash set used for string data storage.
     *
     * Hash for string data computed from the value of each character in the
     * string.
     */
    PBHASH_SET_CHAR_CONTENT_TYPE
} pbhash_set_content_type;

/** Hash set result codes. */
typedef enum
{
    /**
     * @brief Success.
     *
     * The operation finished successfully.
     */
    PBHSR_OK,
    /**
     * @brief Element already added (by value).
     *
     * There was no need to add an element because it already exists in the hash
     * set.
     *
     * \b Important: Passed `element` should be released manually (if required).
     * @note For `PBHASH_SET_CHAR_CONTENT_TYPE` content type it means that the
     *       same string (by value) already added to the hash set.
     */
    PBHSR_VALUE_EXISTS,
    /**
     * @brief Element already added (by pointer).
     *
     * \b Warning: Don't release a passed `element` because it may have an
     * unexpected outcome at runtime.
     * @note This status can be returned even when
     *       `PBHASH_SET_CHAR_CONTENT_TYPE` set as hash set's content type.
     */
    PBHSR_EXACT_MATCH_EXISTS,
    /**
     * @brief Element not found.
     *
     * Referenced element not found in the hash set.
     */
    PBHSR_NOT_FOUND,
    /**
     * @brief Ran out of dynamic memory.
     *
     * The operation failed because there was not enough memory to allocate or
     * reallocate structures.
     */
    PBHSR_OUT_OF_MEMORY,
} pbhash_set_res;

/**
 * @brief Element `free` function prototype.
 *
 * Function which will be used when a hash set is freed using `pbhash_set_free`.
 *
 * @note Element destructor function will `NULL`ify provided pointer.
 */
typedef void (*pbhash_set_element_free)(void*);

/** Hash set definition. */
typedef struct pbhash_set pbhash_set_t;


// ----------------------------------
//             Functions
// ----------------------------------

/**
 * @brief Create an unordered hash set.
 *
 * \b Example:
 * @code
 * // Allocate hash for unique strings which will call `free_ptr()` on entry
 * // remove.
 * pbhash_set_t* set = pbhash_set_alloc(10, PBHASH_SET_CHAR_CONTENT_TYPE, free);
 * @endcode
 *
 * \b Important: Added elements should be the same type as specified in
 * `content_type` (if not set to the `PBHASH_SET_GENERIC_CONTENT_TYPE`).
 *
 * @param length       Maximum number of buckets managed by the hash set.
 * @param content_type Type of data which will be stored in a hash set.
 * @param free_fn      Elements `free` function. This value could be set to
 *                     `NULL` and leave memory management to the hash set user.
 * @return Pointer to the ready to use an unordered hash set or 'NULL' in case
 *         of insufficient memory error. The returned pointer must be passed to
 *         the `pbhash_set_free` or `pbhash_set_free_with_destructor` to avoid a
 *         memory leak.
 */
pbhash_set_t* pbhash_set_alloc(
    size_t length,
    pbhash_set_content_type content_type,
    pbhash_set_element_free free_fn);

/**
 * @brief Insert a new element to the had set if it doesn't exist in the set.
 * @code
 * // Storing "simple" types.
 * if (PBHSR_OK == pbhash_set_add(set, "String", NULL)) {
 *     // `String` value stored in hash.
 * }
 * @endcode
 * @code
 * // Storing structured types.
 * typedef struct {
 *     char* name;
 *     int age;
 * } user_t;
 *
 * user_t* user = malloc(sizeof(user_t));
 * user->name   = name;
 * user->age    = age;
 *
 * if (PBHSR_OK == pbhash_set_add(set, user->name, user)) {
 *     // `user` struct stored along with `name` which is used for hash
 *     // computation and elements match.
 *     // `PBHASH_SET_CHAR_CONTENT_TYPE` content type should be used when
 *     // creating a hash set.
 * }
 * @endcode
 * @code
 * char* my_value = malloc(6 * sizeof(char));
 * strcpy(my_value, "my-value");
 *
 * if (PBHSR_OK == pbhash_set_add(set, my_value, NULL)) {
 *     // `my_value` has been stored in a hash set.
 * }
 *
 * if (PBHSR_EXACT_MATCH_EXISTS == pbhash_set_add(set, my_value, NULL)) {
 *     // 'my-value' has been found in the hash set by value and pointer match.
 *     // There is no action required if `set` has been allocated with elements
 *     // destructor.
 * }
 *
 * // Allocate string with the same value as `my_value`.
 * char* my_new_value = malloc(6 * sizeof(char));
 * strcpy(my_new_value, my_value);
 *
 * if (PBHSR_VALUE_EXISTS == pbhash_set_add(set, my_new_value, NULL)) {
 *     // 'my-value' has been found in the hash set by value match.
 *     // free 'my_new_value' because it has been created (not received).
 *     free(my_new_value);
 * }
 * @endcode
 *
 * \b Warning: A hash set doesn't check `element` data type, and the mechanism
 * used to identify uniqueness may return unexpected results or cause a runtime
 * exception.
 *
 * @param set        Pointer to the hash set to which a new element should be
 *                   inserted (if not present already).
 * @param element    Pointer to the element, which should be inserted.
 * @param containing Pointer to the object which contains an `element`. `NULL`
 *                   can be used if non-structured data is stored.
 * @return Result of `element` addition to the hash set.
 */
pbhash_set_res pbhash_set_add(
    pbhash_set_t* set,
    void* element,
    void* containing);

/**
 * @brief Remove an element from a hash set.
 *
 * Remove value previously added to the hash set:
 * @code
 * pbhash_set_res result = pbhash_set_remove(set, my_value, NULL);
 * if (PBHSR_OK == result) {
 *     // `my_value` has been removed from a hash set.
 * } else if (PBHSR_NOT_FOUND == result) {
 *     // `my_value` not found in the has `set` by value (for
 *     // `PBHASH_SET_CHAR_CONTENT_TYPE` content type) or by address.
 * }
 * @endcode
 * Remove structured data previously added with unique key:
 * @code
 * // 'set' has been allocated with 'PBHASH_SET_CHAR_CONTENT_TYPE' content type.
 * typedef struct {
 *     char* name;
 *     int age;
 * } user_t;
 *
 * user_t* user = malloc(sizeof(user_t));
 * user->name   = name;
 * user->age    = age;
 * pbhash_set_add(set, user->name, user);
 *
 * // Remove structured data by `name`:
 * if (PBHSR_OK == pbhash_set_remove(set, &user->name, &user)) {
 *     // `user` has been removed from hash `set` using same field value which
 *     // has been used to add it.
 * }
 * @endcode
 *
 * \b Important: If provided during set allocation, the element destruct
 * function will be used for the matched element only if there are no other
 * references to it (not shared with other sets after `pbhash_set_union`).<br/>
 * \b Important: The `element` pointer will be NULLified if the memory address
 * and value of the `element` match the elements that have been freed
 * (no references).
 *
 * @param set        Pointer to the hash set from which new element should be
 *                   removed.
 * @param element    Pointer to the element which should be removed.
 * @param containing Pointer to the object which contains an `element`. `NULL`
 *                   can be used if non-structured data is stored.
 * @return Result of `element` removal from the hash set.
 *
 * @see pbhash_set_add
 * @see pbhash_set_union
 */
pbhash_set_res pbhash_set_remove(
    pbhash_set_t* set,
    void** element,
    void** containing);

/**
* @brief Union `other hash` set entries with source set.
 * @code
 * pbhash_set_t* set1 = pbhash_set_alloc(10, PBHASH_SET_CHAR_CONTENT_TYPE, NULL);
 * pbhash_set_add(set1, value_1, NULL);
 * pbhash_set_t* set2 = pbhash_set_alloc(10, PBHASH_SET_CHAR_CONTENT_TYPE, NULL);
 * pbhash_set_add(set2, value_2, NULL);
 *
 * // Merge `set2` into `set1`:
 * if (PBHSR_OUT_OF_MEMORY != pbhash_set_union(set1, set2, NULL)) {
 *     // `value_2` has been merged from `set2` into `set1`.
 * }
 * @endcode
 * Handling duplicated entries:
 * @code
 * pbhash_set_t* set1 = pbhash_set_alloc(10, PBHASH_SET_CHAR_CONTENT_TYPE, NULL);
 * pbhash_set_add(set1, value_1, NULL);
 * pbhash_set_t* set2 = pbhash_set_alloc(10, PBHASH_SET_CHAR_CONTENT_TYPE, NULL);
 * pbhash_set_add(set2, value_1, NULL);
 * pbhash_set_add(set2, value_2, NULL);
 *
 * // Merge `set2` into `set1`:
 * pbhash_set_t* duplicates;
 * if (PBHSR_OUT_OF_MEMORY != pbhash_set_union(set1, set2, &duplicates)) {
 *     // `value_2` has been merged from `set2` into `set1`.
 *     // `duplicates` set will be initialized with single value which already
 *     // existed in `set1`: `value_1`.
 * }
 * @endcode
 *
 * @note Hash set won't let free up (call element destructor) on elements which
 *       are shared with another set. Destructor will be applied only element
 *       not shared between sets anymore.<br/>\b Important: a hash set won't be
 *       able to protect data from direct release with `free`.
 *
 * @param set                  Pointer to the source hash set into which should
 *                             be merged entries from `other` set.
 * @param other_set            Pointer to the hash set with values which will be
 *                             added to the `set`.
 * @param [out] duplicates_set Parameter will hold a set of `other_set` elements
 *                             which already present in `set`.
 * @return Result of `other_hash` union with a source hash set.
 *
 * @see pbhash_set_add
 * @see pbhash_set_subtract
 */
pbhash_set_res pbhash_set_union(
    pbhash_set_t* set,
    pbhash_set_t* other_set,
    pbhash_set_t** duplicates_set);

/**
 * @brief Subtract `other hash` set entries from source set.
 * @code
 * // `pbhash_set_union(set1, set2, NULL)` or same elements from `set2` has been
 * // added to `set1` with `pbhash_set_add`.
 * //
 * // Removing `set2` entries from `set1`:
 * if (PBHSR_OK == pbhash_set_subtract(set1, set2)) {
 *     // All values of `set2` has been removed from `set1`.
 * }
 * @endcode
 *
 * @param set       Pointer to the source hash set from which entries from
 *                  `other` set should be removed.
 * @param other_set Pointer to the hash set with values which will be removed
 *                  from the `hash`.
 * @return Result of `other_hash` minus from a source hash set.
 *
 * @see pbhash_set_add
 * @see pbhash_set_union
 */
pbhash_set_res pbhash_set_subtract(pbhash_set_t* set, pbhash_set_t* other_set);

/**
 * @brief Retrieve `element` or containing it object.
 *
 * \b Important: The type of the `element` should match `content_type` used
 * during provided hash 'set' instantiation.
 *
 * @param set     Pointer to the hash set from which element should be returned.
 * @param element Pointer to the element which should be used for match.
 * @return Pointer to the structure which contains provided `element` as field
 *         value, element itself if stored as non-structured data or `NULL` in
 *         case if `element` can't be found.
 */
const void* pbhash_set_element(pbhash_set_t* set, const void* element);

/**
 * @brief Check whether `element` already added to the `set`.
 * @code
 * const char* value = "hello once";
 * pbhash_set_t* set = pbhash_set_alloc(10, PBHASH_SET_CHAR_CONTENT_TYPE, NULL);
 * char* once = malloc((strlen(value) + 1) * sizeof(char));
 * strcpy(once, value);
 * pbhash_set_add(set, value, NULL);
 *
 * // Check with stack `value`:
 * if (pbhash_set_contains(set, value)) {
 *     // `value` has been found in set by value (not address).
 * }
 *
 * // Check with value allocated on heap:
 * if (pbhash_set_contains(set, once)) {
 *     // `once` has been found in set by value (not address).
 * }
 * @endcode
 * Check with `PBHASH_SET_GENERIC_CONTENT_TYPE` has set content type:
 * @code
 * const char* value = "hello once";
 * pbhash_set_t* set = pbhash_set_alloc(10, PBHASH_SET_GENERIC_CONTENT_TYPE, NULL);
 * char* once = malloc((strlen(value) + 1) * sizeof(char));
 * strcpy(once, value);
 * pbhash_set_add(set, value, NULL);
 *
 * // Check with stack `value`:
 * if (pbhash_set_contains(set, value)) {
 *     // `value` has been found by address.
 * }
 *
 * // Check with value allocated on heap:
 * if (!pbhash_set_contains(set, once)) {
 *     // `once` has the same value as `once` but has different address and value
 *     // with this address not added into the hash `set`.
 * }
 * @endcode
 *
 * \b Important: The type of the `element` should match `content_type` used
 * during provided hash 'set' instantiation.
 *
 * @param set     Pointer to the hash set, which should be checked for `element`
 *                presence.
 * @param element Pointer to the element which should be checked.
 * @return `true` in case if `element` has been found in the hash set.
 *
 * @see pbhash_set_add
 */
bool pbhash_set_contains(pbhash_set_t* set, const void* element);

/**
 * @brief Check whether `set` content is equal to the other set content.
 *
 * @note Check will be done only by value (not `containing` object).
 *
 * @param set       Pointer to the source hash set from which content will be
 *                  compared to the other set.
 * @param other_set Pointer to the hash set with values which will be used
 *                  in comparison with the `hash`.
 * @return `true` if both sets has similar content type, length and set of
 *         objects.
 */
bool pbhash_set_is_equal(pbhash_set_t* set, pbhash_set_t* other_set);

/**
 * @brief Identify type of the `element` match in the `set`.
 * @code
 * const char* value = "hello once";
 * pbhash_set_t* set = pbhash_set_alloc(10, PBHASH_SET_CHAR_CONTENT_TYPE, NULL);
 * char* once = malloc((strlen(value) + 1) * sizeof(char));
 * strcpy(once, value);
 * pbhash_set_add(set, value, NULL);
 *
 * // Check with stack `value`:
 * if (PBHSR_VALUE_EXISTS == pbhash_set_match_element(set, value)) {
 *     // `value` is not exact data (by address).
 * }
 *
 * // Check with value allocated on heap:
 * if (PBHSR_EXACT_MATCH_EXISTS == pbhash_set_contains_exact(set, once)) {
 *     // `once` is exact data (by address).
 * }
 * @endcode
 *
 * \b Important: The type of the `element` should match `content_type` used
 * during provided hash 'set' instantiation.
 *
 * @param set     Pointer to the hash set, which should be checked for `element`
 *                presence.
 * @param element Pointer to the element which should be checked.
 * @return `PBHSR_NOT_FOUND` in case if `element` not found in the hash set.
 *
 * @see pbhash_set_add
 */
pbhash_set_res pbhash_set_match_element(
    pbhash_set_t* set,
    const void* element);

/**
 * @brief Retrieve a type of data stored in hash `set`.
 *
 * @param set Pointer to the hash set for which content type will be returned.
 * @return One of `pbhash_set_content_type` enum fields.
 *
 * @see pbhash_set_content_type
 */
pbhash_set_content_type pbhash_set_type(const pbhash_set_t* set);

/**
 * @brief Get the number of elements in hash set.
 * @code
 * pbhash_set_t* set = pbhash_set_alloc(10, PBHASH_SET_CHAR_CONTENT_TYPE, NULL);
 * pbhash_set_remove(set, "hello once", NULL);
 * pbhash_set_remove(set, "hello twice", NULL);
 * // Output: Number of elements: 2
 * printf("Number of elements: %d\n", pbhash_set_count(set));
 * @endcode
 *
 * @param set Pointer to the hash set for which number of elements should be
 *            retrieved.
 * @return Number of elements stored in the provided hash set.
 */
size_t pbhash_set_count(pbhash_set_t* set);

/**
 * @brief Retrieve elements stored in the set.
 *
 * @param set         Pointer to the hash set from which elements should be retrieved.
 * @param [out] count Parameter will hold the count of returned elements.
 * @return Pointer to the array of pointers to the elements, or `NULL` in case
 *         of insufficient memory error. The returned pointer must be passed to
 *         the `free` to avoid a memory leak.
 */
const void** pbhash_set_elements(pbhash_set_t* set, size_t* count);

/**
 * @brief Remove all hash set entries.
 * @param set Pointer to the hash set from which all entries should be removed.
 */
void pbhash_set_remove_all(pbhash_set_t* set);

/**
 * @brief Clean up resources used by `set`.
 *
 * @note User is responsible for reclaiming resources used by the elements if
 *       element destructing function is not provided. Destructing function will
 *       be used on `containing` object if it has been provided along with added
 *       `element`.
 * @note Function will NULLify provided array pointer.
 *
 * @param set Pointer to the hash set, which should free up resources used for
 *            it and stored elements.
 */
void pbhash_set_free(pbhash_set_t** set);

/**
 * @brief Clean up resources used by `set` with custom element destructor.
 *
 * @note Function will NULLify provided array pointer.
 *
 * @param set     Pointer to the hash set, which should free up resources used
 *                for it and stored elements.
 * @param free_fn Elements `free` function. This value could be set to `NULL`
 *                and leave memory management to the hash set user.
 */
void pbhash_set_free_with_destructor(
    pbhash_set_t** set,
    pbhash_set_element_free free_fn);
#endif // #ifndef PBHASH_SET_H
