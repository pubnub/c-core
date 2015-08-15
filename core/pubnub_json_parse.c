/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_json_parse.h"

#include <string.h>



char const* pbjson_skip_whitespace(char const *start, char const *end)
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


char const* pbjson_find_end_string(char const *start, char const *end)
{
    bool in_escape = false;

    for (; start < end; ++start) {
        switch (*start) {
        case '"':
            if (!in_escape) {
                return start;
            }
            break;
        case '\\':
            in_escape = !in_escape;
            break;
        case '\0':
            return start;
        default:
            in_escape = false;
            break;
        }
    }

    return start;
}


char const *pbjson_find_end_primitive(char const *start, char const *end)
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
            return start-1;
        case '\0':
            return start;
        default:
            break;
        }
    }
    return start;
}


char const *pbjson_find_end_complex(char const *start, char const *end)
{
    bool in_string = false, in_escape = false;
    int bracket_level = 0, brace_level = 0;
    char c;
    char const *s;

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
            case '"':
                if (!in_escape) {
                    in_string = false;
                }
                break;
            case '\\':
                in_escape = !in_escape;
                break;
            default:
                in_escape = false;
                break;
            }
        }
    }
    return s;
}


char const *pbjson_find_end_element(char const *start, char const *end)
{
    switch (*start) {
    case '"':
        return pbjson_find_end_string(start+1, end);
    case '{':
    case '[':
        return pbjson_find_end_complex(start, end);
    default:
        return pbjson_find_end_primitive(start+1, end);
    }
}


enum pbjson_object_name_parse_result pbjson_get_object_value(struct pbjson_elem const *p, char const *name, struct pbjson_elem *parsed)
{
    char const *s = pbjson_skip_whitespace(p->start, p->end);
    unsigned name_len = strlen(name);
    bool found = false;
    char const *end;

    if (*s != '{') {
        return jonmpNoStartCurly;
    }
    while (s < p->end) {
        s = pbjson_skip_whitespace(s+1, p->end);
        if (s == p->end) {
            return jonmpKeyMissing;
        }
        if (*s != '"') {
            return jonmpKeyNotString;
        }
        end = pbjson_find_end_string(s+1, p->end);
        if (end == p->end) {
            return jonmpStringNotTerminated;
        }
        if (*end != '"') {
            return jonmpStringNotTerminated;
        }
        if (0 == strncmp(s+1, name, name_len)) {
            found = true;
        }
        s = pbjson_skip_whitespace(end+1, p->end);
        if (s == p->end) {
            return jonmpMissingColon;
        }
        if (*s != ':') {
            return jonmpMissingColon;
        }
        s = pbjson_skip_whitespace(s+1, p->end);
        end = pbjson_find_end_element(s, p->end);
        if (found) {
            parsed->start = s;
            parsed->end = end+1;
            return jonmpOK;
        }
        s = pbjson_skip_whitespace(end+1, p->end);
        if (*s != ',') {
            if (*s == '}') {
                break;
            }
            return jonmpMissingValueSeparator;
        }
    }
    
    return jonmpObjectIncomplete;
}


bool pbjson_elem_equals_string(struct pbjson_elem const *e, char const *s)
{
    char const *p = e->start;
    for (p = e->start; p != e->end; ++p, ++s) {
        if (*p != *s) {
            return false;
        }
    }
    return *s == '\0';
}


char const *pbjson_object_name_parse_result_2_string(enum pbjson_object_name_parse_result e)
{
    switch (e) {
    case jonmpNoStartCurly: return "No Start Curly";
    case jonmpKeyMissing: return "Key Missing";
    case jonmpKeyNotString: return "Key Not String";
    case jonmpStringNotTerminated: return "String Not Terminated";
    case jonmpMissingColon: return "Missing Colon";
    case jonmpObjectIncomplete: return "Object Incomplete";
    case jonmpMissingValueSeparator: return "Missing Value Separator";
    case jonmpOK: return "OK";
    default: return "?!?";
    }
}


#if 0

int main()
{
    char const *json = "{\"service\": \"xxx\", \"error\": true, \"payload\":{\"group\":\"gr\",\"chan\":[1,2,3]}, \"message\":0}";
    struct pbjson_elem elem = { json, json + strlen(json) };
    struct pbjson_elem parsed;
    enum json_object_name_parse_result x;
    
    x = parse_json_object_name(&elem, "error", &parsed);
    printf("x = %s, elem.start=%s, elem.length=%ld\n", json_object_name_parse_result_2_string(x), parsed.start, parsed.end - parsed.start);
    assert(jonmpOK == x);
    assert(json_elem_equals_string(&parsed, "true"));
    
    x = pbjson_get_object_value(&elem, "service", &parsed);
    printf("x = %s, elem.start=%s, elem.length=%ld\n", json_object_name_parse_result_2_string(x), parsed.start, parsed.end - parsed.start);
    assert(jonmpOK == x);
    assert(json_elem_equals_string(&parsed, "\"xxx\""));
    
    x = pbjson_get_object_value(&elem, "message", &parsed);
    printf("x = %s, elem.start=%s, elem.length=%ld\n", json_object_name_parse_result_2_string(x), parsed.start, parsed.end - parsed.start);
    assert(jonmpOK == x);
    assert(json_elem_equals_string(&parsed, "0"));
    
    x = pbjson_get_object_value(&elem, "payload", &parsed);
    printf("x = %s, elem.start=%s, elem.length=%ld\n", json_object_name_parse_result_2_string(x), parsed.start, parsed.end - parsed.start);
    assert(jonmpOK == x);
    assert(json_elem_equals_string(&parsed, "{\"group\":\"gr\",\"chan\":[1,2,3]}"));

    elem.end = elem.start;
    x = pbjson_get_object_value(&elem, "payload", &parsed);
    printf("x = %s, elem.start=%s, elem.length=%ld\n", json_object_name_parse_result_2_string(x), parsed.start, parsed.end - parsed.start);
    assert(jonmpObjectIncomplete == x);

    elem.end = elem.start+1;
    x = pbjson_get_object_value(&elem, "payload", &parsed);
    printf("x = %s, elem.start=%s, elem.length=%ld\n", json_object_name_parse_result_2_string(x), parsed.start, parsed.end - parsed.start);
    assert(jonmpKeyMissing == x);

    elem.end = elem.start+2;
    x = pbjson_get_object_value(&elem, "payload", &parsed);
    printf("x = %s, elem.start=%s, elem.length=%ld\n", json_object_name_parse_result_2_string(x), parsed.start, parsed.end - parsed.start);
    assert(jonmpStringNotTerminated == x);

    elem.end = elem.start+10;
    x = pbjson_get_object_value(&elem, "payload", &parsed);
    printf("x = %s, elem.start=%s, elem.length=%ld\n", json_object_name_parse_result_2_string(x), parsed.start, parsed.end - parsed.start);
    assert(jonmpMissingColon == x);

    elem.end = elem.start+11;
    x = pbjson_get_object_value(&elem, "payload", &parsed);
    printf("x = %s, elem.start=%s, elem.length=%ld\n", json_object_name_parse_result_2_string(x), parsed.start, parsed.end - parsed.start);
    assert(jonmpMissingValueSeparator == x);

    elem.end = elem.start+12;
    x = pbjson_get_object_value(&elem, "payload", &parsed);
    printf("x = %s, elem.start=%s, elem.length=%ld\n", json_object_name_parse_result_2_string(x), parsed.start, parsed.end - parsed.start);
    assert(jonmpMissingValueSeparator == x);

    elem.end = elem.start+13;
    x = pbjson_get_object_value(&elem, "payload", &parsed);
    printf("x = %s, elem.start=%s, elem.length=%ld\n", json_object_name_parse_result_2_string(x), parsed.start, parsed.end - parsed.start);
    assert(jonmpMissingValueSeparator == x);

    elem.end = elem.start+17;
    x = pbjson_get_object_value(&elem, "payload", &parsed);
    printf("x = %s, elem.start=%s, elem.length=%ld\n", json_object_name_parse_result_2_string(x), parsed.start, parsed.end - parsed.start);
    assert(jonmpObjectIncomplete == x);

    elem.end = elem.start+18;
    x = pbjson_get_object_value(&elem, "payload", &parsed);
    printf("x = %s, elem.start=%s, elem.length=%ld\n", json_object_name_parse_result_2_string(x), parsed.start, parsed.end - parsed.start);
    assert(jonmpKeyMissing == x);

    elem.end = elem.start+19;
    x = pbjson_get_object_value(&elem, "payload", &parsed);
    printf("x = %s, elem.start=%s, elem.length=%ld\n", json_object_name_parse_result_2_string(x), parsed.start, parsed.end - parsed.start);
    assert(jonmpKeyMissing == x);

    elem.end = elem.start+20;
    x = pbjson_get_object_value(&elem, "payload", &parsed);
    printf("x = %s, elem.start=%s, elem.length=%ld\n", json_object_name_parse_result_2_string(x), parsed.start, parsed.end - parsed.start);
    assert(jonmpStringNotTerminated == x);

    elem.end = elem.start+26;
    x = pbjson_get_object_value(&elem, "payload", &parsed);
    printf("x = %s, elem.start=%s, elem.length=%ld\n", json_object_name_parse_result_2_string(x), parsed.start, parsed.end - parsed.start);
    assert(jonmpMissingColon == x);

    elem.end = elem.start+27;
    x = pbjson_get_object_value(&elem, "payload", &parsed);
    printf("x = %s, elem.start=%s, elem.length=%ld\n", json_object_name_parse_result_2_string(x), parsed.start, parsed.end - parsed.start);
    assert(jonmpMissingValueSeparator == x);

    return 0;
}
#endif
