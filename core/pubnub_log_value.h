/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#ifndef PUBNUB_PUBNUB_LOG_VALUE_H
#define PUBNUB_PUBNUB_LOG_VALUE_H


/**
 * @file  pubnub_log_value.h
 * @brief PubNub logging subsystem structured data types for log entries.
 */


#include <stdbool.h>

#include "lib/pb_extern.h"


// ----------------------------------------------
//                     Types
// ----------------------------------------------

/**
 * @brief Value types for structured logging.
 *
 * Represents the type of data contained in a `pubnub_log_value_t`.
 */
enum pubnub_log_value_type {
    /** Null/undefined value */
    PUBNUB_LOG_VALUE_NULL,
    /** Boolean value (true/false) */
    PUBNUB_LOG_VALUE_BOOL,
    /** Numeric value (integer or floating-point) */
    PUBNUB_LOG_VALUE_NUMBER,
    /** String value (null-terminated) */
    PUBNUB_LOG_VALUE_STRING,
    /** Array of values (ordered list) */
    PUBNUB_LOG_VALUE_ARRAY,
    /** Map/dictionary of key-value pairs */
    PUBNUB_LOG_VALUE_MAP
};

/**
 * @brief Structured log value definition.
 *
 * This structure serves dual purposes:
 * - A standalone value (primitive or container).
 * - An element within an array or map (with key and next pointer for linking).
 *
 * @warning All storage is stack-allocated. Strings and pointers are borrowed,
 *          not owned.
 *
 * @note While the struct fields are visible, users should treat this as
 *       semi-opaque and use the provided accessor functions rather than
 *       accessing fields directly.
 */
struct pubnub_log_value {
    /** Type of this value */
    enum pubnub_log_value_type type;
    /**
     * @brief Key for `map` elements.
     *
     * @note `NULL` for `array` elements or standalone values.
     * @note Logging subsystem will borrow pointer - must remain valid for the
     *       lifetime of the value.
     */
    char const* key;
    /**
     * Next element in linked list (for `array`/`map` iteration).
     *
     * @note `NULL` for standalone values or last element in a container.
     */
    struct pubnub_log_value const* next;
    /** Union holding the actual data based on type. */
    union {
        /** Boolean value (for `PUBNUB_LOG_VALUE_BOOL`). */
        bool bool_val;
        /** Numeric value (for `PUBNUB_LOG_VALUE_NUMBER`). */
        double number_val;
        /**
         * @brief String value (for `PUBNUB_LOG_VALUE_STRING`).
         *
         * @note Logging subsystem will borrow pointer - must remain valid for
         *       the lifetime of the value.
         */
        char const* string_val;
        /** Container structure (for `ARRAY` and `MAP`). */
        struct {
            /** First element in the container. */
            struct pubnub_log_value const* first;
            /** Last element (for O(1) append). */
            struct pubnub_log_value const* last;
        } container;
    } data;
};
typedef struct pubnub_log_value pubnub_log_value_t;


// ----------------------------------------------
//                   Functions
// ----------------------------------------------

/**
 * @brief Create stack-allocated `null` value.
 *
 * @return A log value representing `null`.
 *
 * @code
 * pubnub_log_value_t val = pubnub_log_value_null();
 * @endcode
 */
PUBNUB_EXTERN pubnub_log_value_t pubnub_log_value_null(void);

/**
 * @brief Create stack-allocated `boolean` value.
 *
 * @param value The `boolean` value (true/false).
 * @return A log value representing `boolean`.
 *
 * @code
 * pubnub_log_value_t val = pubnub_log_value_bool(true);
 * @endcode
 */
PUBNUB_EXTERN pubnub_log_value_t pubnub_log_value_bool(bool value);

/**
 * @brief Create stack-allocated `numeric` value.
 *
 * @param value The `numeric` value (integer or floating-point).
 * @return A log value representing `number`.
 *
 * @code
 * pubnub_log_value_t val = pubnub_log_value_number(42);
 * @endcode
 */
PUBNUB_EXTERN pubnub_log_value_t pubnub_log_value_number(double value);

/**
 * @brief Create stack-allocated `string` value.
 *
 * @note Logging subsystem will borrow pointer. The caller must ensure the
 *       `string` remains valid for the lifetime of the log value (for the
 *       duration of the log call).
 *
 * @param value Pointer to the null-terminated string value (borrowed, not
 *              copied).
 * @return A log value representing `string`.
 *
 * @code
 * pubnub_log_value_t val = pubnub_log_value_string("my-channel");
 * @endcode
 */
PUBNUB_EXTERN pubnub_log_value_t pubnub_log_value_string(char const* value);

/**
 * @brief Create stack-allocated empty `array` value.
 *
 * @return A log value representing empty `array`.
 *
 * @code
 * pubnub_log_value_t channels = pubnub_log_value_array_init();
 * pubnub_log_value_t ch1 = pubnub_log_value_string("channel-1");
 * pubnub_log_value_t ch2 = pubnub_log_value_string("channel-2");
 * pubnub_log_value_array_append(&channels, &ch1);
 * pubnub_log_value_array_append(&channels, &ch2);
 * @endcode
 *
 * Or using convenience macros:
 * @code
 * pubnub_log_value_t channels = pubnub_log_value_array_init();
 * PUBNUB_LOG_ARRAY_APPEND_STRING(&channels, "channel-1", ch1)
 * PUBNUB_LOG_ARRAY_APPEND_STRING(&channels, "channel-2", ch2)
 * @endcode
 *
 * @see pubnub_log_value_array_append
 */
PUBNUB_EXTERN pubnub_log_value_t pubnub_log_value_array_init(void);

/**
 * @brief Create stack-allocated empty `map` (dictionary) value.
 *
 * @return A log value representing empty `map`.
 *
 * @code
 * pubnub_log_value_t params  = pubnub_log_value_map_init();
 * pubnub_log_value_t ch_val  = pubnub_log_value_string(channel);
 * pubnub_log_value_t ttl_val = pubnub_log_value_number(300);
 * pubnub_log_value_map_set(&params, "channel", &ch_val);
 * pubnub_log_value_map_set(&params, "ttl", &ttl_val);
 * @endcode
 *
 * Or using convenience macros:
 * @code
 * pubnub_log_value_t params = pubnub_log_value_map_init();
 * PUBNUB_LOG_MAP_SET_STRING(&params, channel)
 * PUBNUB_LOG_MAP_SET_NUMBER(&params, opts.ttl, ttl)
 * @endcode
 *
 * @see pubnub_log_value_map_set
 */
PUBNUB_EXTERN pubnub_log_value_t pubnub_log_value_map_init(void);

/**
 * @brief Append a `value` to an array.
 *
 * @note The element becomes part of the array's linked list. The element must
 *       remain valid for the lifetime of the `array` (for the duration of a log
 *       call).
 *
 * @param array   Pointer to the array to append to (must be
 *                `PUBNUB_LOG_VALUE_ARRAY` type).
 * @param element Pointer to the stack-allocated value to append.
 *
 * @code
 * pubnub_log_value_t arr = pubnub_log_value_array_init();
 * pubnub_log_value_t v1  = pubnub_log_value_number(1);
 * pubnub_log_value_t v2  = pubnub_log_value_number(2);
 * pubnub_log_value_array_append(&arr, &v1);
 * pubnub_log_value_array_append(&arr, &v2);
 * @endcode
 *
 * @see pubnub_log_value_array_init
 */
PUBNUB_EXTERN void pubnub_log_value_array_append(
    pubnub_log_value_t* array,
    pubnub_log_value_t* element);

/**
 * @brief Set a key-value pair in a `map`.
 *
 * @note The element becomes part of the map's linked list with the specified
 *       key. Both the element and key must remain valid for the lifetime of
 *       the `map` (for the duration of a log call).
 *
 * @param map     Pointer to the map to add to (must be
 *                `PUBNUB_LOG_VALUE_MAP` type).
 * @param key     Pointer to the key string (borrowed, not copied).
 * @param element Pointer to the stack-allocated value to add.
 *
 * @code
 * pubnub_log_value_t map     = pubnub_log_value_map_init();
 * pubnub_log_value_t ch_val  = pubnub_log_value_string("my-channel");
 * pubnub_log_value_t ttl_val = pubnub_log_value_number(300);
 * pubnub_log_value_map_set(&map, "channel", &ch_val);
 * pubnub_log_value_map_set(&map, "ttl", &ttl_val);
 * @endcode
 *
 * @see pubnub_log_value_map_init
 */
PUBNUB_EXTERN void pubnub_log_value_map_set(
    pubnub_log_value_t* map,
    char const*         key,
    pubnub_log_value_t* element);

/**
 * @brief Get the `type` of a log value.
 *
 * @param value Pointer to the log value to query.
 * @return The value type, or `PUBNUB_LOG_VALUE_NULL` if value is `NULL`.
 *
 * @code
 * if (pubnub_log_value_type(val) == PUBNUB_LOG_VALUE_STRING)
 *     printf("string: %s\n", pubnub_log_value_get_string(val));
 * @endcode
 */
PUBNUB_EXTERN enum pubnub_log_value_type pubnub_log_value_type(
    pubnub_log_value_t const* value);

/**
 * @brief Get the `boolean` from a log value.
 *
 * @param value          Pointer to the log value (must be
 *                       `PUBNUB_LOG_VALUE_BOOL` type).
 * @param[out] out_value Pointer to store the boolean value.
 * @retval 0 on success.
 * @retval -1 on failure (value is `NULL` or not a `boolean`).
 *
 * @code
 * bool flag;
 * if (pubnub_log_value_get_bool(val, &flag) == 0)
 *     printf("flag: %s\n", flag ? "true" : "false");
 * @endcode
 */
PUBNUB_EXTERN int pubnub_log_value_get_bool(
    pubnub_log_value_t const* value,
    bool*                     out_value);

/**
 * @brief Get the `numeric` from a log value.
 *
 * @param value          Pointer to the log value (must be
 *                       `PUBNUB_LOG_VALUE_NUMBER` type).
 * @param[out] out_value Pointer to store the numeric value.
 * @retval 0 on success.
 * @retval -1 on failure (value is `NULL` or not a `number`).
 *
 * @code
 * double num;
 * if (pubnub_log_value_get_number(val, &num) == 0)
 *     printf("number: %f\n", num);
 * @endcode
 */
PUBNUB_EXTERN int pubnub_log_value_get_number(
    pubnub_log_value_t const* value,
    double*                   out_value);

/**
 * @brief Get the `string` from a log value.
 *
 * @warning The returned pointer must not be modified or freed.
 *
 * @param value Pointer to the log value (must be
 *              `PUBNUB_LOG_VALUE_STRING` type).
 * @return Pointer to the string, or `NULL` if value is `NULL` or not a
 *         `string`.
 *
 * @code
 * char const* str = pubnub_log_value_get_string(val);
 * if (str)
 *     printf("string: %s\n", str);
 * @endcode
 */
PUBNUB_EXTERN char const* pubnub_log_value_get_string(
    pubnub_log_value_t const* value);

/**
 * @brief Get a `value` from a `map` by key.
 *
 * @param map Pointer to the `map` to search (must be
 *            `PUBNUB_LOG_VALUE_MAP` type).
 * @param key Pointer to the key to look for.
 * @return Pointer to the value, or `NULL` if key not found or `map` is `NULL`.
 *
 * @code
 * pubnub_log_value_t const* ch = pubnub_log_value_map_get(&params, "channel");
 * if (ch)
 *     printf("channel: %s\n", pubnub_log_value_get_string(ch));
 * @endcode
 *
 * @see pubnub_log_value_map_set
 */
PUBNUB_EXTERN pubnub_log_value_t const* pubnub_log_value_map_get(
    pubnub_log_value_t const* map,
    char const*               key);

/**
 * @brief Get the first element in an `array` or `map`.
 *
 * Works for both `arrays` and `maps`. Use `pubnub_log_value_next` to iterate
 * through remaining elements.
 *
 * @param container Pointer to the `array` or `map` value (must be
 *                  `PUBNUB_LOG_VALUE_ARRAY` or `PUBNUB_LOG_VALUE_MAP` type).
 * @return Pointer to first element, or `NULL` if empty or not a container.
 *
 * Iterate over an array:
 * @code
 * for (pubnub_log_value_t const* el = pubnub_log_value_first(&arr);
 *      el != NULL;
 *      el = pubnub_log_value_next(el)) {
 *     printf("  %s\n", pubnub_log_value_get_string(el));
 * }
 * @endcode
 *
 * Iterate over a map:
 * @code
 * for (pubnub_log_value_t const* el = pubnub_log_value_first(&map);
 *      el != NULL;
 *      el = pubnub_log_value_next(el)) {
 *     printf("  %s = %s\n", pubnub_log_value_key(el),
 *            pubnub_log_value_get_string(el));
 * }
 * @endcode
 *
 * @see pubnub_log_value_next
 * @see pubnub_log_value_key
 */
PUBNUB_EXTERN pubnub_log_value_t const* pubnub_log_value_first(
    pubnub_log_value_t const* container);

/**
 * @brief Get the key from a `map` element.
 *
 * @param value Pointer to the `value` obtained from `pubnub_log_value_first` or
 *              `pubnub_log_value_next` when iterating over a `map`.
 * @return Pointer to the element's key string, or `NULL` if value is `NULL` or
 *         not a `map` element.
 *
 * @code
 * pubnub_log_value_t const* el = pubnub_log_value_first(&map);
 * char const* key = pubnub_log_value_key(el);  // e.g. "channel"
 * @endcode
 *
 * @see pubnub_log_value_first
 * @see pubnub_log_value_next
 */
PUBNUB_EXTERN char const* pubnub_log_value_key(pubnub_log_value_t const* value);

/**
 * @brief Get the next element in an `array` or `map`.
 *
 * @param value A pointer to the element obtained from `pubnub_log_value_first`
 *              or a previous call to this function.
 * @return Pointer to the next element, or `NULL` if this is the last element.
 *
 * @code
 * for (pubnub_log_value_t const* el = pubnub_log_value_first(&arr);
 *      el != NULL;
 *      el = pubnub_log_value_next(el)) {
 *     // Process each element.
 * }
 * @endcode
 *
 * @see pubnub_log_value_first
 */
PUBNUB_EXTERN pubnub_log_value_t const* pubnub_log_value_next(
    pubnub_log_value_t const* value);


// ----------------------------------------------
//            Convenience Macros
// ----------------------------------------------

/** Macro overloading helpers - count number of arguments */
#define PUBNUB_LOG_GET_4TH_ARG_(arg1, arg2, arg3, arg4, ...) arg4
#define PUBNUB_LOG_NARG_(...) PUBNUB_LOG_GET_4TH_ARG_(__VA_ARGS__, 3, 2, 1, 0)
#define PUBNUB_LOG_CONCAT_IMPL_(a, b) a##b
#define PUBNUB_LOG_CONCAT_(a, b) PUBNUB_LOG_CONCAT_IMPL_(a, b)
#define PUBNUB_LOG_DISPATCH_(func, nargs) PUBNUB_LOG_CONCAT_(func, nargs)

/**
 * @brief Add a `string` value to a `map` with automatic `NULL` checking.
 *
 * Two-argument form: PUBNUB_LOG_MAP_SET_STRING(map, name)
 *   - Uses 'name' as both the value source and the key
 *   - Creates variable named `name##_val`
 *
 * Three-argument form: PUBNUB_LOG_MAP_SET_STRING(map, value, key)
 *   - Uses 'value' as the source (can be struct member like opts.store)
 *   - Uses 'key' as the variable name base and map key
 *   - Creates variable named `key##_val`
 *
 * @note This macro expands to multiple statements without wrapping them in a
 *       block, so the variable is declared in the caller's scope and remains
 *       valid for subsequent use.
 *
 * @code
 * PUBNUB_LOG_MAP_SET_STRING(&params, channel)
 * PUBNUB_LOG_MAP_SET_STRING(&params, opts.custom_message_type,
 * custom_message_type)
 * @endcode
 */
#define _PUBNUB_LOG_MAP_SET_STRING_2(map, name)                                \
    pubnub_log_value_t name##_val;                                             \
    if (name && name[0] != '\0') {                                             \
        name##_val = pubnub_log_value_string(name);                            \
        pubnub_log_value_map_set(map, #name, &name##_val);                     \
    }

#define _PUBNUB_LOG_MAP_SET_STRING_3(map, value, key)                          \
    pubnub_log_value_t key##_val;                                              \
    if (value && value[0] != '\0') {                                           \
        key##_val = pubnub_log_value_string(value);                            \
        pubnub_log_value_map_set(map, #key, &key##_val);                       \
    }

#define PUBNUB_LOG_MAP_SET_STRING(...)                                         \
    PUBNUB_LOG_DISPATCH_(                                                      \
        _PUBNUB_LOG_MAP_SET_STRING_,                                           \
        PUBNUB_LOG_NARG_(__VA_ARGS__))(__VA_ARGS__)

/**
 * @brief Add a `number` value to a `map`.
 *
 * Two-argument form: PUBNUB_LOG_MAP_SET_NUMBER(map, name)
 *   - Uses 'name' as both the value source and the key
 *
 * Three-argument form: PUBNUB_LOG_MAP_SET_NUMBER(map, value, key)
 *   - Uses 'value' as the source (can be struct member)
 *   - Uses 'key' as the variable name base and map key
 *
 * @code
 * PUBNUB_LOG_MAP_SET_NUMBER(&params, count)
 * PUBNUB_LOG_MAP_SET_NUMBER(&params, opts.ttl, ttl)
 * @endcode
 */
#define _PUBNUB_LOG_MAP_SET_NUMBER_2(map, name)                                \
    pubnub_log_value_t name##_val = pubnub_log_value_number(name);             \
    pubnub_log_value_map_set(map, #name, &name##_val);

#define _PUBNUB_LOG_MAP_SET_NUMBER_3(map, value, key)                          \
    pubnub_log_value_t key##_val = pubnub_log_value_number(value);             \
    pubnub_log_value_map_set(map, #key, &key##_val);

#define PUBNUB_LOG_MAP_SET_NUMBER(...)                                         \
    PUBNUB_LOG_DISPATCH_(                                                      \
        _PUBNUB_LOG_MAP_SET_NUMBER_,                                           \
        PUBNUB_LOG_NARG_(__VA_ARGS__))(__VA_ARGS__)

/**
 * @brief Add a `boolean` value to a `map`.
 *
 * Two-argument form: PUBNUB_LOG_MAP_SET_BOOL(map, name)
 *   - Uses 'name' as both the value source and the key
 *
 * Three-argument form: PUBNUB_LOG_MAP_SET_BOOL(map, value, key)
 *   - Uses 'value' as the source (can be struct member)
 *   - Uses 'key' as the variable name base and map key
 *
 * @code
 * PUBNUB_LOG_MAP_SET_BOOL(&params, include_token)
 * PUBNUB_LOG_MAP_SET_BOOL(&params, opts.store, store)
 * @endcode
 */
#define _PUBNUB_LOG_MAP_SET_BOOL_2(map, name)                                  \
    pubnub_log_value_t name##_val = pubnub_log_value_bool(name);               \
    pubnub_log_value_map_set(map, #name, &name##_val);

#define _PUBNUB_LOG_MAP_SET_BOOL_3(map, value, key)                            \
    pubnub_log_value_t key##_val = pubnub_log_value_bool(value);               \
    pubnub_log_value_map_set(map, #key, &key##_val);

#define PUBNUB_LOG_MAP_SET_BOOL(...)                                           \
    PUBNUB_LOG_DISPATCH_(                                                      \
        _PUBNUB_LOG_MAP_SET_BOOL_, PUBNUB_LOG_NARG_(__VA_ARGS__))(__VA_ARGS__)

/**
 * @brief Append a `string` value to an `array` with automatic `NULL` checking.
 *
 * Two-argument form: PUBNUB_LOG_ARRAY_APPEND_STRING(array, name)
 *   - Uses 'name' as the value source
 *   - Creates variable named `name##_elm`
 *
 * Three-argument form: PUBNUB_LOG_ARRAY_APPEND_STRING(array, value, name)
 *   - Uses 'value' as the source (can be a literal or expression)
 *   - Uses 'name' as the variable name suffix
 *   - Creates variable named `name##_elm`
 *
 * @code
 * PUBNUB_LOG_ARRAY_APPEND_STRING(&channels, channel)
 * PUBNUB_LOG_ARRAY_APPEND_STRING(&channels, "my-channel", my_channel)
 * @endcode
 */
#define _PUBNUB_LOG_ARRAY_APPEND_STRING_2(array, name)                         \
    pubnub_log_value_t name##_elm;                                             \
    if (name) {                                                                \
        name##_elm = pubnub_log_value_string(name);                            \
        pubnub_log_value_array_append(array, &name##_elm);                     \
    }

#define _PUBNUB_LOG_ARRAY_APPEND_STRING_3(array, value, name)                  \
    pubnub_log_value_t name##_elm;                                             \
    if (value) {                                                               \
        name##_elm = pubnub_log_value_string(value);                           \
        pubnub_log_value_array_append(array, &name##_elm);                     \
    }

#define PUBNUB_LOG_ARRAY_APPEND_STRING(...)                                    \
    PUBNUB_LOG_DISPATCH_(                                                      \
        _PUBNUB_LOG_ARRAY_APPEND_STRING_,                                      \
        PUBNUB_LOG_NARG_(__VA_ARGS__))(__VA_ARGS__)

/**
 * @brief Append a `number` value to an `array`.
 *
 * Two-argument form: PUBNUB_LOG_ARRAY_APPEND_NUMBER(array, name)
 *   - Uses 'name' as the value source
 *
 * Three-argument form: PUBNUB_LOG_ARRAY_APPEND_NUMBER(array, value, name)
 *   - Uses 'value' as the source (can be a literal or expression)
 *   - Uses 'name' as the variable name suffix
 *
 * @code
 * PUBNUB_LOG_ARRAY_APPEND_NUMBER(&limits, max_count)
 * PUBNUB_LOG_ARRAY_APPEND_NUMBER(&limits, 100, default_limit)
 * @endcode
 */
#define _PUBNUB_LOG_ARRAY_APPEND_NUMBER_2(array, name)                         \
    pubnub_log_value_t name##_elm = pubnub_log_value_number(name);             \
    pubnub_log_value_array_append(array, &name##_elm);

#define _PUBNUB_LOG_ARRAY_APPEND_NUMBER_3(array, value, name)                  \
    pubnub_log_value_t name##_elm = pubnub_log_value_number(value);            \
    pubnub_log_value_array_append(array, &name##_elm);

#define PUBNUB_LOG_ARRAY_APPEND_NUMBER(...)                                    \
    PUBNUB_LOG_DISPATCH_(                                                      \
        _PUBNUB_LOG_ARRAY_APPEND_NUMBER_,                                      \
        PUBNUB_LOG_NARG_(__VA_ARGS__))(__VA_ARGS__)

/**
 * @brief Append a `boolean` value to an `array`.
 *
 * Two-argument form: PUBNUB_LOG_ARRAY_APPEND_BOOL(array, name)
 *   - Uses 'name' as the value source
 *
 * Three-argument form: PUBNUB_LOG_ARRAY_APPEND_BOOL(array, value, name)
 *   - Uses 'value' as the source (can be a literal or expression)
 *   - Uses 'name' as the variable name suffix
 *
 * @code
 * PUBNUB_LOG_ARRAY_APPEND_BOOL(&flags, is_enabled)
 * PUBNUB_LOG_ARRAY_APPEND_BOOL(&flags, true, always_on)
 * @endcode
 */
#define _PUBNUB_LOG_ARRAY_APPEND_BOOL_2(array, name)                           \
    pubnub_log_value_t name##_elm = pubnub_log_value_bool(name);               \
    pubnub_log_value_array_append(array, &name##_elm);

#define _PUBNUB_LOG_ARRAY_APPEND_BOOL_3(array, value, name)                    \
    pubnub_log_value_t name##_elm = pubnub_log_value_bool(value);              \
    pubnub_log_value_array_append(array, &name##_elm);

#define PUBNUB_LOG_ARRAY_APPEND_BOOL(...)                                      \
    PUBNUB_LOG_DISPATCH_(                                                      \
        _PUBNUB_LOG_ARRAY_APPEND_BOOL_,                                        \
        PUBNUB_LOG_NARG_(__VA_ARGS__))(__VA_ARGS__)
#endif // PUBNUB_PUBNUB_LOG_VALUE_H
