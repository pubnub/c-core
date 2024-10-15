/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#ifndef PBREF_COUNTER_H
#define PBREF_COUNTER_H

/**
 * @file pbref_counter.h
 *
 * Thread-safe reference counter.
 * The counter facilitates the process of transferring an object between various
 * structures and ensures that it will be released only after all references to
 * it have disappeared.
 */

#include <stdlib.h>


// ----------------------------------
//              Types
// ----------------------------------

// Reference counter type definition.
typedef struct pbref_counter pbref_counter_t;


// ----------------------------------
//             Functions
// ----------------------------------

/**
 * @brief Create a shared resource reference counter.
 *
 * @note Usually, the reference counter allocated along with the object for
 *       which references should be counted and, for simplicity, will be set
 *       to \b 1.
 *
 * @return Pointer to the resource reference counter object.
 */
pbref_counter_t* pbref_counter_alloc(void);

/**
 * @brief Increase number of references.
 *
 * @param counter Pointer to the counter, which should increase the number of
 *                references to the shared resource.
 * @return Current number of references.
 */
size_t pbref_counter_increment(pbref_counter_t* counter);

/**
 * @brief Decrease number of references.
 *
 * @param counter Pointer to the counter, which should decrease the number of
 *                references to the shared resource.
 * @return Current number of references.
 */
size_t pbref_counter_decrement(pbref_counter_t* counter);

/**
 * @brief Clean up resources used by reference counter object.
 *
 * \b Important: This function will try release reference counter resources but
 * if number of references didn't reached \b 0 it will only references count.
 *
 * @param counter Pointer to the reference counter object, which should free up
 *                resources.
 * @return Current number of references.
 */
size_t pbref_counter_free(pbref_counter_t* counter);
#endif // #ifndef PBREF_COUNTER_H