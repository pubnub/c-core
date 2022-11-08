/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_sync.h"

#include "core/pubnub_helper.h"
#include "core/pubnub_timers.h"
#include "core/pubnub_crypto.h"
#include "core/pubnub_objects_api.h"

#include <stdio.h>
#include <time.h>


static void generate_user_id(pubnub_t* pbp)
{
    char const*                      user_id_default = "zeka-peka-iz-jendeka";
    struct Pubnub_UUID               uuid;
    static struct Pubnub_UUID_String str_uuid;

    if (0 != pubnub_generate_uuid_v4_random(&uuid)) {
        pubnub_set_uuid(pbp, user_id_default);
    }
    else {
        str_uuid = pubnub_uuid_to_string(&uuid);
        pubnub_set_uuid(pbp, str_uuid.uuid);
        printf("Generated UUID: %s\n", str_uuid.uuid);
    }
}

static void sync_sample_free(pubnub_t* p)
{
    if (PN_CANCEL_STARTED == pubnub_cancel(p)) {
        enum pubnub_res pnru = pubnub_await(p);
        if (pnru != PNR_OK) {
            printf("Awaiting cancel failed: %d('%s')\n",
                   pnru,
                   pubnub_res_2_string(pnru));
        }
    }
    if (pubnub_free(p) != 0) {
        printf("Failed to free the Pubnub context\n");
    }
}

int main()
{
    time_t          t0;
    enum pubnub_res res;
    pubnub_t*       pbp  = pubnub_alloc();
    const char* uuid_metadata_id = "pandu_sample_uid";
    const char* ch_metadata_id = "pandu_sample_sid";

    if (NULL == pbp) {
        printf("Failed to allocate Pubnub context!\n");
        return -1;
    }

    char* my_env_publish_key = getenv("PUBNUB_PUBLISH_KEY");
    char* my_env_subscribe_key = getenv("PUBNUB_SUBSCRIBE_KEY");
    char* my_env_secret_key = getenv("PUBNUB_SECRET_KEY");

    if (NULL == my_env_publish_key) { my_env_publish_key = "demo"; }
    if (NULL == my_env_subscribe_key) { my_env_subscribe_key = "demo"; }
    if (NULL == my_env_secret_key) { my_env_secret_key = "demo"; }

    printf("%s\n%s\n%s\n",my_env_publish_key,my_env_subscribe_key,my_env_secret_key);

    pubnub_init(pbp, my_env_publish_key, my_env_subscribe_key);
    pubnub_set_secret_key(pbp, my_env_secret_key);

    pubnub_set_transaction_timeout(pbp, PUBNUB_DEFAULT_SUBSCRIBE_TIMEOUT);

    /* Leave this commented out to use the default - which is
       blocking I/O on most platforms. Uncomment to use non-
       blocking I/O.
    */
    pubnub_set_non_blocking_io(pbp);

    generate_user_id(pbp);

    /********** GET ALL UUID METADATA   **********/
    puts("Get All UUID Metadata...");
    time(&t0);
    const char* include = "custom";
    res = pubnub_getall_uuidmetadata(pbp, include, 0, NULL, NULL, pbccTrue);
    if (PNR_STARTED == res) {
        res = pubnub_await(pbp);
    }
    printf("pubnub_getall_uuidmetadata() lasted %lf seconds.\n", difftime(time(NULL), t0));
    if (PNR_OK == res) {
        char const* getall_uuidmetadata_resp = pubnub_get(pbp);
        printf("pubnub_getall_uuidmetadata() Response from Pubnub: %s\n",
               getall_uuidmetadata_resp);
    }
    else if (PNR_OBJECTS_API_ERROR == res) {
        char const* getall_uuidmetadata_resp = pubnub_get(pbp);
        printf("pubnub_getall_uuidmetadata() failed on Pubnub, description: %s\n",
               getall_uuidmetadata_resp);
    }
    else {
        printf("pubnub_getall_uuidmetadata() failed with code: %d('%s')\n",res,pubnub_res_2_string(res));
    }
    printf("\n\n");

    /********** GET UUID METADATA   **********/
    puts("Get Single UUID Metadata...");
    time(&t0);
    include = "custom";
    res = pubnub_get_uuidmetadata(pbp, include, uuid_metadata_id);
    if (PNR_STARTED == res) {
        res = pubnub_await(pbp);
    }
    printf("pubnub_get_uuidmetadata() lasted %lf seconds.\n", difftime(time(NULL), t0));
    if (PNR_OK == res) {
        char const* get_uuidmetadata_resp = pubnub_get(pbp);
        printf("pubnub_get_uuidmetadata() Response from Pubnub: %s\n",get_uuidmetadata_resp);
    }
    else {
        printf("pubnub_get_uuidmetadata() failed with code: %d('%s')\n",res,pubnub_res_2_string(res));
    }
    printf("\n\n");


    /********** REMOVE UUID METADATA   **********/
    puts("Remove UUID Metadata if exists before setting");
    time(&t0);
    res = pubnub_remove_uuidmetadata(pbp, uuid_metadata_id);
    if (PNR_STARTED == res) {
        res = pubnub_await(pbp);
    }
    printf("pubnub_remove_uuidmetadata() lasted %lf seconds.\n", difftime(time(NULL), t0));
    if (PNR_OK == res) {
        char const* remove_uuidmetadata_resp = pubnub_get(pbp);
        printf("pubnub_remove_uuidmetadata() Response from Pubnub: %s\n",
               remove_uuidmetadata_resp);
    }
    else {
        printf("remove_uuidmetadata_resp() failed with code: %d('%s')\n",res,pubnub_res_2_string(res));
    }
    printf("\n\n");
    
    /********** SET UUID METADATA   **********/
    puts("Set UUID Metadata...");
    time(&t0);
    char set_uuidmeta_json[20000] = "{\"name\":\"ccoreuuid\",\"externalId\":\"ccore-external-id\",\"profileUrl\":\"ccore@profileurl.com\",\"email\":\"ccore@test.com\",\"custom\":{\"color\":\"red\"}}";
    res = pubnub_set_uuidmetadata(pbp, uuid_metadata_id, NULL, set_uuidmeta_json);;
    if (PNR_STARTED == res) {
        res = pubnub_await(pbp);
    }
    printf("pubnub_set_uuidmetadata() lasted %lf seconds.\n", difftime(time(NULL), t0));
    if (PNR_OK == res) {
        char const* set_uuidmetadata_resp = pubnub_get(pbp);
        printf("pubnub_set_uuidmetadata() Response from Pubnub: %s\n",
               set_uuidmetadata_resp);
    }
    else {
        printf("pubnub_set_uuidmetadata() failed with code: %d('%s')\n",
               res,
               pubnub_res_2_string(res));
    }
    printf("\n\n");

    /********** GET ALL CHANNEL METADATA   **********/
    puts("Get All Channel Metadata...");
    time(&t0);
    include = "custom";
    res = pubnub_getall_channelmetadata(pbp, include, 0, NULL, NULL, pbccTrue);
    if (PNR_STARTED == res) {
        res = pubnub_await(pbp);
    }
    printf("pubnub_getall_channelmetadata() lasted %lf seconds.\n", difftime(time(NULL), t0));
    if (PNR_OK == res) {
        char const* getall_channelmetadata_resp = pubnub_get(pbp);
        printf("pubnub_getall_channelmetadata() Response from Pubnub: %s\n",getall_channelmetadata_resp);
    }
    else if (PNR_OBJECTS_API_ERROR == res) {
        char const* getall_channelmetadata_resp = pubnub_get(pbp);
        printf("pubnub_getall_channelmetadata() failed on Pubnub, description: %s\n",getall_channelmetadata_resp);
    }
    else {
        printf("pubnub_getall_channelmetadata() failed with code: %d('%s')\n",res,pubnub_res_2_string(res));
    }
    printf("\n\n");

    /********** GET CHANNEL METADATA   **********/
    puts("Get Single Channel Metadata...");
    time(&t0);
    include = "custom";
    res = pubnub_get_channelmetadata(pbp, include, ch_metadata_id);
    if (PNR_STARTED == res) {
        res = pubnub_await(pbp);
    }
    printf("pubnub_get_channelmetadata() lasted %lf seconds.\n", difftime(time(NULL), t0));
    if (PNR_OK == res) {
        char const* get_channelmetadata_resp = pubnub_get(pbp);
        printf("pubnub_get_channelmetadata() Response from Pubnub: %s\n",get_channelmetadata_resp);
    }
    else {
        printf("pubnub_get_channelmetadata() failed with code: %d('%s')\n",res,pubnub_res_2_string(res));
    }
    printf("\n\n");


    /********** REMOVE CHANNEL METADATA   **********/
    puts("Remove Channel Metadata if exists before setting");
    time(&t0);
    res = pubnub_remove_channelmetadata(pbp, ch_metadata_id);
    if (PNR_STARTED == res) {
        res = pubnub_await(pbp);
    }
    printf("pubnub_remove_channelmetadata() lasted %lf seconds.\n", difftime(time(NULL), t0));
    if (PNR_OK == res) {
        char const* remove_channelmetadata_resp = pubnub_get(pbp);
        printf("pubnub_remove_channelmetadata() Response from Pubnub: %s\n",remove_channelmetadata_resp);
    }
    else {
        printf("pubnub_remove_channelmetadata() failed with code: %d('%s')\n",res,pubnub_res_2_string(res));
    }
    printf("\n\n");
    
    /********** SET CHANNEL METADATA   **********/
    puts("Set CHANNEL Metadata...");
    time(&t0);
    include = "custom";
    char set_channelmeta_json[20000] = "{\"name\":\"ccorechid\",\"description\":\"ccore-channel-desc\",\"custom\":{\"foo\":\"bar\"}}";
    res = pubnub_set_channelmetadata(pbp, ch_metadata_id, include, set_channelmeta_json);
    if (PNR_STARTED == res) {
        res = pubnub_await(pbp);
    }
    printf("pubnub_set_channelmetadata() lasted %lf seconds.\n", difftime(time(NULL), t0));
    if (PNR_OK == res) {
        char const* set_channelmetadata_resp = pubnub_get(pbp);
        printf("pubnub_set_channelmetadata() Response from Pubnub: %s\n",set_channelmetadata_resp);
    }
    else {
        printf("pubnub_set_channelmetadata() failed with code: %d('%s')\n",res,pubnub_res_2_string(res));
    }
    printf("\n\n");


    /********** GET MEMBERSHIPS   **********/
    puts("Get Memberships based on UUID MetadataId...");
    time(&t0);
    include = "custom,channel,channel_custom";
    res = pubnub_get_memberships(pbp, uuid_metadata_id, include, 5, NULL, NULL, pbccTrue);
    if (PNR_STARTED == res) {
        res = pubnub_await(pbp);
    }
    printf("pubnub_get_memberships() lasted %lf seconds.\n", difftime(time(NULL), t0));
    if (PNR_OK == res) {
        char const* get_memberships_resp = pubnub_get(pbp);
        printf("pubnub_get_memberships() Response from Pubnub: %s\n",get_memberships_resp);
    }
    else {
        printf("pubnub_get_memberships() failed with code: %d('%s')\n",res,pubnub_res_2_string(res));
    }
    printf("\n\n");


/********** SET MEMBERSHIPS   **********/
    puts("SET Memberships based on UUID MetadataId...");
    time(&t0);
    include = "custom,channel,channel_custom";
    char set_membership_json[2000];
    sprintf(set_membership_json, "[{\"channel\":{\"id\":\"%s\"},\"custom\":{\"item\":\"book\"}},{\"channel\":{\"id\":\"new-%s\"},\"custom\":{\"itemnew\":\"booknew\"}}]",ch_metadata_id,ch_metadata_id);
    res = pubnub_set_memberships(pbp, uuid_metadata_id, include, set_membership_json);
    if (PNR_STARTED == res) {
        res = pubnub_await(pbp);
    }
    printf("pubnub_set_memberships() lasted %lf seconds.\n", difftime(time(NULL), t0));
    if (PNR_OK == res) {
        char const* set_memberships_resp = pubnub_get(pbp);
        printf("pubnub_set_memberships() Response from Pubnub: %s\n",set_memberships_resp);
    }
    else {
        printf("pubnub_set_memberships() failed with code: %d('%s')\n",res,pubnub_res_2_string(res));
    }
    printf("\n\n");


/********** REMOVE MEMBERSHIPS   **********/
    puts("REMOVE Memberships based on UUID MetadataId...");
    time(&t0);
    include = "custom,channel,channel_custom";
    char remove_membership_json[2000];
    sprintf(remove_membership_json, "[{\"channel\":{\"id\":\"%s\"}},{\"channel\":{\"id\":\"new-%s\"}}]",ch_metadata_id,ch_metadata_id);
    res = pubnub_remove_memberships(pbp, uuid_metadata_id, include, remove_membership_json);
    if (PNR_STARTED == res) {
        res = pubnub_await(pbp);
    }
    printf("pubnub_remove_memberships() lasted %lf seconds.\n", difftime(time(NULL), t0));
    if (PNR_OK == res) {
        char const* remove_memberships_resp = pubnub_get(pbp);
        printf("pubnub_remove_memberships() Response from Pubnub: %s\n",remove_memberships_resp);
    }
    else {
        printf("pubnub_remove_memberships() failed with code: %d('%s')\n",res,pubnub_res_2_string(res));
    }
    printf("\n\n");

    /********** GET MEMBERS   **********/
    puts("Get Members based on Channel MetadataId...");
    time(&t0);
    include = "custom,uuid,uuid_custom";
    res = pubnub_get_members(pbp, ch_metadata_id, include, 5, NULL, NULL, pbccTrue);
    if (PNR_STARTED == res) {
        res = pubnub_await(pbp);
    }
    printf("pubnub_get_members() lasted %lf seconds.\n", difftime(time(NULL), t0));
    if (PNR_OK == res) {
        char const* get_members_resp = pubnub_get(pbp);
        printf("pubnub_get_members() Response from Pubnub: %s\n",get_members_resp);
    }
    else {
        printf("pubnub_get_members() failed with code: %d('%s')\n",res,pubnub_res_2_string(res));
    }
    printf("\n\n");

/********** SET MEMBERS   **********/
    puts("SET Members based on Channel MetadataId...");
    time(&t0);
    include = "custom,uuid,uuid_custom";
    char set_member_json[2000];
    sprintf(set_member_json, "[{\"uuid\":{\"id\":\"%s\"},\"custom\":{\"planet\":\"earth\"}},{\"uuid\":{\"id\":\"new-%s\"},\"custom\":{\"sky\":\"blue\"}}]",uuid_metadata_id,uuid_metadata_id);
    res = pubnub_set_members(pbp, ch_metadata_id, include, set_member_json);
    if (PNR_STARTED == res) {
        res = pubnub_await(pbp);
    }
    printf("pubnub_set_members() lasted %lf seconds.\n", difftime(time(NULL), t0));
    if (PNR_OK == res) {
        char const* set_members_resp = pubnub_get(pbp);
        printf("pubnub_set_members() Response from Pubnub: %s\n",set_members_resp);
    }
    else {
        printf("pubnub_set_members() failed with code: %d('%s')\n",res,pubnub_res_2_string(res));
    }
    printf("\n\n");


    /********** REMOVE MEMBERS   **********/
    puts("REMOVE Members based on Channel MetadataId...");
    time(&t0);
    include = "custom,uuid,uuid_custom";
    char remove_members_json[2000];
    sprintf(remove_members_json, "[{\"uuid\":{\"id\":\"%s\"},\"custom\":{\"planet\":\"earth\"}},{\"uuid\":{\"id\":\"new-%s\"},\"custom\":{\"sky\":\"blue\"}}]",uuid_metadata_id,uuid_metadata_id);
    res = pubnub_remove_members(pbp, ch_metadata_id, include, remove_members_json);
    if (PNR_STARTED == res) {
        res = pubnub_await(pbp);
    }
    printf("pubnub_remove_members() lasted %lf seconds.\n", difftime(time(NULL), t0));
    if (PNR_OK == res) {
        char const* remove_members_resp = pubnub_get(pbp);
        printf("pubnub_remove_members() Response from Pubnub: %s\n",remove_members_resp);
    }
    else {
        printf("pubnub_remove_members() failed with code: %d('%s')\n",res,pubnub_res_2_string(res));
    }
    printf("\n\n");


    /* We're done, but, keep-alive might be on, so we need to cancel,
     * then free */
    sync_sample_free(pbp);

    puts("Pubnub Objects demo over.");

    return 0;
}
