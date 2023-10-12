/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pbaes256.h"

#include "core/pubnub_log.h"
#include "core/pubnub_assert.h"

#include "openssl/pubnub_config.h"
#include <openssl/evp.h>
#include <openssl/err.h>

#include <stdlib.h>
#include <string.h>

static int print_to_pubnub_log(const char* s, size_t len, void* p)
{
    PUBNUB_UNUSED(len);
    PUBNUB_UNUSED(p);

    PUBNUB_LOG_ERROR("%s", s);

    return 0;
}


static int do_encrypt(EVP_CIPHER_CTX* aes256, pubnub_bymebl_t msg, uint8_t const* key, uint8_t const* iv, pubnub_bymebl_t* encrypted)
{
    int len = 0;

    if (!EVP_EncryptInit_ex(aes256, EVP_aes_256_cbc(), NULL, key, iv)) {
        ERR_print_errors_cb(print_to_pubnub_log, NULL);
        PUBNUB_LOG_ERROR("Failed to initialize AES-256 encryption\n");
        return -1;
    }
    if (!EVP_EncryptUpdate(aes256, encrypted->ptr, &len, msg.ptr, msg.size)) {
        ERR_print_errors_cb(print_to_pubnub_log, NULL);
        PUBNUB_LOG_ERROR("Failed to AES-256 encrypt the mesage\n");
        return -1;
    }
    encrypted->size = len;
    if (!EVP_EncryptFinal_ex(aes256, encrypted->ptr + len, &len)) {
        ERR_print_errors_cb(print_to_pubnub_log, NULL);
        PUBNUB_LOG_ERROR("Failed to finalize AES-256 encryption\n");
        return -1;
    }
    encrypted->size += len;

    return 0;
}


int pbaes256_encrypt(pubnub_bymebl_t msg, uint8_t const* key, uint8_t const* iv, pubnub_bymebl_t* encrypted)
{
    int result;
    EVP_CIPHER_CTX* aes256 = EVP_CIPHER_CTX_new();

    if (NULL == aes256) {
        PUBNUB_LOG_ERROR("Failed to allocate AES-256 encryption context\n");
        return -1;
    }

    result = do_encrypt(aes256, msg, key, iv, encrypted);

    EVP_CIPHER_CTX_free(aes256);

    return result;
}


pubnub_bymebl_t pbaes256_encrypt_alloc(pubnub_bymebl_t msg, uint8_t const* key, uint8_t const* iv)
{
    int encrypt_result;
    pubnub_bymebl_t result = { NULL, 0 };
    EVP_CIPHER_CTX* aes256 = EVP_CIPHER_CTX_new();

    if (NULL == aes256) {
        PUBNUB_LOG_ERROR("Failed to allocate AES-256 encryption context\n");
        return result;
    }

#if PUBNUB_RAND_INIT_VECTOR
    result.ptr = (uint8_t*)malloc(msg.size + EVP_CIPHER_block_size(EVP_aes_256_cbc())*2);
#else
    result.ptr = (uint8_t*)malloc(msg.size + EVP_CIPHER_block_size(EVP_aes_256_cbc()));
#endif

    if (NULL == result.ptr) {
        EVP_CIPHER_CTX_free(aes256);
        PUBNUB_LOG_ERROR("Failed to allocate memory for AES-256 encryption\n");
        return result;
    }

    encrypt_result = do_encrypt(aes256, msg, key, iv, &result);
    if (-1 == encrypt_result) {
        free(result.ptr);
        result.ptr = NULL;
    }
    EVP_CIPHER_CTX_free(aes256);

    return result;
}


static int do_decrypt(EVP_CIPHER_CTX* aes256, pubnub_bymebl_t data, uint8_t const* key, uint8_t const* iv, pubnub_bymebl_t* msg)
{
    int len = 0;
    uint8_t terminated_iv[17];
    memcpy(terminated_iv, iv, 16);
    terminated_iv[16] = '\0';

    if (!EVP_DecryptInit_ex(aes256, EVP_aes_256_cbc(), NULL, key, terminated_iv)) {
        ERR_print_errors_cb(print_to_pubnub_log, NULL);
        PUBNUB_LOG_ERROR("Failed to initialize AES-256 decryption\n");
        return -1;
    }

    if (!EVP_DecryptUpdate(aes256, msg->ptr, &len, data.ptr, data.size)) {
        ERR_print_errors_cb(print_to_pubnub_log, NULL);
        PUBNUB_LOG_ERROR("Failed to AES-256 decrypt the mesage\n");
        return -1;
    }
    msg->size = len;

    if (!EVP_DecryptFinal_ex(aes256, msg->ptr + len, &len)) {
        ERR_print_errors_cb(print_to_pubnub_log, NULL);
        PUBNUB_LOG_ERROR("Failed to finalize AES-256 decryption\n");
        return -1;
    }
    msg->size += len;
    msg->ptr[msg->size] = '\0';
    return 0;
}


int pbaes256_decrypt(pubnub_bymebl_t data, uint8_t const* key, uint8_t const* iv, pubnub_bymebl_t* msg)
{
    int result;
    EVP_CIPHER_CTX* aes256;

    if (msg->size < data.size + EVP_CIPHER_block_size(EVP_aes_256_cbc()) + 1) {
        PUBNUB_LOG_ERROR("Not enough room to save AES-256 decrypted data\n");
        return -1;
    }

    aes256 = EVP_CIPHER_CTX_new();
    if (NULL == aes256) {
        PUBNUB_LOG_ERROR("Failed to allocate AES-256 decryption context\n");
        return -1;
    }

    result = do_decrypt(aes256, data, key, iv, msg);
    
    EVP_CIPHER_CTX_free(aes256);

    return result;
}


pubnub_bymebl_t pbaes256_decrypt_alloc(pubnub_bymebl_t data, uint8_t const* key, uint8_t const* iv)
{
    int decrypt_result;
    EVP_CIPHER_CTX* aes256;
    pubnub_bymebl_t result;

    result.size = data.size + EVP_CIPHER_block_size(EVP_aes_256_cbc()) + 1;
    result.ptr = (uint8_t*)malloc(result.size);
    if (NULL == result.ptr) {
        return result;
    }

    aes256 = EVP_CIPHER_CTX_new();
    if (NULL == aes256) {
        PUBNUB_LOG_ERROR("Failed to allocate AES-256 decryption context\n");
        free(result.ptr);
        result.ptr = NULL;
        return result;;
    }

    decrypt_result = do_decrypt(aes256, data, key, iv, &result);

    EVP_CIPHER_CTX_free(aes256);

    if (decrypt_result != 0) {
        PUBNUB_LOG_ERROR("Failed AES-256 decryption\n");
        free(result.ptr);
        result.ptr = NULL;
        result.size = 0;
        return result;
    }
    result.ptr[result.size] = '\0';
    return result;
}
