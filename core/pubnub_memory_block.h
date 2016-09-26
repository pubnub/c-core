/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_MEMORY_BLOCK
#define	INC_PUBNUB_MEMORY_BLOCK

#include <stdint.h>
#include <stdlib.h>


/** @file pubnub_memory_block.h

    Memory block module.

    Support module for having a standard way to represent a 
    "block" of memory - that is, a pointer and the size of
    memory allocated (in whatever way) to said pointer.
 */


/** A block of memory whose pointer is pointing to a
    bytes. This is most often the preferred one to use.
*/
struct pubnub_byte_mem_block {
    uint8_t *ptr;
    size_t size;
};

/** A block of memory whose pointer is void pointer.
    Not nice, but useful at times.
 */
struct pubnub_mem_block {
    void *ptr;
    size_t size;
};

/** Helper typedef for a general (void) memory block */
typedef struct pubnub_mem_block pubnub_mebl_t;

/** Helper typedef for a byte memory block */
typedef struct pubnub_byte_mem_block pubnub_bymebl_t;


#endif /* !defined INC_PUBNUB_MEMORY_BLOCK */
