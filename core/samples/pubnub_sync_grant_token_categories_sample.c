/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_sync.h"

#include "core/pubnub_helper.h"
#include "core/pubnub_crypto.h"
#include "core/pubnub_grant_token_api.h"
#include "core/pubnub_json_parse.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// snippet.parseTokenCategories
// Prints the category permissions of a token, `cat` section of the parsed token.
// Each value is the permission bit mask, 32 is the `get` permission.
static void print_token_categories(pubnub_t* pb, char const* token)
{
    char* parsed = pubnub_parse_token(pb, token);
    if (NULL == parsed) {
        printf("pubnub_parse_token() failed\n");
        return;
    }

    struct pbjson_elem parsed_elem = { parsed, parsed + strlen(parsed) };
    struct pbjson_elem categories;
    struct pbjson_elem channels;
    struct pbjson_elem uuids;

    if (jonmpOK != pbjson_get_object_value(&parsed_elem, "cat", &categories)) {
        printf("The token has no category permissions\n");
    }
    else {
        if (jonmpOK == pbjson_get_object_value(&categories, "chan", &channels)) {
            printf("List all channel metadata: %s\n",
                   pbjson_elem_equals_string(&channels, "32") ? "yes" : "no");
        }
        if (jonmpOK == pbjson_get_object_value(&categories, "uuid", &uuids)) {
            printf("List all uuid metadata: %s\n",
                   pbjson_elem_equals_string(&uuids, "32") ? "yes" : "no");
        }
    }

    free(parsed);
}
// snippet.end

// snippet.grantTokenCategories
// Grants a token that can list all channel metadata and all uuid metadata.
// The categories permissions need no resources, `32` is the only valid value (`get`).
static enum pubnub_res grant_token_categories(pubnub_t* pb)
{
    char const*     perm_obj =
        "{\"ttl\":60, \"uuid\":\"my_authorized_uuid\", \"permissions\":{"
        "\"categories\":{\"channels\":32, \"uuids\":32}}}";
    enum pubnub_res res = pubnub_grant_token(pb, perm_obj);

    if (PNR_STARTED == res) {
        res = pubnub_await(pb);
    }
    if (PNR_OK == res) {
        pubnub_chamebl_t grant_token = pubnub_get_grant_token(pb);
        printf("pubnub_grant_token() Response from Pubnub: %s\n", grant_token.ptr);
    }
    else {
        printf("pubnub_grant_token() failed with code: %d('%s')\n",
               res,
               pubnub_res_2_string(res));
    }

    return res;
}
// snippet.end

int main()
{
    char const*     my_env_publish_key   = getenv("PUBNUB_PUBLISH_KEY");
    char const*     my_env_subscribe_key = getenv("PUBNUB_SUBSCRIBE_KEY");
    char const*     my_env_secret_key    = getenv("PUBNUB_SECRET_KEY");
    pubnub_t*       pbp                  = pubnub_alloc();
    enum pubnub_res res;

    if (NULL == pbp) {
        printf("Failed to allocate Pubnub context!\n");
        return -1;
    }

    if (NULL == my_env_publish_key) { my_env_publish_key = "demo"; }
    if (NULL == my_env_subscribe_key) { my_env_subscribe_key = "demo"; }
    if (NULL == my_env_secret_key) { my_env_secret_key = "demo"; }

    pubnub_init(pbp, my_env_publish_key, my_env_subscribe_key);
    pubnub_set_secret_key(pbp, my_env_secret_key);
    pubnub_set_user_id(pbp, "my_user_id");

    res = grant_token_categories(pbp);
    if (PNR_OK == res) {
        /* The token stays valid until the next transaction on the context */
        print_token_categories(pbp, pubnub_get_grant_token(pbp).ptr);
    }

    if (PN_CANCEL_STARTED == pubnub_cancel(pbp)) {
        pubnub_await(pbp);
    }
    pubnub_free(pbp);

    return (PNR_OK == res) ? 0 : 1;
}
