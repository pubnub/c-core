/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_json_parse.h"

#include "pubnub_assert.h"
#include "pubnub_log.h"
#include <string.h>


char const* pbjson_skip_whitespace(char const* start, char const* end)
{
    for (; start < end; ++start) {
        switch (*start) {
        case ' ':
        case '\t':
        case '\r':
        case '\n':
            break;
        default:
            return start;
        }
    }
    return start;
}


char const* pbjson_find_end_string(char const* start, char const* end)
{
    bool in_escape = false;

    for (; start < end; ++start) {
        switch (*start) {
        case '\\':
            in_escape = !in_escape;
            break;
        case '\0':
            return start;
        case '"':
            if (!in_escape) {
                return start;
            }
            /*FALLTHRU*/
        default:
            in_escape = false;
            break;
        }
    }

    return start;
}


char const* pbjson_find_end_primitive(char const* start, char const* end)
{
    for (; start < end; ++start) {
        switch (*start) {
        case ' ':
        case '\t':
        case '\r':
        case '\n':
        case ',':
        case '}':
        case ']':
            return start - 1;
        case '\0':
            return start;
        default:
            break;
        }
    }
    return start;
}


char const* pbjson_find_end_complex(char const* start, char const* end)
{
    bool        in_string = false, in_escape = false;
    int         bracket_level = 0, brace_level = 0;
    char        c;
    char const* s;

    for (s = start, c = *s; (c != '\0') && (s < end); ++s, c = *s) {
        if (!in_string) {
            switch (c) {
            case '{':
                ++brace_level;
                break;
            case '}':
                if ((--brace_level == 0) && (0 == bracket_level)) {
                    return s;
                }
                break;
            case '[':
                ++bracket_level;
                break;
            case ']':
                if ((--bracket_level == 0) && (0 == brace_level)) {
                    return s;
                }
                break;
            case '"':
                in_string = true;
                in_escape = false;
                break;
            default:
                break;
            }
        }
        else {
            switch (c) {
            case '\\':
                in_escape = !in_escape;
                break;
            case '"':
                if (!in_escape) {
                    in_string = false;
                    break;
                }
                /*FALLTHRU*/
            default:
                in_escape = false;
                break;
            }
        }
    }
    return s;
}


char const* pbjson_find_end_element(char const* start, char const* end)
{
    switch (*start) {
    case '"':
        return pbjson_find_end_string(start + 1, end);
    case '{':
    case '[':
        return pbjson_find_end_complex(start, end);
    default:
        return pbjson_find_end_primitive(start + 1, end);
    }
}


enum pbjson_object_name_parse_result
pbjson_get_object_value(struct pbjson_elem const* p,
                        char const*               name,
                        struct pbjson_elem*       parsed)
{
    char const* s        = pbjson_skip_whitespace(p->start, p->end);
    unsigned    name_len = strlen(name);
    char const* end;

    if (0 == name_len) {
        return jonmpInvalidKeyName;
    }
    if (*s != '{') {
        return jonmpNoStartCurly;
    }
    while (s < p->end) {
        bool found = false;
        s          = pbjson_skip_whitespace(s + 1, p->end);
        if (s == p->end) {
            return jonmpKeyMissing;
        }
        if (*s != '"') {
            return jonmpKeyNotString;
        }
        end = pbjson_find_end_string(s + 1, p->end);
        if (end == p->end) {
            return jonmpStringNotTerminated;
        }
        if (*end != '"') {
            return jonmpStringNotTerminated;
        }
        found = (end - s - 1 == name_len) && (0 == memcmp(s + 1, name, name_len));
        s     = pbjson_skip_whitespace(end + 1, p->end);
        if (s == p->end) {
            return jonmpMissingColon;
        }
        if (*s != ':') {
            return jonmpMissingColon;
        }
        s   = pbjson_skip_whitespace(s + 1, p->end);
        end = pbjson_find_end_element(s, p->end);
        if (end == p->end) {
            return jonmpValueIncomplete;
        }
        if ('\0' == *end) {
            return jonmpValueIncomplete;
        }
        if (found) {
            parsed->start = s;
            parsed->end   = end + 1;
            return jonmpOK;
        }
        s = pbjson_skip_whitespace(end + 1, p->end);
        if (*s != ',') {
            if (*s == '}') {
                break;
            }
            return jonmpMissingValueSeparator;
        }
    }

    return (s < p->end) ? jonmpKeyNotFound : jonmpObjectIncomplete;
}


bool pbjson_elem_equals_string(struct pbjson_elem const* e, char const* s)
{
    char const* p;
    for (p = e->start; p != e->end; ++p, ++s) {
        if (*p != *s) {
            return false;
        }
    }
    return *s == '\0';
}


char const* pbjson_object_name_parse_result_2_string(enum pbjson_object_name_parse_result e)
{
    switch (e) {
    case jonmpNoStartCurly:
        return "No Start Curly";
    case jonmpKeyMissing:
        return "Key Missing";
    case jonmpKeyNotString:
        return "Key Not String";
    case jonmpStringNotTerminated:
        return "String Not Terminated";
    case jonmpMissingColon:
        return "Missing Colon";
    case jonmpObjectIncomplete:
        return "Object Incomplete";
    case jonmpMissingValueSeparator:
        return "Missing Value Separator";
    case jonmpKeyNotFound:
        return "Key Not Found";
    case jonmpInvalidKeyName:
        return "Invalid Key Name";
    case jonmpOK:
        return "OK";
    default:
        return "?!?";
    }
}


size_t pbjson_element_strcpy(struct pbjson_elem const* p, char* s, size_t n)
{
    size_t len;

    PUBNUB_ASSERT_OPT(p != NULL);
    PUBNUB_ASSERT_OPT(s != NULL);
    PUBNUB_ASSERT_OPT(n > 0);

    len = p->end - p->start;
    if (len >= n) {
        len = n - 1;
    }
    if (len > 0) {
        memcpy(s, p->start, len);
    }
    s[len] = '\0';

    return len + 1;
}

bool pbjson_value_for_field_found(struct pbjson_elem const* p, char const* name, char const* value) {
    enum pbjson_object_name_parse_result result;
    struct pbjson_elem                   found;

    result = pbjson_get_object_value(p, name, &found);

    return jonmpOK == result && pbjson_elem_equals_string(&found, value);
}

char* pbjson_get_status_400_message_value(struct pbjson_elem const* el){
    enum pbjson_object_name_parse_result json_rslt;
    struct pbjson_elem parsed;
    json_rslt = pbjson_get_object_value(el, "message", &parsed);
    if (json_rslt == jonmpOK) {
        int parse_len = (int)(parsed.end - parsed.start);
        PUBNUB_LOG_ERROR("pbjson_get_status_400_message_value: \"error\"='%.*s'\n",
                        parse_len,
                        parsed.start);
        char* msgtext = (char*)malloc(sizeof(char) * (parse_len+3));
        sprintf(msgtext, "%.*s", parse_len, parsed.start);
        return msgtext;
    }

    return NULL;
}