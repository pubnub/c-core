/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_log_value.h"

#include <string.h>


// ----------------------------------------------
//                   Functions
// ----------------------------------------------

pubnub_log_value_t pubnub_log_value_null(void)
{
    pubnub_log_value_t value;
    memset(&value, 0, sizeof(value));
    value.type = PUBNUB_LOG_VALUE_NULL;
    return value;
}

pubnub_log_value_t pubnub_log_value_bool(bool val)
{
    pubnub_log_value_t value;
    memset(&value, 0, sizeof(value));
    value.type          = PUBNUB_LOG_VALUE_BOOL;
    value.data.bool_val = val;
    return value;
}

pubnub_log_value_t pubnub_log_value_number(double val)
{
    pubnub_log_value_t value;
    memset(&value, 0, sizeof(value));
    value.type            = PUBNUB_LOG_VALUE_NUMBER;
    value.data.number_val = val;
    return value;
}

pubnub_log_value_t pubnub_log_value_string(char const* val)
{
    pubnub_log_value_t value;
    memset(&value, 0, sizeof(value));
    value.type            = PUBNUB_LOG_VALUE_STRING;
    value.data.string_val = val;
    return value;
}

pubnub_log_value_t pubnub_log_value_array_init(void)
{
    pubnub_log_value_t value;
    memset(&value, 0, sizeof(value));
    value.type = PUBNUB_LOG_VALUE_ARRAY;
    return value;
}

pubnub_log_value_t pubnub_log_value_map_init(void)
{
    pubnub_log_value_t value;
    memset(&value, 0, sizeof(value));
    value.type = PUBNUB_LOG_VALUE_MAP;
    return value;
}

void pubnub_log_value_array_append(pubnub_log_value_t* array,
                                   pubnub_log_value_t* element)
{
    if (NULL == array || NULL == element || array->type != PUBNUB_LOG_VALUE_ARRAY)
        return;

    *(struct pubnub_log_value const**)&element->next = NULL;
    *(char const**)&element->key                     = NULL;

    if (NULL == array->data.container.first) {
        array->data.container.first = element;
    }
    else {
        pubnub_log_value_t* last = (pubnub_log_value_t*)array->data.container.last;
        *(struct pubnub_log_value const**)&last->next = element;
    }
    array->data.container.last = element;
}

void pubnub_log_value_map_set(pubnub_log_value_t* map,
                              char const*         key,
                              pubnub_log_value_t* element)
{
    if (NULL == map || NULL == key || NULL == element
        || map->type != PUBNUB_LOG_VALUE_MAP) {
        return;
    }

    *(struct pubnub_log_value const**)&element->next = NULL;
    *(char const**)&element->key                     = key;

    if (NULL == map->data.container.first) {
        map->data.container.first = element;
    }
    else {
        pubnub_log_value_t* last = (pubnub_log_value_t*)map->data.container.last;
        *(struct pubnub_log_value const**)&last->next = element;
    }
    map->data.container.last = element;
}

enum pubnub_log_value_type pubnub_log_value_type(pubnub_log_value_t const* value)
{
    return value ? value->type : PUBNUB_LOG_VALUE_NULL;
}

int pubnub_log_value_get_bool(pubnub_log_value_t const* value, bool* out_value)
{
    if (NULL == value || NULL == out_value || value->type != PUBNUB_LOG_VALUE_BOOL)
        return -1;

    *out_value = value->data.bool_val;
    return 0;
}

int pubnub_log_value_get_number(pubnub_log_value_t const* value, double* out_value)
{
    if (NULL == value || NULL == out_value
        || value->type != PUBNUB_LOG_VALUE_NUMBER) {
        return -1;
    }

    *out_value = value->data.number_val;
    return 0;
}

char const* pubnub_log_value_get_string(pubnub_log_value_t const* value)
{
    if (NULL == value || value->type != PUBNUB_LOG_VALUE_STRING)
        return NULL;

    return value->data.string_val;
}

pubnub_log_value_t const* pubnub_log_value_map_get(pubnub_log_value_t const* map,
                                                   char const* key)
{
    if (NULL == map || NULL == key || map->type != PUBNUB_LOG_VALUE_MAP)
        return NULL;

    pubnub_log_value_t const* element =
        (pubnub_log_value_t const*)map->data.container.first;
    while (element) {
        if (element->key && strcmp(element->key, key) == 0)
            return element;
        element = element->next;
    }

    return NULL;
}

pubnub_log_value_t const* pubnub_log_value_first(pubnub_log_value_t const* container)
{
    if (NULL == container)
        return NULL;

    if (container->type != PUBNUB_LOG_VALUE_ARRAY
        && container->type != PUBNUB_LOG_VALUE_MAP) {
        return NULL;
    }

    return (pubnub_log_value_t const*)container->data.container.first;
}

char const* pubnub_log_value_key(pubnub_log_value_t const* value)
{
    return value ? value->key : NULL;
}

pubnub_log_value_t const* pubnub_log_value_next(pubnub_log_value_t const* value)
{
    return value ? value->next : NULL;
}
