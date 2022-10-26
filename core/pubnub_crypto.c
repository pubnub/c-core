/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "core/pubnub_crypto.h"

#include "core/pubnub_assert.h"
#include "pubnub_internal.h"
#include "core/pubnub_pubsubapi.h"
#include "core/pubnub_coreapi_ex.h"

#include "lib/md5/pbmd5.h"
#include "pbsha256.h"
#include "pbaes256.h"
#include "lib/base64/pbbase64.h"
#include "pubnub_log.h"
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/pem.h>
#include <openssl/rand.h>

#ifdef _MSC_VER
#define strdup(p) _strdup(p)
#endif

int pbcrypto_signature(struct pbcc_context* pbcc, char const* channel, char const* msg, char* signature, size_t n)
{
#if !PUBNUB_CRYPTO_API
    return -1;
#else
    PBMD5_CTX md5;
    char s[2] = { '/', '\0' };
    uint8_t digest[16];

    PUBNUB_ASSERT_OPT(channel != NULL);
    PUBNUB_ASSERT_OPT(msg != NULL);
    PUBNUB_ASSERT_OPT(signature != NULL);
    PUBNUB_ASSERT_OPT(n > 32);

    if (NULL == pbcc->secret_key) {
        return -1;
    }

    pbmd5_init(&md5);
    pbmd5_update_str(&md5, pbcc->publish_key);
    pbmd5_update(&md5, s, 1);
    pbmd5_update_str(&md5, pbcc->subscribe_key);
    pbmd5_update(&md5, s, 1);
    pbmd5_update_str(&md5, pbcc->secret_key);
    pbmd5_update(&md5, s, 1);
    pbmd5_update_str(&md5, channel);
    pbmd5_update(&md5, s, 1);
    pbmd5_update_str(&md5, msg);
    s[0] = '\0';
    pbmd5_update(&md5, s, 1);

    pbmd5_final(&md5, digest);

    snprintf(signature, n,
        "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
        digest[0], digest[1], digest[2], digest[3],
        digest[4], digest[5], digest[6], digest[7],
        digest[8], digest[9], digest[10], digest[11],
        digest[12], digest[13], digest[14], digest[15]
    );
    return 0;
#endif /* !PUBNUB_CRYPTO_API */
}


static int cipher_hash(char const* cipher_key, uint8_t hash[33])
{
    uint8_t digest[32];
    pbsha256_digest_str(cipher_key, digest);

    snprintf((char*)hash, 33,
        "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
        digest[0], digest[1], digest[2], digest[3],
        digest[4], digest[5], digest[6], digest[7],
        digest[8], digest[9], digest[10], digest[11],
        digest[12], digest[13], digest[14], digest[15]
    );

    return 0;
}


int pubnub_encrypt(char const* cipher_key, pubnub_bymebl_t msg, char* base64_str, size_t* n)
{
    pubnub_bymebl_t encrypted;
    uint8_t key[33];
    int result;
    unsigned char iv[17] = "0123456789012345";
#if PUBNUB_RAND_INIT_VECTOR
    int rand_status = RAND_bytes(iv, 16);
    PUBNUB_ASSERT_OPT(rand_status == 1);
#endif

    cipher_hash(cipher_key, key);
    encrypted = pbaes256_encrypt_alloc(msg, key, iv);
    if (NULL == encrypted.ptr) {
        return -1;
    }

#if PUBNUB_RAND_INIT_VECTOR
    memmove(encrypted.ptr + 16, encrypted.ptr, encrypted.size);
    memcpy(encrypted.ptr, iv, 16);
    encrypted.size += 16;
    encrypted.ptr[encrypted.size] = '\0';
#endif

    #if PUBNUB_LOG_LEVEL >= PUBNUB_LOG_LEVEL_DEBUG
    PUBNUB_LOG_DEBUG("\nbytes before encoding iv + encrypted msg = [");
    for (int i = 0; i < (int)encrypted.size; i++) {
        PUBNUB_LOG_DEBUG("%d ", encrypted.ptr[i]);
    }
    PUBNUB_LOG_DEBUG("]\n");
    #endif

    int max_size = base64_max_size(encrypted.size);
    if (*n + 1 < (size_t)max_size) {
        PUBNUB_LOG_DEBUG("base64encode needs %d bytes but only %zu bytes are available\n", max_size, *n);
        return -1;
    }
    char* base64_output = (char*)malloc(max_size);
    if (base64encode(base64_output, max_size, encrypted.ptr, encrypted.size) != 0) {
        PUBNUB_LOG_DEBUG("base64encode tried to use more than %d bytes to encode %zu bytes\n", max_size, encrypted.size);
        free(base64_output);
        return -1;
    }
    result = snprintf(base64_str, *n, "%s", base64_output);
    *n = (size_t)strlen(base64_str);

    free(base64_output);
    free(encrypted.ptr);

    return result >= 0 ? 0 : -1;
}


int pubnub_encrypt_buffered(char const* cipher_key, pubnub_bymebl_t msg, char* base64_str, size_t* n, pubnub_bymebl_t buffer)
{
    uint8_t key[33];
    unsigned char iv[17] = "0123456789012345";
#if PUBNUB_RAND_INIT_VECTOR
    int rand_status = RAND_bytes(iv, 16);
    PUBNUB_ASSERT_OPT(rand_status == 1);
#endif

    cipher_hash(cipher_key, key);

    if (-1 == pbaes256_encrypt(msg, key, iv, &buffer)) {
        return -1;
    }

#if PUBNUB_RAND_INIT_VECTOR
    memmove(buffer.ptr + 16, buffer.ptr, buffer.size);
    memcpy(buffer.ptr, iv, 16);
    buffer.size += 16;
    buffer.ptr[buffer.size] = '\0';
#endif

    #if PUBNUB_LOG_LEVEL >= PUBNUB_LOG_LEVEL_DEBUG
    PUBNUB_LOG_DEBUG("\nbytes before encoding iv + encrypted msg = [");
    for (int i = 0; i < (int)buffer.size; i++) {
        PUBNUB_LOG_DEBUG("%d ", buffer.ptr[i]);
    }
    PUBNUB_LOG_DEBUG("]\n");
    #endif

    int max_size = base64_max_size(buffer.size);
    if (*n + 1 < (size_t)max_size) {
        PUBNUB_LOG_DEBUG("base64encode needs %d bytes but only %zu bytes are available\n", max_size, *n);
        return -1;
    }
    char* base64_output = (char*)malloc(max_size);
    if (base64encode(base64_output, max_size, buffer.ptr, buffer.size) != 0) {
        PUBNUB_LOG_DEBUG("base64encode tried to use more than %d bytes to encode %zu bytes\n", max_size, buffer.size);
        free(base64_output);
        return -1;
    }
    int result = snprintf(base64_str, *n, "%s", base64_output);
    *n = (size_t)strlen(base64_str);

    free(base64_output);

    return result >= 0 ? 0 : -1;
}



int pubnub_decrypt(char const* cipher_key, char const* base64_str, pubnub_bymebl_t* data)
{
    pubnub_bymebl_t decoded;
    unsigned char iv[17] = "0123456789012345";
    uint8_t key[33];

    cipher_hash(cipher_key, key);

    decoded = pbbase64_decode_alloc_std_str(base64_str);
    #if PUBNUB_LOG_LEVEL >= PUBNUB_LOG_LEVEL_DEBUG
    PUBNUB_LOG_DEBUG("\nbytes after decoding base64 string = [");
    for (size_t i = 0; i < decoded.size; i++) {
        PUBNUB_LOG_DEBUG("%d ", decoded.ptr[i]);
    }
    PUBNUB_LOG_DEBUG("]\n");
    #endif

    if (decoded.ptr != NULL) {
        int result;

#if PUBNUB_RAND_INIT_VECTOR
        memcpy(iv, decoded.ptr, 16);
        memmove(decoded.ptr, decoded.ptr + 16, decoded.size - 16);
        decoded.size = decoded.size - 16;
#endif
        decoded.ptr[decoded.size] = '\0';

        result = pbaes256_decrypt(decoded, key, iv, data);
        free(decoded.ptr);

        return result;
    }
    return -1;
}


int pubnub_decrypt_buffered(char const* cipher_key, char const* base64_str, pubnub_bymebl_t* data, pubnub_bymebl_t* buffer)
{
    if (0 == pbbase64_decode_std_str(base64_str, buffer)) {
        #if PUBNUB_LOG_LEVEL >= PUBNUB_LOG_LEVEL_DEBUG
        PUBNUB_LOG_DEBUG("\nbytes after decoding base64 string = [");
        for (size_t i = 0; i < buffer->size; i++) {
            PUBNUB_LOG_DEBUG("%d ", buffer->ptr[i]);
        }
        PUBNUB_LOG_DEBUG("]\n");
        #endif

        unsigned char iv[17] = "0123456789012345";
        uint8_t key[33];

        cipher_hash(cipher_key, key);
#if PUBNUB_RAND_INIT_VECTOR
        memcpy(iv, buffer->ptr, 16);
        memmove(buffer->ptr, buffer->ptr + 16, buffer->size - 16);
        buffer->size = buffer->size - 16;
#endif        
        buffer->ptr[buffer->size] = '\0';

        return pbaes256_decrypt(*buffer, key, iv, data);
    }

    return -1;
}


pubnub_bymebl_t pubnub_decrypt_alloc(char const* cipher_key, char const* base64_str)
{
    pubnub_bymebl_t decoded;
    unsigned char iv[17] = "0123456789012345";
    uint8_t key[33];

    cipher_hash(cipher_key, key);
    decoded = pbbase64_decode_alloc_std_str(base64_str);
    if (decoded.ptr != NULL) {
#if PUBNUB_RAND_INIT_VECTOR
        memcpy(iv, decoded.ptr, 16);
        memmove(decoded.ptr, decoded.ptr + 16, decoded.size - 16);
        decoded.size = decoded.size - 16;
#endif
        decoded.ptr[decoded.size] = '\0';

        pubnub_bymebl_t result;
        result = pbaes256_decrypt_alloc(decoded, key, iv);
        free(decoded.ptr);

        return result;
    }
    else{
        PUBNUB_LOG_ERROR("Failed to decode %s\n", base64_str);
    }

    return decoded;
}


char* pubnub_json_string_unescape_slash(char* json_string)
{
    char* s = json_string;
    size_t to_pad = 0;
    size_t distance;
    char* dest = s;
    char* end = s;

    while (*end) {
        if (('\\' == *end) && ('/' == end[1])) {
            distance = end - s;
            if (distance > 0) {
                memmove(dest, s, distance);
                dest += distance;
            }
            *dest++ = '/';
            end += 2;
            s = end;
            ++to_pad;
        }
        else {
            ++end;
        }
    }
    distance = end - s;
    if (distance > 0) {
        memmove(dest, s, distance);
        dest += distance;
    }

    if (to_pad > 0) {
        *dest = '\0';
    }
    PUBNUB_ASSERT_OPT(dest[to_pad] == '\0');

    return json_string;
}


enum pubnub_res pubnub_get_decrypted(pubnub_t* pb, char const* cipher_key, char* s, size_t* n)
{
    char* msg;
    size_t msg_len;
    uint8_t decoded_msg[PUBNUB_BUF_MAXLEN];
    pubnub_bymebl_t data = { (uint8_t*)s, *n };
    pubnub_bymebl_t buffer = { decoded_msg, PUBNUB_BUF_MAXLEN };

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_ASSERT_OPT(cipher_key != NULL);
    PUBNUB_ASSERT_OPT(s != NULL);
    PUBNUB_ASSERT_OPT(n != NULL);

    msg = (char*)pubnub_get(pb);
    if (NULL == msg) {
        return PNR_INTERNAL_ERROR;
    }
    msg_len = strlen(msg);
    if ((msg[0] != '"') || (msg[msg_len - 1] != '"')) {
        return PNR_FORMAT_ERROR;
    }
    msg[msg_len - 1] = '\0';
    ++msg;

    pubnub_json_string_unescape_slash(msg);

    if (0 != pubnub_decrypt_buffered(cipher_key, msg, &data, &buffer)) {
        return PNR_INTERNAL_ERROR;
    }
    *n = data.size;
    data.ptr[data.size] = '\0';

    return PNR_OK;
}


pubnub_bymebl_t pubnub_get_decrypted_alloc(pubnub_t* pb, char const* cipher_key)
{
    char* msg;
    size_t msg_len;
    pubnub_bymebl_t result = { NULL, 0 };

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_ASSERT_OPT(cipher_key != NULL);

    msg = (char*)pubnub_get(pb);
    if (NULL == msg) {
        return result;
    }
    msg_len = strlen(msg);
    if ((msg[0] != '"') || (msg[msg_len - 1] != '"')) {
        return result;
    }
    msg[msg_len - 1] = '\0';
    ++msg;

    pubnub_json_string_unescape_slash(msg);

    result = pubnub_decrypt_alloc(cipher_key, msg);
    if (NULL != result.ptr) {
        result.ptr[result.size] = '\0';
    }

    return result;
}


enum pubnub_res pubnub_publish_encrypted(pubnub_t* p, char const* channel, char const* message, char const* cipher_key)
{
    struct pubnub_publish_options opts = pubnub_publish_defopts();
    opts.cipher_key = cipher_key;
    return pubnub_publish_ex(p, channel, message, opts);
}


enum pubnub_res pubnub_set_secret_key(pubnub_t* p, char const* secret_key)
{
    PUBNUB_ASSERT_OPT(p != NULL);

#if PUBNUB_CRYPTO_API
    pubnub_mutex_lock(p->monitor);
    p->core.secret_key = secret_key;
    pubnub_mutex_unlock(p->monitor);

    return PNR_OK;
#else
    return PNR_CRYPTO_NOT_SUPPORTED;
#endif
}

#if __UWP__
int mx_hmac_sha256(
    const char* key,
    int keylen,
    const unsigned char* msg,
    size_t mlen,
    unsigned char** sig, size_t* slen) {
    /* Returned to caller */
    int result = -1;

    EVP_PKEY* pkey = EVP_PKEY_new_mac_key(EVP_PKEY_HMAC, NULL, (const unsigned char*)key, strlen(key));

    if (!msg || !mlen || !sig || !pkey) {
        printf("Param error: mx_hmac_sha256()\n");
        return -1;
    }

    if (*sig)
        OPENSSL_free(*sig);

    *sig = NULL;
    *slen = 0;

    EVP_MD_CTX* ctx = NULL;

    do
    {
        ctx = EVP_MD_CTX_create();
        if (ctx == NULL) {
            printf("EVP_MD_CTX_create failed, error 0x%x\n", ERR_get_error());
            break; /* failed */
        }

        OpenSSL_add_all_algorithms();
        const EVP_MD* md = EVP_get_digestbyname("SHA256");
        if (md == NULL) {
            printf("EVP_get_digestbyname failed, error 0x%x\n", ERR_get_error());
            break; /* failed */
        }

        int rc = EVP_DigestInit_ex(ctx, md, NULL);
        if (rc != 1) {
            printf("EVP_DigestInit_ex failed, error 0x%x\n", ERR_get_error());
            break; /* failed */
        }

        rc = EVP_DigestSignInit(ctx, NULL, md, NULL, pkey);
        if (rc != 1) {
            printf("EVP_DigestSignInit failed, error 0x%x\n", ERR_get_error());
            break; /* failed */
        }

        rc = EVP_DigestSignUpdate(ctx, msg, mlen);
        if (rc != 1) {
            printf("EVP_DigestSignUpdate failed, error 0x%x\n", ERR_get_error());
            break; /* failed */
        }

        size_t req = 0;
        rc = EVP_DigestSignFinal(ctx, NULL, &req);
        if (rc != 1) {
            printf("EVP_DigestSignFinal failed (1), error 0x%x\n", ERR_get_error());
            break; /* failed */
        }

        if (!(req > 0)) {
            printf("EVP_DigestSignFinal failed (2), error 0x%x\n", ERR_get_error());
            break; /* failed */
        }

        *sig = (char*)OPENSSL_malloc(req);
        if (*sig == NULL) {
            printf("OPENSSL_malloc failed, error 0x%x\n", ERR_get_error());
            break; /* failed */
        }

        *slen = req;
        rc = EVP_DigestSignFinal(ctx, (unsigned char*)*sig, slen);
        if (rc != 1) {
            printf("EVP_DigestSignFinal failed (3), return code %d, error 0x%x\n", rc, ERR_get_error());
            break; /* failed */
        }

        if (req != *slen) {
            printf("EVP_DigestSignFinal failed, mismatched signature sizes %d, %d\n", req, *slen);
            break; /* failed */
        }

        result = 0;

    } while (0);

    if (ctx) {
        EVP_MD_CTX_destroy(ctx);
        ctx = NULL;
    }

    /* Convert to 0/1 result */
    return !!result;
}
#else
unsigned char* mx_hmac_sha256(
    const void* key,
    int keylen,
    const unsigned char* data,
    int datalen,
    unsigned char* result, unsigned int* resultlen) {
    return HMAC(EVP_sha256(), key, keylen, data, datalen, result, resultlen);
}
#endif

int base64_max_size(int encode_this_many_bytes) {
    // Base64 overhead is up to 36%, so we allocate an extra 40% + a null character
    return (int)((float)encode_this_many_bytes * 1.4) + 1;
}

int base64encode(char* result, int max_size, const void* b64_encode_this, int encode_this_many_bytes) {
    size_t buf_size = 4 * ((encode_this_many_bytes + 2)/3) + 1; // +1 for tailing 0
    unsigned char *buf = (unsigned char*)calloc(buf_size, 1);
    if (!buf) {
        return -1;
    }

    size_t size = EVP_EncodeBlock(buf, (const unsigned char*)b64_encode_this, encode_this_many_bytes);
    // size does not include tailing 0
    // https://www.openssl.org/docs/man3.0/man3/EVP_EncodeBlock.html
    if (size + 1 > (size_t)max_size) {
        free(buf);
        return -1;
    }
    memcpy(result, buf, size + 1);
    free(buf);
    return 0;
}

char* pn_pam_hmac_sha256_sign(char const* key, char const* message) {
    int keylen = strlen(key);
    const unsigned char* msgdata = (const unsigned char*)strdup(message);
    if (NULL == msgdata) { return NULL; }

    int datalen = strlen((char*)msgdata);
#if __UWP__
    unsigned char* result = NULL;
    size_t* resultlen = 0;
    int ret = mx_hmac_sha256(key, keylen, (const unsigned char*)msgdata, (size_t)datalen, &result, (size_t*)&resultlen);
    if (ret != 0) { return NULL; }
#else
    unsigned char* result = NULL;
    unsigned int resultlen = -1;
    result = mx_hmac_sha256((const void*)key, keylen, msgdata, datalen, result, &resultlen);
#endif
    int bytes_to_encode = (int)resultlen;
    if (bytes_to_encode <= 0) {
        PUBNUB_LOG_DEBUG("hmac_sha256 result len %d is low\n", bytes_to_encode);
        return NULL;
    }
    int max_size = base64_max_size(bytes_to_encode);
    char* base64_encoded = (char*)malloc(max_size);
    if (base64encode(base64_encoded, max_size, result, bytes_to_encode) != 0) {
        PUBNUB_LOG_DEBUG("base64encode tried to use more than %d bytes to encode %d bytes\n", max_size, bytes_to_encode);
        free(base64_encoded);
        return NULL;
    }
    int counter = 0;
    while (base64_encoded[counter] != '\0') {
        if (base64_encoded[counter] == '+') {
            base64_encoded[counter] = '-';
        }
        else if (base64_encoded[counter] == '/') {
            base64_encoded[counter] = '_';
        }
        counter++;
    }
    free((unsigned char*)msgdata);
    return base64_encoded;
}

enum pubnub_res pn_gen_pam_v2_sign(pubnub_t* p, char const* qs_to_sign, char const* partial_url, char* signature) {
    enum pubnub_res sign_status = PNR_OK;
    int str_to_sign_len = strlen(p->core.subscribe_key) + strlen(p->core.publish_key) + strlen(partial_url) + strlen(qs_to_sign);
    char* str_to_sign = (char*)malloc(sizeof(char) * str_to_sign_len + 5); // 4 variables concat + 1
    if (str_to_sign != NULL) {
        sprintf(str_to_sign, "%s\n%s\n%s\n%s", p->core.subscribe_key, p->core.publish_key, partial_url, qs_to_sign);
    }
    PUBNUB_LOG_DEBUG("\nv2 str_to_sign = %s\n", str_to_sign);
    char* part_sign = (char*)"";
#if PUBNUB_CRYPTO_API
    part_sign = pn_pam_hmac_sha256_sign(p->core.secret_key, str_to_sign);
    if (NULL == part_sign) { sign_status = PNR_CRYPTO_NOT_SUPPORTED; }
#else
    sign_status = PNR_CRYPTO_NOT_SUPPORTED;
#endif
    free((char*)str_to_sign);
    if (sign_status == PNR_OK) {
        sprintf(signature, "%s", part_sign);
    }
    free(part_sign);
    return sign_status;
}

enum pubnub_res pn_gen_pam_v3_sign(pubnub_t* p, char const* qs_to_sign, char const* partial_url, char const* msg, char* signature) {
    enum pubnub_res sign_status = PNR_OK;
    bool hasBody = false;
    char* method_verb;
    switch (p->method) {
    case pubnubSendViaGET:
        method_verb = (char*)"GET";
        break;
    case pubnubSendViaPOST:
#if PUBNUB_USE_GZIP_COMPRESSION
    case pubnubSendViaPOSTwithGZIP:
#endif
        method_verb = (char*)"POST";
        hasBody = true;
        break;
    case pubnubUsePATCH:
#if PUBNUB_USE_GZIP_COMPRESSION
    case pubnubUsePATCHwithGZIP:
#endif
        method_verb = (char*)"PATCH";
        hasBody = true;
        break;
    case pubnubUseDELETE:
        method_verb = (char*)"DELETE";
        break;
    default:
        PUBNUB_LOG_ERROR("Error: get_method_verb_string(method): unhandled method: %u\n", p->method);
        method_verb = (char*)"UNKOWN";
        return PNR_CRYPTO_NOT_SUPPORTED;
    }
    int str_to_sign_len = strlen(method_verb) + strlen(p->core.publish_key) + strlen(partial_url) + strlen(qs_to_sign) + 4 * strlen("\n") + (hasBody ? strlen(msg) : 0);
    char* str_to_sign = (char*)malloc(sizeof(char) * (str_to_sign_len + 1));
    if (str_to_sign != NULL) {
        if (hasBody) {
            sprintf(str_to_sign, "%s\n%s\n%s\n%s\n%s", method_verb, p->core.publish_key, partial_url, qs_to_sign, msg);
        }
        else {
            sprintf(str_to_sign, "%s\n%s\n%s\n%s\n", method_verb, p->core.publish_key, partial_url, qs_to_sign);
        }
    }
    PUBNUB_LOG_DEBUG("\nv3 str_to_sign = %s\n", str_to_sign);
    char* part_sign = (char*)"";
#if PUBNUB_CRYPTO_API
    part_sign = pn_pam_hmac_sha256_sign(p->core.secret_key, str_to_sign);
    if (NULL == part_sign) { sign_status = PNR_CRYPTO_NOT_SUPPORTED; }
#else
    sign_status = PNR_CRYPTO_NOT_SUPPORTED;
#endif
    free((char*)str_to_sign);
    if (sign_status == PNR_OK) {
        char last_sign_char = part_sign[strlen(part_sign) - 1];
        while (last_sign_char == '=') {
            part_sign[strlen(part_sign) - 1] = '\0';
            last_sign_char = part_sign[strlen(part_sign) - 1];
        }
        sprintf(signature, "v2.%s", part_sign);
    }
    free(part_sign);
    return sign_status;
}

