#ifndef PUBNUB_ENTITIES_INTERNAL_H
#define PUBNUB_ENTITIES_INTERNAL_H
// Currently, entities is part of the subscribe event engine feature.
#if PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE

/**
 * @file  pubnub_entities_internal.h
 * @brief Module to manage first-class types internal / lib interface.
 */

#include "core/pubnub_memory_block.h"
#include "core/pubnub_api_types.h"
#if PUBNUB_THREADSAFE
#include "core/pubnub_mutex.h"
#endif

// ----------------------------------
//              Types
// ----------------------------------

// Known PubNub entities.
typedef enum {
    PUBNUB_ENTITY_CHANNEL,
    PUBNUB_ENTITY_CHANNEL_GROUP,
    PUBNUB_ENTITY_CHANNEL_METADATA,
    PUBNUB_ENTITY_USER_METADATA,
} pubnub_entity_type;

// PubNub Channel entity definition.
struct pubnub_channel {
    // Channel entity type (`PUBNUB_ENTITY_CHANNEL`).
    pubnub_entity_type type;
    // Identifier which will be used with channel-specific REST API.
    struct pubnub_char_mem_block name;
};

// PubNub Channel Group entity definition.
struct pubnub_channel_group {
    // Channel group entity type (`PUBNUB_ENTITY_CHANNEL_GROUP`).
    pubnub_entity_type type;
    // Identifier which will be used with channel group-specific REST API.
    struct pubnub_char_mem_block name;
};

// PubNub Channel Metadata entity (App Context) definition.
struct pubnub_channel_metadata {
    // Channel metadata entity type (`PUBNUB_ENTITY_CHANNEL_METADATA`).
    pubnub_entity_type type;
    // Identifier which will be used with channel metadata specific REST API.
    struct pubnub_char_mem_block id;
};

// PubNub User Metadata entity (App Context) definition.
struct pubnub_user_metadata {
    // User metadata entity type (`PUBNUB_ENTITY_USER_METADATA`).
    pubnub_entity_type type;
    // Identifier which will be used with user metadata specific REST API.
    struct pubnub_char_mem_block id;
};

// PubNub Entity definition.
struct pubnub_entity {
    // PubNub entity type.
    pubnub_entity_type type;
    // Entity identifier.
    struct pubnub_char_mem_block id;
    /**
     * @brief PubNub context, which will be used by the created entity to
     *        interact with PubNub REST API
     */
    pubnub_t* pb;
    /**
     * @brief How many subscriptions have reference to this entity.
     *
     * \b Important: `pubnub_entity_free` won't do anything if value is
     * larger than 0.
     */
    int reference_count;
#if PUBNUB_THREADSAFE
    // Shared resources access lock.
    pubnub_mutex_t mutw;
#endif
};


// ----------------------------------
//             Functions
// ----------------------------------

/**
 * @brief Update number of subscriptions which refer to PubNub `entity`.
 *
 * @param entity   PubNub entity for which number of subscription references
 *                 should be updated.
 * @param increase Whether reference count should be increased or decreased.
 */
void _entity_reference_count_update(pubnub_entity_t* entity, bool increase);

#else
#error To use entities API you must define PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE=1
#endif //PUBNUB_USE_SUBSCRIBE_EVENT_ENGINE
#endif //PUBNUB_ENTITIES_INTERNAL_H