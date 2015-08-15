/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_JSON_PARSE
#define      INC_PUBNUB_JSON_PARSE

#include <stdbool.h>


/** @file pubnub_json_parse.h 

    A bunch of functions for parsing JSON. These are designed for
    internal use by the Pubnub client, but, since they are rather
    generic, the user can use them, too, though their availability and
    interface are not guaranteed to survive Pubnub client version
    changes.
 */

/** A representation of a JSON element. */
struct pbjson_elem {
    /** The start of the element - pointer to the first
        character. */
    char const *start;
    /** The end of the element - pointer to the character
        _after_ the last character */
    char const *end;
};


/** Results of parsing a JSON (object) definition */
enum pbjson_object_name_parse_result {
    /** No starting curly brace `{` */
    jonmpNoStartCurly,
    /** Key is missing from a JSON object definition */
    jonmpKeyMissing,
    /** Key is not a string (this forbidden by JSON) */
    jonmpKeyNotString,
    /** String not terminated (no end `"`) */
    jonmpStringNotTerminated,
    /** Colon (`:`) is missing in JSON object definition */
    jonmpMissingColon,
    /** Ending curly brace `}` is missing */
    jonmpObjectIncomplete,
    /** A comma `,`, delimiting key-value pairs in a JSON
        object, is missing.*/
    jonmpMissingValueSeparator,
    /** The key was not found in the JSON object definition */
    jonmpKeyNotFound,
    /** The name of the key is empty or otherwise not valid */
    jonmpInvalidKeyName,
    /** Parsed OK, JSON (object) is valid */
    jonmpOK
};


/** Skips whitespace starting from @p start, until @p end.
    Interprets whitespace as JSON does - that should be
    compatible with a lot of other specifications.

    @return Pointer to the first character that is not whitespace.
    It is == @p end if the whole input is skipped.
 */
char const* pbjson_skip_whitespace(char const *start, char const *end);


/** Finds the end of the string starting from @p start, until @p end.
    Interprets string as JSON does (starting and ending with
    double-quotation and allowing escape characters with backslash) -
    that should be compatible with a lot of other specifications.

    Assumes that @p start points to the first character of the string
    (that is, one past the opening double-quote).

    @return Pointer to the double-quotation character that is the end
    of string from input.  It is == @p end if the end of the string
    was not found in input.
*/
    
char const* pbjson_find_end_string(char const *start, char const *end);


/** Finds the end of the "primitive" value starting from @p start,
    until @p end.  Interprets "primitive value" a little more broadly
    than JSON does (basically, we allow anything that is not a JSON
    string, array or object) - that should be compatible with a lot of
    other specifications.

    Assumes that @p start points to the first character of the
    primitive.

    @return Pointer to the character that is the end of the primitive.
    It is == @p end if the end of the primitive was not found in
    input.
 */
char const *pbjson_find_end_primitive(char const *start, char const *end);


/** Finds the end of the "complex" value starting from @p start, until
    @p end.  Interprets "complex value" as a JSON object or array -
    that should be compatible with a lot of other specifications.

    Assumes that @p start points to the first character of the
    complex object (opening curly brace or square bracket).

    @return Pointer to the character that is the end of the complex.
    It is == @p end if the end of the complex was not found in
    input.
 */
char const *pbjson_find_end_complex(char const *start, char const *end);


/** Finds the end of the JSON element starting from @p start, until
    @p end.  Interprets element as JSON does (primitive, object or array) -
    that should be compatible with a lot of other specifications.

    Assumes that @p start points to the first character of the
    element.

    @return Pointer to the character that is the end of the element.
    It is == @p end if the end of the element was not found in
    input.
 */
char const *pbjson_find_end_element(char const *start, char const *end);


/** Gets the value from a JSON object from @p p, with the key @p name
    and puts it to @p parsed o success, returning jonmpOK. On failure,
    returns the error code and the effects on @p parsed are not
    defined.
*/
enum pbjson_object_name_parse_result pbjson_get_object_value(struct pbjson_elem const *p, char const *name, struct pbjson_elem *parsed);


/** Helper function, returns whether string @p s is equal to the
    contents of the JSON element @p e.
*/
bool pbjson_elem_equals_string(struct pbjson_elem const *e, char const *s);

/** Helper function, returns a string describing an enum for
    the JSON (object) parse result.
 */
char const *pbjson_object_name_parse_result_2_string(enum pbjson_object_name_parse_result e);


#endif /* !defined INC_PUBNUB_JSON_PARSE */
