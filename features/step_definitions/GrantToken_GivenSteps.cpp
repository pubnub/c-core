#include <cucumber-cpp/autodetect.hpp>
#include <cucumber-cpp/internal/Scenario.hpp>
#include <boost/algorithm/string/replace.hpp>
#include "pubnub.hpp"

using cucumber::ScenarioScope;

inline std::string get_env(const char* key)
{
    if (key == nullptr) {
        throw std::invalid_argument(
            "Null pointer passed as environment variable name");
    }
    if (*key == '\0') {
        throw std::invalid_argument(
            "Value requested for the empty-name environment variable");
    }
    const char* ev_val = getenv(key);
    if (ev_val == nullptr) {
        throw std::runtime_error("Environment variable not defined");
    }
    return std::string(ev_val);
}

GIVEN("^I have a keyset with access manager enabled$")
{
    ScenarioScope<Ctx> context;
    std::string        my_env_publish_key   = get_env("PAM_PUB_KEY");
    std::string        my_env_subscribe_key = get_env("PAM_SUB_KEY");
    std::string        my_env_secret_key    = get_env("PAM_SEC_KEY");
    context->pubKey                         = my_env_publish_key;
    context->subKey                         = my_env_subscribe_key;
    context->secKey                         = my_env_secret_key;
    BOOST_REQUIRE_NE("", my_env_publish_key);
    BOOST_REQUIRE_NE("", my_env_subscribe_key);
    BOOST_REQUIRE_NE("", my_env_secret_key);
}

GIVEN("^the TTL (\\d+)")
{
    REGEX_PARAM(int, TTL);
    ScenarioScope<Ctx> context;
    context->TTL = TTL;
}

GIVEN("^the '(.*)' (.*) access permissions$")
{
    REGEX_PARAM(std::string, channelName);
    REGEX_PARAM(std::string, type);
    ScenarioScope<Ctx>                 context;
    std::shared_ptr<NameTypeAndGrants> newResource =
        std::make_shared<NameTypeAndGrants>();

    newResource->name =
        boost::algorithm::replace_all_copy(channelName, "\\", "\\\\");
    newResource->type     = type;
    newResource->grants   = 0;
    context->currentGrant = newResource;
    context->grants.push_back(newResource);
}

GIVEN("^grant \\w+ permission READ$")
{
    ScenarioScope<Ctx> context;
    context->currentGrant->grants |=
        pubnub_get_grant_bit_mask_value({ .read   = true,
                                          .write  = false,
                                          .manage = false,
                                          .del    = false,
                                          .create = false,
                                          .get    = false,
                                          .update = false,
                                          .join   = false });
}

GIVEN("^grant \\w+ permission WRITE$")
{
    ScenarioScope<Ctx> context;
    context->currentGrant->grants |=
        pubnub_get_grant_bit_mask_value({ .read   = false,
                                          .write  = true,
                                          .manage = false,
                                          .del    = false,
                                          .create = false,
                                          .get    = false,
                                          .update = false,
                                          .join   = false });
}

GIVEN("^grant \\w+ permission GET$")
{
    ScenarioScope<Ctx> context;
    context->currentGrant->grants |=
        pubnub_get_grant_bit_mask_value({ .read   = false,
                                          .write  = false,
                                          .manage = false,
                                          .del    = false,
                                          .create = false,
                                          .get    = true,
                                          .update = false,
                                          .join   = false });
}

GIVEN("^grant \\w+ permission MANAGE$")
{
    ScenarioScope<Ctx> context;
    context->currentGrant->grants |=
        pubnub_get_grant_bit_mask_value({ .read   = false,
                                          .write  = false,
                                          .manage = true,
                                          .del    = false,
                                          .create = false,
                                          .get    = false,
                                          .update = false,
                                          .join   = false });
}

GIVEN("^grant \\w+ permission UPDATE$")
{
    ScenarioScope<Ctx> context;
    context->currentGrant->grants |=
        pubnub_get_grant_bit_mask_value({ .read   = false,
                                          .write  = false,
                                          .manage = false,
                                          .del    = false,
                                          .create = false,
                                          .get    = false,
                                          .update = true,
                                          .join   = false });
}

GIVEN("^grant \\w+ permission JOIN$")
{
    ScenarioScope<Ctx> context;
    context->currentGrant->grants |=
        pubnub_get_grant_bit_mask_value({ .read   = false,
                                          .write  = false,
                                          .manage = false,
                                          .del    = false,
                                          .create = false,
                                          .get    = false,
                                          .update = false,
                                          .join   = true });
}


GIVEN("^grant \\w+ permission DELETE$")
{
    ScenarioScope<Ctx> context;
    context->currentGrant->grants |=
        pubnub_get_grant_bit_mask_value({ .read   = false,
                                          .write  = false,
                                          .manage = false,
                                          .del    = true,
                                          .create = false,
                                          .get    = false,
                                          .update = false,
                                          .join   = false });
}

GIVEN("^the authorized UUID \"(.*)\"$")
{
    REGEX_PARAM(std::string, authorizedUUID);
    ScenarioScope<Ctx> context;
    context->authorizedUUID = authorizedUUID;
}


GIVEN("^deny resource permission GET$")
{
    // do nothing
}

GIVEN("^I have a known token .*$")
{
    ScenarioScope<Ctx> context;
    context->token =
        "qEF2AkF0GmEI03xDdHRsGDxDcmVzpURjaGFuoWljaGFubmVsLTEY70NncnChb2NoYW5uZW"
        "xfZ3JvdXAtMQVDdXNyoENzcGOgRHV1aWShZnV1aWQtMRhoQ3BhdKVEY2hhbqFtXmNoYW5u"
        "ZWwtXFMqJBjvQ2dycKF0XjpjaGFubmVsX2dyb3VwLVxTKiQFQ3VzcqBDc3BjoER1dWlkoW"
        "pedXVpZC1cUyokGGhEbWV0YaBEdXVpZHR0ZXN0LWF1dGhvcml6ZWQtdXVpZENzaWdYIPpU"
        "-vCe9rkpYs87YUrFNWkyNq8CVvmKwEjVinnDrJJc";
}


GIVEN("^a token$")
{
    ScenarioScope<Ctx> context;
    context->token =
        "qEF2AkF0GmEI03xDdHRsGDxDcmVzpURjaGFuoWljaGFubmVsLTEY70NncnChb2NoYW5uZWxfZ3JvdXAtMQ"
        "VDdXNyoENzcGOgRHV1aWShZnV1aWQtMRhoQ3BhdKVEY2hhbqFtXmNoYW5uZWwtXFMqJBjvQ2dycKF0XjpjaG"
        "FubmVsX2dyb3VwLVxTKiQFQ3VzcqBDc3BjoER1dWlkoWpedXVpZC1cUyokGGhEbWV0YaBEdXVpZHR0ZXN0LWF1"
        "dGhvcml6ZWQtdXVpZENzaWdYIPpU-vCe9rkpYs87YUrFNWkyNq8CVvmKwEjVinnDrJJc";

}


GIVEN("^the token string '(.*)'$") 
{
    REGEX_PARAM(std::string, token);
    ScenarioScope<Ctx> context;
    context->token = "unescaped-_.ABCabc123 escaped;,/?:@&=+$#";
}
