/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"

#include "core/pubnub_ccore.h"
#include "core/pubnub_netcore.h"
#include "core/pubnub_assert.h"
#include "core/pubnub_timers.h"
#include "core/pubnub_helper.h"
#include "core/pubnub_log.h"
#include "lib/pb_strnlen_s.h"
#include "core/pbcc_grant_token_api.h"
#include "core/pubnub_grant_token_api.h"
#include "core/pubnub_api_types.h"

#include "core/pbpal.h"

#include "lib/cbor/cbor.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "lib/base64/pbbase64.h"
#ifdef _MSC_VER
#define strdup(p) _strdup(p)
#endif

enum grant_bit_flag { 
    PERM_READ = 1
    , PERM_WRITE = 2
    , PERM_MANAGE = 4
    , PERM_DELETE = 8
    , PERM_CREATE = 16
    , PERM_GET = 32
    , PERM_UPDATE = 64
    , PERM_JOIN = 128 };

int pubnub_get_grant_bit_mask_value(struct pam_permission perm)
{
    int result = 0;
    if (perm.read)   { result = PERM_READ; }
    if (perm.write)  { result = result | PERM_WRITE; }
    if (perm.manage) { result = result | PERM_MANAGE; }
    if (perm.del) { result = result | PERM_DELETE; }
    if (perm.create) { result = result | PERM_CREATE; }
    if (perm.get) { result = result | PERM_GET; }
    if (perm.update) { result = result | PERM_UPDATE; }
    if (perm.join) { result = result | PERM_JOIN; }
    return result;
}

enum pubnub_res pubnub_grant_token(pubnub_t* pb, char const* perm_obj)
{
    enum pubnub_res rslt;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (!pbnc_can_start_transaction(pb)) {
        pubnub_mutex_unlock(pb->monitor);
        return PNR_IN_PROGRESS;
    }
    
    pb->method = pubnubSendViaPOST;
    pb->trans = PBTT_GRANT_TOKEN;
    rslt = pbcc_grant_token_prep(&pb->core, perm_obj, pb->trans);
    if (PNR_STARTED == rslt) {
        pb->trans            = PBTT_GRANT_TOKEN;
        pb->core.last_result = PNR_STARTED;
        pb->method           = pubnubSendViaPOST;
        pbnc_fsm(pb);
        rslt = pb->core.last_result;
    }
    pubnub_mutex_unlock(pb->monitor);

    return rslt;
}

pubnub_chamebl_t pubnub_get_grant_token(pubnub_t* pb)
{
    pubnub_chamebl_t result;
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    pubnub_mutex_lock(pb->monitor);
    if (pb->trans != PBTT_GRANT_TOKEN) {
        PUBNUB_LOG_ERROR("pubnub_get_grant_token(pb=%p) can be called only if "
                         "previous transaction is PBTT_GRANT_TOKEN(%d). "
                         "Previous transaction was: %d\n",
                         pb,
                         PBTT_GRANT_TOKEN,
                         pb->trans);
        pubnub_mutex_unlock(pb->monitor);
        result.ptr = NULL;
        result.size = 0;
        return result;
    }
    result = pbcc_get_grant_token(&pb->core);
    pubnub_mutex_unlock(pb->monitor);

    return result;
}

static unsigned int safe_alloc_strcat(char** destination, const char* source, unsigned int alloc_size) {
    unsigned int concat_size = strlen(*destination) + strlen(source);
    if (concat_size > alloc_size) {
    	unsigned int new_allocation_size = (5 * alloc_size) / 4; // +20%
    	char* new_alloc = (char*)malloc(new_allocation_size);
    	memmove(new_alloc, *destination, alloc_size);

    	free(*destination);

    	*destination = new_alloc;
    	return safe_alloc_strcat(destination, source, new_allocation_size);
    }

    strcat(*destination, source);

    return alloc_size;
}

static int currentNestLevel = 0;
static unsigned int current_allocation_size = 0;
static CborError data_recursion(CborValue* it, int nestingLevel, char** json_result, unsigned int init_allocation_size)
{
    current_allocation_size = init_allocation_size;
    bool containerEnd = false;
    bool sig_flag = false;
    bool uuid_flag = false;

    while (!cbor_value_at_end(it)) {
        CborError err;
        CborType type = cbor_value_get_type(it);

        switch (type) {
        case CborArrayType:
        case CborMapType: {
            // recursive type
            CborValue recursed;
            assert(cbor_value_is_container(it));

            if (nestingLevel != currentNestLevel || containerEnd) {
                
                currentNestLevel = nestingLevel;
            }
            else {
                current_allocation_size = safe_alloc_strcat(json_result, ", ", current_allocation_size);
            }
            current_allocation_size = safe_alloc_strcat(json_result, type == CborArrayType ? "[" : "{", current_allocation_size);
            err = cbor_value_enter_container(it, &recursed);
            if (err) { return err; }
            err = data_recursion(&recursed, nestingLevel + 1, json_result, current_allocation_size);
            if (err) { return err; }
            err = cbor_value_leave_container(it, &recursed);
            if (err) { return err; }

            current_allocation_size = safe_alloc_strcat(json_result, type == CborArrayType ? "]" : "}", current_allocation_size);
            
            bool is_container = cbor_value_is_container(it);
            bool is_array = cbor_value_is_container(it);
            if (!is_container && !is_array && currentNestLevel == nestingLevel && cbor_value_get_type(it) != CborInvalidType) {
                type = cbor_value_get_type(it);
                current_allocation_size = safe_alloc_strcat(json_result, ", ", current_allocation_size);
            }
            containerEnd = true;
            currentNestLevel--;


            continue;
        }

        case CborIntegerType: {
            int64_t val;
            cbor_value_get_int64(it, &val);     // can't fail
            char int_str[100];
            snprintf(int_str, sizeof(int_str), "%lld", (long long)val);
            current_allocation_size = safe_alloc_strcat(json_result, int_str, current_allocation_size);
            break;
        }

        case CborByteStringType: {
            uint8_t* buf;
            size_t n;
            err = cbor_value_dup_byte_string(it, &buf, &n, it);
            if (err) { return err; }
            type = cbor_value_get_type(it);
            if (cbor_value_get_type(it) != CborInvalidType) {
                char byte_str[1000];
                snprintf(byte_str, sizeof(byte_str), "\"%s\":", buf);
                current_allocation_size = safe_alloc_strcat(json_result, byte_str, current_allocation_size);
            }
            else {
                if (sig_flag) {
                    pubnub_bymebl_t decoded_sig;
                    decoded_sig.size = n;
                    decoded_sig.ptr = buf;

                    pubnub_bymebl_t encoded_sig = pbbase64_encode_alloc_std(decoded_sig);
                    if (encoded_sig.size == 0 && encoded_sig.ptr == NULL) {
                        PUBNUB_LOG_WARNING("\"sig\" field coudn't be encoded! Leaving it empty!");

                        encoded_sig.ptr = (uint8_t*)malloc(sizeof(uint8_t));
                        encoded_sig.ptr[0] = '\0';
                    }

                    char base64_str[67]; // HMAC+SHA256 max size + quotes + tailing null
                    snprintf(base64_str, sizeof(base64_str), "\"%s\"", encoded_sig.ptr);

                    free(encoded_sig.ptr);
                    current_allocation_size = safe_alloc_strcat(json_result, base64_str, current_allocation_size);

                    sig_flag = false;
                }
                else {
                    size_t buff_size = sizeof(char) * (n+3);
                    char* buff_str = (char*)malloc(buff_size);
                    snprintf(buff_str, buff_size, "\"%s\"", buf);
                    current_allocation_size = safe_alloc_strcat(json_result, buff_str, current_allocation_size);
                    free(buff_str);
                }
            }
            
            // TODO: check cbor docs to fix these flags 
            if (strcmp((const char*)buf, "sig") == 0) {
                sig_flag = true;
            } 
            if (strcmp((const char*)buf, "uuid") == 0) {
                uuid_flag = true;
            }

            free(buf);
            continue;
        }

        case CborTextStringType: {
            char* buf;
            size_t n;
            err = cbor_value_dup_text_string(it, &buf, &n, it);
            if (err) { return err; } // parse error

	    size_t txt_size = sizeof(char) * (n+4);
	    char* txt_str = (char*)malloc(txt_size);
	    
	    type = cbor_value_get_type(it);
	    if (!uuid_flag) {
	        snprintf(txt_str, txt_size, "\"%s\":", buf);
		    uuid_flag = false;
	    } else {
	        snprintf(txt_str, txt_size, "\"%s\",", buf);
	    }

	    current_allocation_size = safe_alloc_strcat(json_result, txt_str, current_allocation_size);
	    free(buf);
	    free(txt_str);

	    continue;
	}

        case CborTagType: {
            CborTag tag;
            cbor_value_get_tag(it, &tag);       // can't fail
            printf("Tag(%lld)\n", (long long)tag);
            break;
        }

        case CborSimpleType: {
            uint8_t type;
            cbor_value_get_simple_type(it, &type);  // can't fail
            printf("simple(%u)\n", type);
            break;
        }

        case CborNullType:
            printf("null");
            break;

        case CborUndefinedType:
            printf("undefined");
            break;

        case CborBooleanType: {
            bool val;
            cbor_value_get_boolean(it, &val);       // can't fail
            char bool_str[10];
	        snprintf(bool_str, sizeof(bool_str), "%s:", val ? "true" : "false");
            current_allocation_size = safe_alloc_strcat(json_result, bool_str, current_allocation_size);

            break;
        }

        case CborDoubleType: {
            double val;
            if (false) {
                float f;
        case CborFloatType:
            cbor_value_get_float(it, &f);
            val = f;
            }
            else {
                cbor_value_get_double(it, &val);
            }
            char flt_str[1000];
	        snprintf(flt_str, sizeof(flt_str), "%lf", val);
            current_allocation_size = safe_alloc_strcat(json_result, flt_str, current_allocation_size);

            break;
        }
        case CborHalfFloatType: {
            uint16_t val;
            cbor_value_get_half_float(it, &val);
            printf("__f16(%04x)\n", val);
            break;
        }

        case CborInvalidType:
            assert(false);      // can't happen
            break;
        }

        err = cbor_value_advance_fixed(it);
        if (err) { return err; }
        if (!cbor_value_at_end(it)) {
            current_allocation_size = safe_alloc_strcat(json_result, ", ", current_allocation_size);
        }
            
    }

    return CborNoError;
}

char* pubnub_parse_token(pubnub_t* pb, char const* token){
    PUBNUB_ASSERT_OPT(token != NULL);

    char * rawToken = strdup(token);
    replace_char((char*)rawToken, '_', '/');
    replace_char((char*)rawToken, '-', '+');

    pubnub_bymebl_t decoded;
    decoded = pbbase64_decode_alloc_std_str(rawToken);
    
    if (decoded.size == 0 && decoded.ptr == NULL) {
        PUBNUB_LOG_ERROR("Base64 decoding failed! Token \"%s\" is not a valid base64 value!\n", token);
        
        free(rawToken);
        return NULL;
    }

    #if PUBNUB_LOG_LEVEL >= PUBNUB_LOG_LEVEL_DEBUG
    PUBNUB_LOG_DEBUG("\nbytes after decoding base64 string = [");
    for (size_t i = 0; i < decoded.size; i++) {
        PUBNUB_LOG_DEBUG("%d ", decoded.ptr[i]);
    }
    PUBNUB_LOG_DEBUG("]\n");
    #endif

    size_t length;
    uint8_t *buf = decoded.ptr;
    length = decoded.size;

    CborParser parser;
    CborValue it;
  
    unsigned int init_allocation_size = 5*(strlen(rawToken)/4);
    char * json_result = (char*)malloc(init_allocation_size);
	snprintf(json_result, init_allocation_size, "%s", "");
    CborError err = cbor_parser_init(buf, length, 0, &parser, &it);

    if (!err){
        err = data_recursion(&it, 1, &json_result, init_allocation_size);
    } else {
        PUBNUB_LOG_ERROR("JSON parsing failed! Cbor error code = %d\n", err);
        
        free(json_result);
        json_result = NULL;
    }
    
    free(decoded.ptr);
    free(rawToken);

    return json_result;
}
