/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#ifndef PUBNUB_ENTITIES_H
#define PUBNUB_ENTITIES_H
// Currently, entities is part of the subscribe event engine feature.
#if PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE


/**
 * @file  pubnub_entities.h
 * @brief Module to manage first-class types.
 *
 * Entities used to access contextual endpoints related to a specific entity.
 */

#include <stdbool.h>

#include "core/pubnub_api_types.h"
#include "lib/pb_extern.h"


// ----------------------------------
//              Types
// ----------------------------------

/** PubNub Channel entity definition. */
typedef struct pubnub_channel pubnub_channel_t;

/** PubNub Channel Group entity definition. */
typedef struct pubnub_channel_group pubnub_channel_group_t;

/** PubNub Channel Metadata entity (App Context) definition. */
typedef struct pubnub_channel_metadata pubnub_channel_metadata_t;

/** PubNub User Metadata entity (App Context) definition. */
typedef struct pubnub_user_metadata pubnub_user_metadata_t;

/** PubNub Entity definition. */
typedef struct pubnub_entity pubnub_entity_t;


// ----------------------------------
//             Functions
// ----------------------------------

/**
 * @brief Create channel entity.
 *
 * @code
 * pubnub_channel_t* channel = pubnub_channel_alloc(pb, "my_channel");
 * @endcode
 *
 * @param pb   Pointer to the PubNub context, which will be used by the created
 *             entity to interact with PubNub REST API.
 * @param name Pointer to the name which will be used with channel-specific
 *             REST API.
 * @return Pointer to the ready to use PubNub Channel entity. The returned
 *         pointer must be passed to the `pubnub_entity_free` to avoid a memory
 *         leak.
 */
PUBNUB_EXTERN pubnub_channel_t* pubnub_channel_alloc(
    pubnub_t*   pb,
    const char* name);

/**
 * @brief Create channel group entity.
 *
 * @code
 * pubnub_channel_group_t* group = pubnub_channel_group_alloc(pb, "my_group");
 * @endcode
 *
 * @note `name` will be copied.
 *
 * @param pb   PubNub context, which will be used by the created entity to
 *             interact with PubNub REST API.
 * @param name Identifier which will be used with channel group-specific
 *             REST API.
 * @return Pointer to the ready to use PubNub Channel Group entity. The returned
 *         pointer must be passed to the `pubnub_entity_free` to avoid a memory
 *         leak.
 */
PUBNUB_EXTERN pubnub_channel_group_t* pubnub_channel_group_alloc(
    pubnub_t*   pb,
    const char* name);

/**
 * @brief Create channel metadata entity.
 *
 * @code
 * pubnub_channel_metadata_t* metadata = pubnub_channel_metadata_alloc(pb, "channel_meta");
 * @endcode
 *
 * @note `id` will be copied.
 *
 * @param pb PubNub context, which will be used by the created entity to
 *           interact with PubNub REST API.
 * @param id Identifier which will be used with channel metadata specific
 *           REST API.
 * @return Ready to use PubNub Channel Metadata entity. The returned
 *         pointer must be passed to the `pubnub_entity_free` to avoid a memory
 *         leak.
 */
PUBNUB_EXTERN pubnub_channel_metadata_t* pubnub_channel_metadata_alloc(
    pubnub_t*   pb,
    const char* id);

/**
 * @brief Create user metadata entity.
 *
 * @code
 * pubnub_user_metadata_t* metadata = pubnub_user_metadata_alloc(pb, "user_meta");
 * @endcode
 *
 * @note `id` will be copied.
 *
 * @param pb PubNub context, which will be used by the created entity to
 *           interact with PubNub REST API.
 * @param id Identifier which will be used with user metadata specific REST API.
 * @return Ready to use PubNub User Metadata entity. The returned
 *         pointer must be passed to the `pubnub_entity_free` to avoid a memory
 *         leak.
 */
PUBNUB_EXTERN pubnub_user_metadata_t* pubnub_user_metadata_alloc(
    pubnub_t*   pb,
    const char* id);

/**
 * @brief Release resources used by PubNub entity.
 *
 * @code
 * pubnub_channel_group_t* group = pubnub_channel_group_alloc(pb, "my_group");
 * // ...
 * pubnub_entity_free(&group);
 * @endcode
 *
 * \b Warning: Make sure that calls to the `pubnub_entity_free` is done only
 * once to not disrupt other types which still use this entity.
 *
 * @note Function will `NULL`ify provided entity pointer (if there is no more
 *       references to the entity).
 *
 * @param entity PubNub entity, which should free up resources.
 * @return `false` if there are more references to the entity.
 */
PUBNUB_EXTERN bool pubnub_entity_free(void** entity);
#else // #if PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE
#error To use entities API you must define PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE=1
#endif // #if PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE
#endif // #ifndef PUBNUB_ENTITIES_H