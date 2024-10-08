/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pbhash_set.h"

#include <stdio.h>
#include <string.h>

#include "core/pubnub_mutex.h"
#include "lib/pbref_counter.h"


// ----------------------------------
//              Types
// ----------------------------------

typedef struct pbhash_set_node {
    /**
     * @brieg Optionally, could be a pointer to the structure which may contain
     *        `value`.
     */
    void* containing;
    /** Pointer to the current node value. */
    void* value;
    /** Object references counter. */
    pbref_counter_t* counter;
    /** Pointer to the next node with similar hash value. */
    struct pbhash_set_node* next;
    /** Shared resources access lock. */
    pubnub_mutex_t mutw;
} pbhash_set_node_t;

/** Hash set definition. */
struct pbhash_set {
    /** Type of data which will be stored in a hash set. */
    pbhash_set_content_type content_type;
    /** Pointer to the list of node pointers which hold values. */
    pbhash_set_node_t** buckets;
    /** A number of buckets can be maintained by hash set. */
    size_t bucket_length;
    /** How many currently stored in a hash set. */
    size_t elements_count;
    /** Element entries destructor. */
    pbhash_set_element_free free;
    /** Shared resources access lock. */
    pubnub_mutex_t mutw;
};


// ----------------------------------
//        Function prototypes
// ----------------------------------

/**
 * @brief Free up resources used by hash set node.
 *
 * @param set  Pointer to the hash set, from which node should be removed.
 * @param node Pointer to the node which should be freed.
 * @return `true` if node's value has been freed.
 */
static bool pbhash_set_free_node_(
    const pbhash_set_t* set,
    pbhash_set_node_t*  node);

/**
 * @brief Add hash set node into the bucket.
 *
 * @note New node with state of origin `node` will be created.
 *
 * @param set          Pointer to the hash set, into which new node should be
 *                     added.
 * @param node         Pointer to the hash set node with value to add into the
 *                     bucket.
 * @param element_hash Computed hash of bucket to store `element` in it.
 * @param clone        Whether `node` clone should be inserted into `set` or
 *                     not.
 * @return Result of `node` addition to the hash set.
 */
static pbhash_set_res pbhash_set_add_node_(
    pbhash_set_t*      set,
    pbhash_set_node_t* node,
    size_t             element_hash,
    bool               clone);

/**
 * @brief Quick hash compute for the `element`.
 *
 * @param set     Pointer to the hash set which maybe used to store provided
 *                `element`.
 * @param element Pointer to the element for which hash should be computed.
 * @return Content type-dependant `element` hash value.
 */
static size_t pbhash_set_element_hash_(
    const pbhash_set_t* set,
    const void*         element);

/**
 * @brief Check whether `element` already added to the `set`.
 *
 * \b Important: The type of the `element` should match `content_type` used
 * during provided hash 'set' instantiation.
 *
 * @param set          Pointer to the hash set, which should be checked for
 *                     `element` presence.
 * @param element      Pointer to the element which should be checked.
 * @param element_hash Computed hash of bucket to store `element` in it.
 * @return `PBHSR_NOT_FOUND` in case if `element` not found in the hash set.
 */
static pbhash_set_res pbhash_set_contains_(
    const pbhash_set_t* set,
    const void*         element,
    size_t              element_hash);

/**
 * @brief Identify type of the `element` match in the `set`.
 *
 * @param set     Pointer to the hash set, which should be checked for `element`
 *                presence.
 * @param element Pointer to the element which should be checked.
 * @return `PBHSR_NOT_FOUND` in case if `element` not found in the hash set.
 *
 * @see pbhash_set_add
 */
static pbhash_set_res pbhash_set_match_element_(
    const pbhash_set_t* set,
    const void*         element);

/**
 * @brief Check whether an element from hash `set` matches the other element.
 *
 * \b Important: The types of elements should match `content_type` used during
 * provided hash `set` instantiation.
 *
 * @param set      Pointer to the hash set, for which two elements should be
 *                 compared.
 * @param element1 Pointer to the left-hand element which should be used in
 *                 compare.
 * @param element2 Pointer to the right-hand element which should be used in
 *                 compare.
 * @return `true` in case if two elements match each other.
 */
static bool pbhash_set_is_equal_elements_(
    const pbhash_set_t* set,
    const void*         element1,
    const void*         element2);


// ----------------------------------
//             Functions
// ----------------------------------

pbhash_set_t* pbhash_set_alloc(
    const size_t                  length,
    const pbhash_set_content_type content_type,
    const pbhash_set_element_free free_fn)
{
    pbhash_set_t* set = malloc(sizeof(pbhash_set_t));
    if (NULL == set) { return NULL; }

    set->buckets = calloc(length, sizeof(pbhash_set_node_t*));
    if (NULL == set->buckets) {
        free(set);
        return NULL;
    }

    pubnub_mutex_init(set->mutw);
    set->content_type   = content_type;
    set->bucket_length  = length;
    set->elements_count = 0;
    set->free           = free_fn;

    return set;
}

pbhash_set_res pbhash_set_add(
    pbhash_set_t* set,
    void*         element,
    void*         containing)
{
    pubnub_mutex_lock(set->mutw);
    const size_t   hash = pbhash_set_element_hash_(set, element);
    pbhash_set_res rslt = pbhash_set_contains_(set, element, hash);

    if (PBHSR_NOT_FOUND != rslt) {
        pubnub_mutex_unlock(set->mutw);
        return rslt;
    }

    pbhash_set_node_t* node = malloc(sizeof(pbhash_set_node_t));
    if (NULL == node) {
        pubnub_mutex_unlock(set->mutw);
        return PBHSR_OUT_OF_MEMORY;
    }

    pubnub_mutex_init(node->mutw);
    node->counter    = pbref_counter_alloc();
    node->containing = containing;
    node->value      = element;
    node->next       = NULL;
    rslt             = pbhash_set_add_node_(set, node, hash, false);
    pubnub_mutex_unlock(set->mutw);

    return rslt;
}

pbhash_set_res pbhash_set_remove(
    pbhash_set_t* set,
    void**        element,
    void**        containing)
{
    pubnub_mutex_lock(set->mutw);
    if (NULL == element || NULL == *element || 0 == set->elements_count) {
        pubnub_mutex_unlock(set->mutw);
        return PBHSR_NOT_FOUND;
    }

    const size_t hash = pbhash_set_element_hash_(set, *element);
    pbhash_set_node_t* node = set->buckets[hash];
    pbhash_set_node_t* prev = NULL;

    while (NULL != node) {
        if (pbhash_set_is_equal_elements_(set, node->value, *element)) {
            if (NULL == prev) { set->buckets[hash] = node->next; }
            else { prev->next = node->next; }
            if (pbhash_set_free_node_(set, node)) {
                if (NULL != containing) { *containing = NULL; }
                else { *element = NULL; }
            }
            set->elements_count--;
            pubnub_mutex_unlock(set->mutw);
            return PBHSR_OK;
        }

        prev = node;
        node = node->next;
    }
    pubnub_mutex_unlock(set->mutw);

    return PBHSR_NOT_FOUND;
}

pbhash_set_res pbhash_set_union(
    pbhash_set_t*  set,
    pbhash_set_t*  other_set,
    pbhash_set_t** duplicates_set)
{
    pbhash_set_res rslt = PBHSR_OK;

    if (NULL != duplicates_set) {
        *duplicates_set = pbhash_set_alloc(set->bucket_length,
                                           set->content_type,
                                           NULL);
    }

    pubnub_mutex_lock(set->mutw);
    pubnub_mutex_lock(other_set->mutw);
    for (int i = 0; i < other_set->bucket_length; ++i) {
        pbhash_set_node_t* node = other_set->buckets[i];
        while (NULL != node && PBHSR_OUT_OF_MEMORY != rslt) {
            const size_t hash = pbhash_set_element_hash_(set, node->value);
            if (PBHSR_NOT_FOUND ==
                pbhash_set_contains_(set, node->value, hash)) {
                rslt = pbhash_set_add_node_(set, node, hash, true);
            }
            else if (NULL != duplicates_set) {
                pbhash_set_add(*duplicates_set, node->value, node->containing);
            }

            node = node->next;
        }
    }
    pubnub_mutex_unlock(other_set->mutw);
    pubnub_mutex_unlock(set->mutw);

    return rslt;
}

pbhash_set_res pbhash_set_subtract(
    pbhash_set_t* set,
    pbhash_set_t* other_set)
{
    pbhash_set_res rslt = PBHSR_OK;

    pubnub_mutex_lock(other_set->mutw);
    for (int i = 0; i < other_set->bucket_length; ++i) {
        const pbhash_set_node_t* node = other_set->buckets[i];
        while (NULL != node) {
            const pbhash_set_node_t* next = node->next;
            rslt                          = pbhash_set_remove(set,
                (void**)&node->value,
                (void**)&node->containing);
            node = next;
        }
    }
    pubnub_mutex_unlock(other_set->mutw);

    return rslt;
}

const void* pbhash_set_element(pbhash_set_t* set, const void* element)
{
    const void* matched_element = element;
    pubnub_mutex_lock(set->mutw);
    const size_t             hash  = pbhash_set_element_hash_(set, element);
    const pbhash_set_node_t* node  = set->buckets[hash];
    bool                     found = false;

    while (NULL != node) {
        if (pbhash_set_is_equal_elements_(set, node->value, element)) {
            if (NULL != node->containing) {
                matched_element = node->containing;
            }
            found = true;
            break;
        }

        node = node->next;
    }
    pubnub_mutex_unlock(set->mutw);
    if (!found) { matched_element = NULL; }

    return matched_element;
}

bool pbhash_set_contains(pbhash_set_t* set, const void* element)
{
    pubnub_mutex_lock(set->mutw);
    const size_t hash     = pbhash_set_element_hash_(set, element);
    const bool   contains = PBHSR_NOT_FOUND != pbhash_set_contains_(
                                set,
                                element,
                                hash);
    pubnub_mutex_unlock(set->mutw);

    return contains;
}

bool pbhash_set_is_equal(pbhash_set_t* set, pbhash_set_t* other_set)
{
    pubnub_mutex_lock(set->mutw);
    pubnub_mutex_lock(other_set->mutw);
    bool equal = set->content_type == other_set->content_type;
    equal      = equal && set->elements_count == other_set->elements_count;
    if (equal) {
        size_t       count;
        const void** elements = pbhash_set_elements(set, &count);
        for (size_t i = 0; i < count; ++i) {
            equal = pbhash_set_contains(other_set, elements[i]);
            if (!equal) { break; }
        }
        free(elements);
    }
    pubnub_mutex_unlock(other_set->mutw);
    pubnub_mutex_unlock(set->mutw);

    return equal;
}

pbhash_set_res pbhash_set_match_element(
    pbhash_set_t* set,
    const void*   element)
{
    pubnub_mutex_lock(set->mutw);
    const pbhash_set_res rslt = pbhash_set_match_element_(set, element);
    pubnub_mutex_unlock(set->mutw);

    return rslt;
}

pbhash_set_content_type pbhash_set_type(
    const pbhash_set_t* set)
{
    return set->content_type;
}

size_t pbhash_set_count(pbhash_set_t* set)
{
    pubnub_mutex_lock(set->mutw);
    const size_t count = set->elements_count;
    pubnub_mutex_unlock(set->mutw);

    return count;
}

const void** pbhash_set_elements(pbhash_set_t* set, size_t* count)
{
    pubnub_mutex_lock(set->mutw);
    if (NULL != count) { *count = set->elements_count; }

    const void** elements = calloc(
        0 == set->elements_count ? 1 : set->elements_count,
        sizeof(void*));
    if (NULL == elements || 0 == set->elements_count) {
        pubnub_mutex_unlock(set->mutw);
        return elements;
    }

    int element_index = 0;

    for (int i = 0; i < set->bucket_length; ++i) {
        const pbhash_set_node_t* node = set->buckets[i];
        while (NULL != node) {
            elements[element_index++] = NULL != node->containing
                                            ? node->containing
                                            : node->value;
            node = node->next;
        }
    }
    pubnub_mutex_unlock(set->mutw);

    return elements;
}

void pbhash_set_remove_all(pbhash_set_t* set)
{
    pubnub_mutex_lock(set->mutw);
    for (size_t i = 0; i < set->bucket_length; ++i) {
        pbhash_set_node_t* node = set->buckets[i];
        while (NULL != node) {
            pbhash_set_node_t* next = node->next;
            pbhash_set_free_node_(set, node);
            node = next;
        }
    }
    memset(set->buckets, 0, set->bucket_length * sizeof(pbhash_set_node_t *));
    set->elements_count = 0;
    pubnub_mutex_unlock(set->mutw);
}

void pbhash_set_free(pbhash_set_t** set)
{
    if (NULL == set || NULL == *set) { return; }

    pbhash_set_free_with_destructor(set, (*set)->free);
}

void pbhash_set_free_with_destructor(
    pbhash_set_t**                set,
    const pbhash_set_element_free free_fn)
{
    if (NULL == set || NULL == *set) { return; }

    pubnub_mutex_lock((*set)->mutw);
    if (NULL != free_fn) { (*set)->free = free_fn; }
    for (size_t i = 0; i < (*set)->bucket_length; ++i) {
        pbhash_set_node_t* node = (*set)->buckets[i];
        while (NULL != node) {
            pbhash_set_node_t* next = node->next;
            pbhash_set_free_node_(*set, node);
            node = next;
        }
    }

    free((*set)->buckets);
    pubnub_mutex_unlock((*set)->mutw);
    pubnub_mutex_destroy((*set)->mutw);
    free(*set);
    *set = NULL;
}

pbhash_set_res pbhash_set_match_element_(
    const pbhash_set_t* set,
    const void*         element)
{
    const size_t hash = pbhash_set_element_hash_(set, element);
    return pbhash_set_contains_(set, element, hash);
}

bool pbhash_set_free_node_(const pbhash_set_t* set, pbhash_set_node_t* node)
{
    if (NULL == node) { return true; }

    bool removed = false;

    /** Ensure that node doesn't have any other references before `free`. */
    if (0 == pbref_counter_free(node->counter)) {
        if (NULL != set->free) {
            if (node->containing) { set->free(node->containing); }
            else { set->free(node->value); }
            removed = true;
        }
    }

    free(node);
    return removed;
}

pbhash_set_res pbhash_set_add_node_(
    pbhash_set_t*      set,
    pbhash_set_node_t* node,
    const size_t       element_hash,
    const bool         clone)
{
    pbhash_set_node_t* node_to_add = node;

    if (clone) {
        node_to_add = malloc(sizeof(pbhash_set_node_t));
        if (NULL == node_to_add) { return PBHSR_OUT_OF_MEMORY; }

        /** Sharing source data with a copy. */
        pubnub_mutex_init(node_to_add->mutw);
        node_to_add->counter    = node->counter;
        node_to_add->containing = node->containing;
        node_to_add->value      = node->value;
        node_to_add->next       = NULL;
        pbref_counter_increment(node_to_add->counter);
    }

    node_to_add->next          = set->buckets[element_hash];
    set->buckets[element_hash] = node_to_add;
    set->elements_count++;

    return PBHSR_OK;
}

size_t pbhash_set_element_hash_(const pbhash_set_t* set, const void* element)
{
    size_t hash = 0;

    if (set->content_type == PBHASH_SET_GENERIC_CONTENT_TYPE) {
        hash = (size_t)element % set->bucket_length;
    }
    else if (set->content_type == PBHASH_SET_CHAR_CONTENT_TYPE) {
        const size_t bucket_count = set->bucket_length;
        const char*  str          = element;
        while (*str) {
            hash = (hash * 31 + *str++) % bucket_count;
        }
    }

    return hash;
}

pbhash_set_res pbhash_set_contains_(
    const pbhash_set_t* set,
    const void*         element,
    const size_t        element_hash)
{
    const pbhash_set_node_t* node = set->buckets[element_hash];

    while (NULL != node) {
        if (pbhash_set_is_equal_elements_(set, node->value, element)) {
            if (node->value == element) { return PBHSR_EXACT_MATCH_EXISTS; }
            return PBHSR_VALUE_EXISTS;
        }

        node = node->next;
    }

    return PBHSR_NOT_FOUND;
}

bool pbhash_set_is_equal_elements_(
    const pbhash_set_t* set,
    const void*         element1,
    const void*         element2)
{
    if (set->content_type == PBHASH_SET_CHAR_CONTENT_TYPE) {
        return strcmp(element1, element2) == 0;
    }

    return element1 == element2;
}