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
#include "core/pubnub_crypto.h"

#include <ctype.h>
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

static int currentNestLevel = 0;
static CborError data_recursion(CborValue* it, int nestingLevel, char* json_result)
{
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
                strcat(json_result, ", ");
            }
            strcat(json_result, type == CborArrayType ? "[" : "{");
            err = cbor_value_enter_container(it, &recursed);
            if (err) { return err; }
            err = data_recursion(&recursed, nestingLevel + 1, json_result);
            if (err) { return err; }
            err = cbor_value_leave_container(it, &recursed);
            if (err) { return err; }

            strcat(json_result, type == CborArrayType ? "]" : "}");

            bool is_container = cbor_value_is_container(it);
            bool is_array = cbor_value_is_container(it);
            if (!is_container && !is_array && currentNestLevel == nestingLevel && cbor_value_get_type(it) != CborInvalidType) {
                type = cbor_value_get_type(it);
                strcat(json_result, ", ");
            }
            containerEnd = true;
            currentNestLevel--;


            continue;
        }

        case CborIntegerType: {
            int64_t val;
            cbor_value_get_int64(it, &val);     // can't fail
            char int_str[100];
            sprintf(int_str, "%lld", (long long)val);
            strcat(json_result, int_str);
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
                sprintf(byte_str, "\"%s\":", buf);
                strcat(json_result, byte_str);
            }
            else {
                if (sig_flag) {
                    int max_size = base64_max_size(n);
                    char* sig_base64 = (char*)malloc(max_size);
                    base64encode(sig_base64, max_size, buf, n);
                    char base64_str[1000];
                    sprintf(base64_str, "\"%s\"", sig_base64);
                    free(sig_base64);
                    strcat(json_result, base64_str);
                    sig_flag = false;
                }
                else {
                    char* buff_str = (char*)malloc(sizeof(char) * (n+3));
                    sprintf(buff_str, "\"%s\"", buf);
                    strcat(json_result, buff_str);
                    free(buff_str);
                }
            }
            if (strcmp((const char*)buf, "sig") == 0) {
                sig_flag = true;
            } 
	    // TODO: why????
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
	    
	    char* txt_str = (char*)malloc(sizeof(char) * (n+4));
	    
	    type = cbor_value_get_type(it);
	    if (!uuid_flag) {
		    sprintf(txt_str, "\"%s\":", buf);
		    uuid_flag = false;
	    } else {
		    sprintf(txt_str, "\"%s\",", buf);
	    }

	    strcat(json_result, txt_str);
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
            sprintf(bool_str, "%s:", val ? "true" : "false");
            strcat(json_result, bool_str);

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
            sprintf(flt_str, "%lf", val);
            strcat(json_result, flt_str);

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
            strcat(json_result, ", ");
        }
            
    }

    return CborNoError;
}

char* pubnub_parse_token(pubnub_t* pb, char const* token){
    char * rawToken = strdup(token);
    replace_char((char*)rawToken, '_', '/');
    replace_char((char*)rawToken, '-', '+');

    pubnub_bymebl_t decoded;
    decoded = pbbase64_decode_alloc_std_str(rawToken);
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
    
    char * json_result = (char*)malloc(5*(strlen(rawToken)/4));
    sprintf(json_result, "%s", "");
    CborError err = cbor_parser_init(buf, length, 0, &parser, &it);
    if (!err){
        data_recursion(&it, 1, json_result);
    }
    free(decoded.ptr);
    free(rawToken);
    return json_result;
}
