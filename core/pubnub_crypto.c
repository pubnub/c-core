/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_crypto.h"

#include "pubnub_assert.h"
#include "pubnub_internal.h"
#include "pubnub_coreapi.h"
#include "pubnub_coreapi_ex.h"

#include "pbmd5.h"
#include "pbsha256.h"
#include "pbaes256.h"
#include "pbbase64.h"



int pbcrypto_signature(struct pbcc_context *pbcc, char const *channel, char const* msg, char *signature, size_t n)
{
    PBMD5_CTX md5;
    char s[2] = { '/', '\0' };
    uint8_t digest[16];

    PUBNUB_ASSERT_OPT(channel != NULL);
    PUBNUB_ASSERT_OPT(msg != NULL);
    PUBNUB_ASSERT_OPT(signature != NULL);
    PUBNUB_ASSERT_OPT(n > 32);

    if (!PUBNUB_CRYPTO_API || (NULL == pbcc->secret_key)) {
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
             "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
             digest[0], digest[1], digest[2], digest[3],
             digest[4], digest[5], digest[6], digest[7],
             digest[8], digest[9], digest[10], digest[11],
             digest[12], digest[13], digest[14], digest[15]
        );

    return 0;
}


static int cipher_hash(char const* cipher_key, uint8_t hash[33])
{
    uint8_t digest[32];
    pbsha256_digest_str(cipher_key, digest);

    snprintf((char*)hash, 33, 
             "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
             digest[0], digest[1], digest[2], digest[3],
             digest[4], digest[5], digest[6], digest[7],
             digest[8], digest[9], digest[10], digest[11],
             digest[12], digest[13], digest[14], digest[15]
        );

    return 0;
}


int pubnub_encrypt(char const *cipher_key, pubnub_bymebl_t msg, char *base64_str, size_t *n)
{
    pubnub_bymebl_t encrypted;
    uint8_t const iv[] = "0123456789012345";
    uint8_t key[33];
    int result;

    cipher_hash(cipher_key, key);
    encrypted = pbaes256_encrypt_alloc(msg, key, iv);
    if (NULL == encrypted.ptr) {
        return -1;
    }
    result = pbbase64_encode_std(encrypted, base64_str, n);
    free(encrypted.ptr);

    return result;
}


int pubnub_encrypt_buffered(char const *cipher_key, pubnub_bymebl_t msg, char *base64_str, size_t *n, pubnub_bymebl_t buffer)
{
    uint8_t const iv[] = "0123456789012345";
    uint8_t key[33];

    cipher_hash(cipher_key, key);
    if (-1 == pbaes256_encrypt(msg, key, iv, &buffer)) {
        return -1;
    }
    return pbbase64_encode_std(buffer, base64_str, n);
}



int pubnub_decrypt(char const *cipher_key, char const *base64_str, pubnub_bymebl_t *data)
{
    pubnub_bymebl_t decoded;
    uint8_t iv[] = "0123456789012345";
    uint8_t key[33];

    cipher_hash(cipher_key, key);
    decoded = pbbase64_decode_alloc_std_str(base64_str);
    if (decoded.ptr != NULL) {
        int result;

        decoded.ptr[decoded.size] = '\0';
        result = pbaes256_decrypt(decoded, key, iv, data);
        free(decoded.ptr);

        return result;
    }

    return -1;
}


int pubnub_decrypt_buffered(char const *cipher_key, char const *base64_str, pubnub_bymebl_t *data, pubnub_bymebl_t *buffer)
{
    if (0 == pbbase64_decode_std_str(base64_str, buffer)) {
        uint8_t const iv[] = "0123456789012345";
        uint8_t key[33];

        cipher_hash(cipher_key, key);
        buffer->ptr[buffer->size] = '\0';

        return pbaes256_decrypt(*buffer, key, iv, data);
    }

    return -1;
}


pubnub_bymebl_t pubnub_decrypt_alloc(char const *cipher_key, char const *base64_str)
{
    pubnub_bymebl_t decoded;
    uint8_t iv[] = "0123456789012345";
    uint8_t key[33];

    cipher_hash(cipher_key, key);
    decoded = pbbase64_decode_alloc_std_str(base64_str);
    if (decoded.ptr != NULL) {
        pubnub_bymebl_t result;

        decoded.ptr[decoded.size] = '\0';
        result = pbaes256_decrypt_alloc(decoded, key, iv);
        free(decoded.ptr);

        return result;
    }

    return decoded;
}


enum pubnub_res pubnub_get_decrypted(pubnub_t *pb, char const* cipher_key, char *s, size_t *n)
{
    char *msg;
    size_t msg_len;
    uint8_t decoded_msg[PUBNUB_BUF_MAXLEN];
    pubnub_bymebl_t data = { (uint8_t*)s, *n };
    pubnub_bymebl_t buffer = { decoded_msg, PUBNUB_BUF_MAXLEN };

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_ASSERT_OPT(cipher_key != NULL);
    PUBNUB_ASSERT_OPT(s != NULL);
    PUBNUB_ASSERT_OPT(n > 0);

    msg = (char*)pubnub_get(pb);
    if (NULL == msg) {
        return PNR_INTERNAL_ERROR;
    }
    msg_len = strlen(msg);
    if ((msg[0] != '"') || (msg[msg_len-1] != '"')) {
        return PNR_FORMAT_ERROR;
    }
    msg[msg_len - 1] = '\0';

    if (0 != pubnub_decrypt_buffered(cipher_key, msg + 1, &data, &buffer)) {
        return PNR_INTERNAL_ERROR;
    }
    *n = data.size;
    data.ptr[data.size] = '\0';

    return PNR_OK;
}


pubnub_bymebl_t pubnub_get_decrypted_alloc(pubnub_t *pb, char const* cipher_key)
{
    char *msg;
    size_t msg_len;
    pubnub_bymebl_t result = { NULL, 0 };

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_ASSERT_OPT(cipher_key != NULL);

    msg = (char*)pubnub_get(pb);
    if (NULL == msg) {
        return result;
    }
    msg_len = strlen(msg);
    if ((msg[0] != '"') || (msg[msg_len-1] != '"')) {
        return result;
    }
    msg[msg_len - 1] = '\0';

    result = pubnub_decrypt_alloc(cipher_key, msg + 1);
    if (NULL != result.ptr) {
        result.ptr[result.size] = '\0';
    }

    return result;
}


enum pubnub_res pubnub_publish_encrypted(pubnub_t *p, char const* channel, char const* message, char const* cipher_key)
{
    struct pubnub_publish_options opts =  pubnub_publish_defopts();
    opts.cipher_key = cipher_key;
    return pubnub_publish_ex(p, channel, message, opts);
}


enum pubnub_res pubnub_set_secret_key(pubnub_t *p, char const* secret_key)
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

