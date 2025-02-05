#include "core/pubnub_api_types.h"
#include "core/pubnub_blocking_io.h"
#include "core/pubnub_pubsubapi.h"
#include "core/pubnub_helper.h"
#include "core/pubnub_alloc.h"
#include "core/pubnub_ntf_sync.h"
#include "core/pubnub_objects_api.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// This sample demo is split into 4 sections:
// 1. UUID metadata section 
// 2. Channels metadata section 
// 3. Memberships section 
// 4. Members section 
//
// You can navigate to each section by searching for the section name.
//
// Each section shows basic usage of the Pubnub Objects API. 
// There are more options available for each API call,
// which can be found in the Pubnub Objects API documentation.

static void sync_sample_free(pubnub_t* p);

int main(void) {
    // No demo keysets as app context is not applicable to it
    char* publish_key = getenv("PUBNUB_PUBLISH_KEY");
    char* subscribe_key = getenv("PUBNUB_SUBSCRIBE_KEY");

    enum pubnub_res result;

    pubnub_t* pb = pubnub_alloc();
    if (NULL == pb) {
        printf("Failed to allocate Pubnub context!\n");
        return -1;
    }

    pubnub_init(pb, publish_key, subscribe_key);
    pubnub_set_user_id(pb, "my_user");

    // Depending on requirements, you can use non-blocking I/O
    pubnub_set_blocking_io(pb);

    printf("~~~~~~~~~~~~~~~~~~uuid metadata section~~~~~~~~~~~~~~~~~~\n");

    printf("Set users' metadata\n");
    struct pubnub_set_uuidmetadata_opts set_opts = pubnub_set_uuidmetadata_defopts();
    set_opts.data.name = "my_name";
    set_opts.data.external_id = "my_external_id";
    set_opts.data.profile_url = "my_profile_url";
    set_opts.data.email = "my_email";
    set_opts.data.custom = "{\"key\":\"value\"}";
    set_opts.data.status = "active";
    set_opts.data.type = "admin";
    set_opts.include = "custom";

    // we didn't set the UUID, so it will be the one from the context
    result = pubnub_set_uuidmetadata_ex(pb, set_opts);
    if (PNR_STARTED == result) {
        result = pubnub_await(pb);
    }

    if (PNR_OK == result) {
        printf("Set UUID metadata successful!\n");
    } else {
        printf("Set UUID metadata failed with code: %d('%s')\n",
               result,
               pubnub_res_2_string(result));
    }

    struct pubnub_set_uuidmetadata_opts set_opts2 = pubnub_set_uuidmetadata_defopts();
    set_opts2.data.custom = "{\"key\":\"totally different value\"}";
    set_opts2.uuid = "some_user";

    result = pubnub_set_uuidmetadata_ex(pb, set_opts2);
    if (PNR_STARTED == result) {
        result = pubnub_await(pb);
    }

    if (PNR_OK == result) {
        printf("Set UUID metadata successful!\n");
    } else {
        printf("Set UUID metadata failed with code: %d('%s')\n",
               result,
               pubnub_res_2_string(result));
    }

    printf("Get users' metadata\n");
    struct pubnub_getall_metadata_opts getall_opts = pubnub_getall_metadata_defopts();
    // getall_opts.page.next = you can retrieve the next page in the response when the response is paginated
    // getall_opts.page.prev = as above
    getall_opts.limit = 1;
    getall_opts.include = "custom,status,type";
    getall_opts.filter = "name=='my_name'";
    getall_opts.count = pbccTrue;
    getall_opts.sort = "name:desc";
    
    result = pubnub_getall_uuidmetadata_ex(pb, getall_opts);
    if (PNR_STARTED == result) {
        result = pubnub_await(pb);
    }

    if (PNR_OK == result) {
        printf("Get UUID metadata successful!\n");

        for (const char* response = pubnub_get(pb); response != NULL; response = pubnub_get(pb)) {
            printf("Response: %s\n", response);
            if (NULL == strstr(response, "\"status\":\"active\"")) {
                printf("\"status\" is missing in response.\n");
                return 1;
            }
            if (NULL == strstr(response, "\"type\":\"admin\"")) {
                printf("\"type\" is missing in response.\n");
                return 1;
            }
        }
    } else {
        printf("Get UUID metadata failed with code: %d('%s')\n",
               result,
               pubnub_res_2_string(result));
    }

    sleep(1);
    printf("Get users' metadata by UUID\n");
    result = pubnub_get_uuidmetadata(pb, "custom,status", "my_user");

    if (PNR_STARTED == result) {
        result = pubnub_await(pb);
    }

    if (PNR_OK == result) {
        printf("Get UUID metadata by UUID successful!\n");

        const char* response = pubnub_get(pb);
        printf("Response: %s\n", response);
        if (NULL == strstr(response, "\"status\":\"active\"")) {
            printf("\"status\" is missing in response.\n");
            return 1;
        }
    } else {
        printf("Get UUID metadata by UUID failed with code: %d('%s')\n",
               result,
               pubnub_res_2_string(result));
    }

    sleep(1);
    printf("Remove users' metadata by UUID\n");

    result = pubnub_remove_uuidmetadata(pb, "my_user");

    if (PNR_STARTED == result) {
        result = pubnub_await(pb);
    }

    if (PNR_OK == result) {
        printf("Remove UUID metadata successful!\n");
    } else {
        printf("Remove UUID metadata failed with code: %d('%s')\n",
               result,
               pubnub_res_2_string(result));
    }

    result = pubnub_remove_uuidmetadata(pb, "some_user");

    if (PNR_STARTED == result) {
        result = pubnub_await(pb);
    }

    if (PNR_OK == result) {
        printf("Remove UUID metadata successful!\n");
    } else {
        printf("Remove UUID metadata failed with code: %d('%s')\n",
               result,
               pubnub_res_2_string(result));
    }

    printf("~~~~~~~~~~~~~~~~channels metadata section~~~~~~~~~~~~~~~~\n");

    printf("Set channels' metadata\n");
    struct pubnub_set_channelmetadata_opts set_channel_opts = pubnub_set_channelmetadata_defopts();
    set_channel_opts.data.name = "my_channel_name";
    set_channel_opts.data.description = "my_channel_description";
    set_channel_opts.data.custom = "{\"key\":\"value\"}";
    set_channel_opts.data.status = "closed";
    set_channel_opts.data.type = "lobby";
    set_channel_opts.include = "custom";

    result = pubnub_set_channelmetadata_ex(pb, "channel_id", set_channel_opts);

    if (PNR_STARTED == result) {
        result = pubnub_await(pb);
    }

    if (PNR_OK == result) {
        printf("Set channel metadata successful!\n");
    } else {
        printf("Set channel metadata failed with code: %d('%s')\n",
               result,
               pubnub_res_2_string(result));
    }

    sleep(1);
    printf("Get channels' metadata\n");

    struct pubnub_getall_metadata_opts getall_channel_opts = pubnub_getall_metadata_defopts();
    getall_channel_opts.limit = 1;
    getall_channel_opts.include = "custom,status,type";
    getall_channel_opts.filter = "custom.key=='value'";
    getall_channel_opts.count = pbccTrue;
    getall_channel_opts.sort = "name:desc";

    result = pubnub_getall_channelmetadata_ex(pb, getall_channel_opts);

    if (PNR_STARTED == result) {
        result = pubnub_await(pb);
    }

    if (PNR_OK == result) {
        printf("Get channel metadata successful!\n");

        for (const char* response = pubnub_get(pb); response != NULL; response = pubnub_get(pb)) {
            printf("Response: %s\n", response);
        }
    } else {
        printf("Get channel metadata failed with code: %d('%s')\n",
               result,
               pubnub_res_2_string(result));
    }

    sleep(1);
    printf("Get channels' metadata by channel ID\n");

    result = pubnub_get_channelmetadata(pb, "custom,type", "channel_id");

    if (PNR_STARTED == result) {
        result = pubnub_await(pb);
    }

    if (PNR_OK == result) {
        printf("Get channel metadata by channel ID successful!\n");

        const char* response = pubnub_get(pb);
        printf("Response: %s\n", response);
        if (NULL == strstr(response, "\"type\":\"lobby\"")) {
            printf("\"type\" is missing in response.\n");
            return 1;
        }
    } else {
        printf("Get channel metadata by channel ID failed with code: %d('%s')\n",
               result,
               pubnub_res_2_string(result));
    }

    sleep(1);
    printf("Remove channels' metadata by channel ID\n");
    result = pubnub_remove_channelmetadata(pb, "channel_id");

    if (PNR_STARTED == result) {
        result = pubnub_await(pb);
    }

    if (PNR_OK == result) {
        printf("Remove channel metadata successful!\n");
    } else {
        printf("Remove channel metadata failed with code: %d('%s')\n",
               result,
               pubnub_res_2_string(result));
    }

    printf("~~~~~~~~~~~~~~~~~~~memberships section~~~~~~~~~~~~~~~~~~~\n");

    printf("Set users' memberships\n");
    struct pubnub_membership_opts set_memberships_opts = pubnub_memberships_defopts();
    set_memberships_opts.include = "custom,status,type";
    set_memberships_opts.limit = 1;
    set_memberships_opts.sort = "channel.name:desc";
    set_memberships_opts.filter = "custom.key=='value'";
    set_memberships_opts.count = pbccTrue;

    const char* channels_meta = "["
         "{"
           "\"channel\":{ \"id\": \"main-channel-id\" },"
           "\"custom\": {"
             "\"starred\": true"
           "},"
           "\"status\":\"active\","
           "\"type\":\"hall\""
         "},"
         "{"
           "\"channel\":{ \"id\": \"channel-0\" },"
            "\"some_key\": {"
              "\"other_key\": \"other_value\""
            "}"
         "}"
       "]";

    result = pubnub_set_memberships_ex(pb, channels_meta, set_memberships_opts);

    if (PNR_STARTED == result) {
        result = pubnub_await(pb);
    }

    if (PNR_OK == result) {
        printf("Set memberships successful!\n");
    } else {
        printf("Set memberships failed with code: %d('%s')\n",
               result,
               pubnub_res_2_string(result));
    }

    sleep(1);
    printf("Get users' memberships by UUID\n");

    struct pubnub_membership_opts get_memberships_opts = pubnub_memberships_defopts();
    get_memberships_opts.limit = 1;
    get_memberships_opts.include = "custom,status,type";
    get_memberships_opts.filter = "custom.starred==true";
    get_memberships_opts.count = pbccTrue;
    get_memberships_opts.sort = "channel.id:desc";

    result = pubnub_get_memberships_ex(pb, get_memberships_opts);

    if (PNR_STARTED == result) {
        result = pubnub_await(pb);
    }

    if (PNR_OK == result) {
        printf("Get memberships by UUID successful!\n");

        const char* response = pubnub_get(pb);
        printf("Response: %s\n", response);
        if (NULL == strstr(response, "\"status\":\"active\"")) {
            printf("\"status\" is missing in response.\n");
            return 1;
        }
        if (NULL == strstr(response, "\"type\":\"hall\"")) {
            printf("\"type\" is missing in response.\n");
            return 1;
        }
    } else {
        printf("Get memberships by UUID failed with code: %d('%s')\n",
               result,
               pubnub_res_2_string(result));
    }

    sleep(1);
    printf("Remove users' memberships by UUID\n");

    struct pubnub_membership_opts remove_memberships_opts = pubnub_memberships_defopts();
    remove_memberships_opts.include = "custom";
    remove_memberships_opts.limit = 1;
    remove_memberships_opts.sort = "channel.id:desc";
    remove_memberships_opts.filter = "custom.starred==true";
    remove_memberships_opts.count = pbccTrue;

    const char* channels_meta_remove = "["
         "{"
           "\"channel\":{ \"id\": \"main-channel-id\" }"
         "},"
         "{"
           "\"channel\":{ \"id\": \"channel-0\" }"
         "}"
       "]";


    result = pubnub_remove_memberships_ex(pb, channels_meta_remove, remove_memberships_opts);

    if (PNR_STARTED == result) {
        result = pubnub_await(pb);
    }

    if (PNR_OK == result) {
        printf("Remove memberships successful!\n");
    } else {
        printf("Remove memberships failed with code: %d('%s')\n",
               result,
               pubnub_res_2_string(result));
    }

    printf("~~~~~~~~~~~~~~~~~~~~~members section~~~~~~~~~~~~~~~~~~~~~\n");

    printf("Set channels' members\n");

    struct pubnub_members_opts set_members_opts = pubnub_members_defopts();
    set_members_opts.include = "custom";
    set_members_opts.limit = 1;
    set_members_opts.sort = "uuid.id:desc";
    set_members_opts.filter = "custom.starred==true";
    set_members_opts.count = pbccTrue;

    const char* members_meta = "["
         "{"
           "\"uuid\":{ \"id\": \"main-user-id\" },"
           "\"custom\": {"
             "\"starred\": true"
           "},"
           "\"status\":\"moderating\","
           "\"type\":\"admin\""
         "},"
         "{"
           "\"uuid\":{ \"id\": \"user-0\" },"
            "\"some_key\": {"
              "\"other_key\": \"other_value\""
            "}"
         "}"
       "]";

    result = pubnub_set_members_ex(pb, "channel_id", members_meta, set_members_opts);

    if (PNR_STARTED == result) {
        result = pubnub_await(pb);
    }

    if (PNR_OK == result) {
        printf("Set members successful!\n");
    } else {
        printf("Set members failed with code: %d('%s')\n",
               result,
               pubnub_res_2_string(result));
    }

    sleep(1);
    printf("Get channels' members\n");

    struct pubnub_members_opts get_members_opts = pubnub_members_defopts();
    get_members_opts.limit = 1;
    get_members_opts.include = "custom,status";
    get_members_opts.filter = "custom.starred==true";
    get_members_opts.count = pbccTrue;
    get_members_opts.sort = "uuid.id:desc";

    result = pubnub_get_members_ex(pb, "channel_id", get_members_opts);

    if (PNR_STARTED == result) {
        result = pubnub_await(pb);
    }

    if (PNR_OK == result) {
        printf("Get members successful!\n");

        for (const char* response = pubnub_get(pb); response != NULL; response = pubnub_get(pb)) {
            printf("Response: %s\n", response);
            if (NULL == strstr(response, "\"status\":\"moderating\"")) {
                printf("\"status\" is missing in response.\n");
                return 1;
            }
        }
    } else {
        printf("Get members failed with code: %d('%s')\n",
               result,
               pubnub_res_2_string(result));
    }

    sleep(1);
    printf("Remove channels' members by UUID\n");

    struct pubnub_members_opts remove_members_opts = pubnub_members_defopts();
    remove_members_opts.include = "custom";
    remove_members_opts.limit = 1;
    remove_members_opts.sort = "uuid.id:desc";
    remove_members_opts.filter = "custom.starred==true";
    remove_members_opts.count = pbccTrue;

    const char* members_meta_remove = "["
         "{"
           "\"uuid\":{ \"id\": \"main-user-id\" }"
         "},"
         "{"
           "\"uuid\":{ \"id\": \"user-0\" }"
         "}"
       "]";

    result = pubnub_remove_members_ex(pb, "channel_id", members_meta_remove, remove_members_opts);

    if (PNR_STARTED == result) {
        result = pubnub_await(pb);
    }

    if (PNR_OK == result) {
        printf("Remove members successful!\n");
    } else {
        printf("Remove members failed with code: %d('%s')\n",
               result,
               pubnub_res_2_string(result));
    }

    // ~~~~END OF SAMPLE~~~~

    sync_sample_free(pb);

    return 0;
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


