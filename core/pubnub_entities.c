/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_entities.h"

#include <stdlib.h>

#include "core/pubnub_entities_internal.h"
#include "core/pbcc_memory_utils.h"
#include "core/pubnub_assert.h"
#include "core/pubnub_log.h"
#include "pubnub_internal.h"
#include "lib/pbstrdup.h"


// ----------------------------------
//        Function prototypes
// ----------------------------------

/**
 * @brief Create a specific PubNub entity.
 *
 * @param pb   Pointer to the PubNub context, which will be used by the created
 *             entity to interact with PubNub REST API.
 * @param id   Pointer to the identifier which will be used with entity-specific
 *             REST API.
 * @param type Specific PubNub entity type.
 * @return Ready to use PubNub entity. The returned pointer must be passed to
 *         the `pubnub_entity_free` to avoid a memory leak.
 */
static pubnub_entity_t* create_entity_(
    pubnub_t*          pb,
    const char*        id,
    pubnub_entity_type type);

/**
 * @brief Check whether the passed value is one of known PubNub entity types.
 *
 * @param entity Pointer to the value which should be checked to be PubNub
 *               entity.
 * @return `true` if passed `entity` is one of PubNub entities.
 */
static bool is_pubnub_entity_(void* entity);


// ----------------------------------
//             Functions
// ----------------------------------

pubnub_channel_t* pubnub_channel_alloc(
    pubnub_t*   pb,
    const char* name)
{
    return (pubnub_channel_t*)create_entity_(pb, name, PUBNUB_ENTITY_CHANNEL);
}

pubnub_channel_group_t* pubnub_channel_group_alloc(
    pubnub_t*   pb,
    const char* name)
{
    return (pubnub_channel_group_t*)create_entity_(
        pb,
        name,
        PUBNUB_ENTITY_CHANNEL_GROUP);
}

pubnub_channel_metadata_t* pubnub_channel_metadata_alloc(
    pubnub_t*   pb,
    const char* id)
{
    return (pubnub_channel_metadata_t*)create_entity_(
        pb,
        id,
        PUBNUB_ENTITY_CHANNEL_METADATA);
}

pubnub_user_metadata_t* pubnub_user_metadata_alloc(
    pubnub_t*   pb,
    const char* id)
{
    return (pubnub_user_metadata_t*)create_entity_(
        pb,
        id,
        PUBNUB_ENTITY_USER_METADATA);
}

bool pubnub_entity_free(void** entity)
{
    if (NULL == entity || NULL == *entity) { return false; }

    PUBNUB_ASSERT_OPT(true == is_pubnub_entity_(*entity));

    pubnub_entity_t* _entity = *entity;
    pubnub_mutex_lock(_entity->mutw);
    if (0 == pbref_counter_free(_entity->counter)) {
        free(_entity->id.ptr);
        pubnub_mutex_unlock(_entity->mutw);
        pubnub_mutex_destroy(_entity->mutw);
        free(*entity);
        *entity = NULL;
        return true;
    }
    pubnub_mutex_unlock(_entity->mutw);

    return false;
}

pubnub_entity_t* create_entity_(
    pubnub_t*                pb,
    const char*              id,
    const pubnub_entity_type type)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_ASSERT_OPT(NULL != id);

    PBCC_ALLOCATE_TYPE(entity, pubnub_entity_t, true, NULL);
    entity->id.size = strlen(id);
    entity->id.ptr  = pbstrndup(id, entity->id.size);
    if (NULL == entity->id.ptr) {
        PUBNUB_LOG_ERROR(
            "create_entity: failed to allocate memory for entity id\n");
        free(entity);
        return NULL;
    }

    pubnub_mutex_init(entity->mutw);
    entity->counter = pbref_counter_alloc();
    entity->type    = type;
    entity->pb      = pb;

    return entity;
}

bool is_pubnub_entity_(void* entity)
{
    if (NULL == entity) { return false; }

    const pubnub_entity_type type = *(pubnub_entity_type*)entity;

    return type == PUBNUB_ENTITY_CHANNEL || type == PUBNUB_ENTITY_CHANNEL_GROUP
           || type == PUBNUB_ENTITY_CHANNEL_METADATA
           || type == PUBNUB_ENTITY_USER_METADATA;
}

void entity_reference_count_update_(
    pubnub_entity_t* entity,
    const bool       increase)
{
    PUBNUB_ASSERT_OPT(NULL != entity);

    pubnub_mutex_lock(entity->mutw);
    if (increase) { pbref_counter_increment(entity->counter); }
    else { pbref_counter_decrement(entity->counter); }
    pubnub_mutex_unlock(entity->mutw);
}