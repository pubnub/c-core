#include <cucumber-cpp/autodetect.hpp>
#include <cucumber-cpp/internal/Scenario.hpp>
#include <regex>
#include <boost/algorithm/string/replace.hpp>
#include "pubnub.hpp"

using cucumber::ScenarioScope;

int getInt(std::string name, std::string token)
{
    std::string escapedName = boost::algorithm::replace_all_copy(
        boost::algorithm::replace_all_copy(
            boost::algorithm::replace_all_copy(
                boost::algorithm::replace_all_copy(name, "\\", "\\\\"), "^", "\\^"),
            "*",
            "\\*"),
        "$",
        "\\$");
    std::regex  int_regex("\"" + escapedName + "\":(\\d+)");
    std::smatch sm;
    if (!std::regex_search(token, sm, int_regex)) {
        BOOST_FAIL(name + " not found");
    }
    return std::stoi(sm[1]);
}

std::string getString(std::string name, std::string token)
{
    std::regex  int_regex("\"" + name + "\":\"(.*?)\"");
    std::smatch sm;
    if (!std::regex_search(token, sm, int_regex)) {
        BOOST_FAIL(name + " not found");
    }
    return sm[1];
}

std::string getArray(std::string name, std::string token)
{
    std::regex  int_regex("\"" + name + "\":(\\[.*\\])");
    std::smatch sm;
    if (!std::regex_search(token, sm, int_regex)) {
        BOOST_FAIL(name + " not found");
    }
    return sm[1];
}

THEN("^the token contains the TTL (\\d+)$")
{
    ScenarioScope<Ctx> context;
    REGEX_PARAM(int, TTL);

    int ttlFromToken = getInt("ttl", context->parsedToken);
    BOOST_CHECK_EQUAL(ttlFromToken, TTL);
}

THEN("^token \\w+ permission READ$")
{
    ScenarioScope<Ctx> context;

    int mask = pubnub_get_grant_bit_mask_value({ .read   = true,
                                                 .write  = false,
                                                 .manage = false,
                                                 .del    = false,
                                                 .create = false,
                                                 .get    = false,
                                                 .update = false,
                                                 .join   = false });
    BOOST_CHECK_NE(mask & context->currentResourceGrants, 0);
}

THEN("^token \\w+ permission WRITE$")
{
    ScenarioScope<Ctx> context;

    int mask = pubnub_get_grant_bit_mask_value({ .read   = false,
                                                 .write  = true,
                                                 .manage = false,
                                                 .del    = false,
                                                 .create = false,
                                                 .get    = false,
                                                 .update = false,
                                                 .join   = false });

    BOOST_CHECK_NE(mask & context->currentResourceGrants, 0);
}

THEN("^token \\w+ permission GET$")
{
    ScenarioScope<Ctx> context;

    int mask = pubnub_get_grant_bit_mask_value({ .read   = false,
                                                 .write  = false,
                                                 .manage = false,
                                                 .del    = false,
                                                 .create = false,
                                                 .get    = true,
                                                 .update = false,
                                                 .join   = false });

    BOOST_CHECK_NE(mask & context->currentResourceGrants, 0);
}

THEN("^token \\w+ permission MANAGE$")
{
    ScenarioScope<Ctx> context;

    int mask = pubnub_get_grant_bit_mask_value({ .read   = false,
                                                 .write  = false,
                                                 .manage = true,
                                                 .del    = false,
                                                 .create = false,
                                                 .get    = false,
                                                 .update = false,
                                                 .join   = false });

    BOOST_CHECK_NE(mask & context->currentResourceGrants, 0);
}

THEN("^token \\w+ permission UPDATE$")
{
    ScenarioScope<Ctx> context;

    int mask = pubnub_get_grant_bit_mask_value({ .read   = false,
                                                 .write  = false,
                                                 .manage = false,
                                                 .del    = false,
                                                 .create = false,
                                                 .get    = false,
                                                 .update = true,
                                                 .join   = false });

    BOOST_CHECK_NE(mask & context->currentResourceGrants, 0);
}

THEN("^token \\w+ permission JOIN$")
{
    ScenarioScope<Ctx> context;

    int mask = pubnub_get_grant_bit_mask_value({ .read   = false,
                                                 .write  = false,
                                                 .manage = false,
                                                 .del    = false,
                                                 .create = false,
                                                 .get    = false,
                                                 .update = false,
                                                 .join   = true });

    BOOST_CHECK_NE(mask & context->currentResourceGrants, 0);
}

THEN("^token \\w+ permission DELETE$")
{
    ScenarioScope<Ctx> context;

    int mask = pubnub_get_grant_bit_mask_value({ .read   = false,
                                                 .write  = false,
                                                 .manage = false,
                                                 .del    = true,
                                                 .create = false,
                                                 .get    = false,
                                                 .update = false,
                                                 .join   = false });

    BOOST_CHECK_NE(mask & context->currentResourceGrants, 0);
}


THEN("^the token does not contain an authorized uuid$")
{
    ScenarioScope<Ctx> context;

    std::regex  uuid_regex("\"uuid\":\"");
    std::smatch sm;
    if (std::regex_search(context->parsedToken, sm, uuid_regex)) {
        BOOST_ERROR("Unexpected uuid in token");
    }
}

THEN("^the token has '(.*)' .* access permissions$")
{
    ScenarioScope<Ctx> context;
    REGEX_PARAM(std::string, name);


    int grants = getInt(name, context->parsedToken);

    context->currentResourceGrants = grants;
}

THEN("^.* contains the authorized UUID \"(.*)\"$")
{
    REGEX_PARAM(std::string, authorizedUUID);
    ScenarioScope<Ctx> context;

    BOOST_CHECK_EQUAL(getString("uuid", context->parsedToken), authorizedUUID);
}

THEN("^an error is returned$") {}

THEN("^the error status code is (\\d+)$")
{
    REGEX_PARAM(int, expectedStatusCode);
    ScenarioScope<Ctx> context;
    BOOST_CHECK_EQUAL(context->statusCode, expectedStatusCode);
}

THEN("^the error message is '(.*)'$")
{
    REGEX_PARAM(std::string, expectedErrorMessage);
    ScenarioScope<Ctx> context;

    std::string message = getString("message", context->errorMessage);
    BOOST_CHECK_EQUAL(message, expectedErrorMessage);
}

THEN("^the error source is '(.*)'$")
{
    REGEX_PARAM(std::string, expectedSource);
    ScenarioScope<Ctx> context;

    std::string source = getString("source", context->errorMessage);
    BOOST_CHECK_EQUAL(source, expectedSource);
}


THEN("^the error detail message is '(.*)'$")
{
    REGEX_PARAM(std::string, expectedMessage);
    ScenarioScope<Ctx> context;

    std::size_t found = context->errorMessage.find(expectedMessage);
    BOOST_CHECK_NE(found, std::string::npos);
}


THEN("^the error detail location is '(.*)'$")
{
    REGEX_PARAM(std::string, location);
    ScenarioScope<Ctx> context;
    std::size_t        found = context->errorMessage.find(location);
    BOOST_CHECK_NE(found, std::string::npos);
}


THEN("^the error detail message is not empty$") 
{
    REGEX_PARAM(std::string, locationType);
    ScenarioScope<Ctx> context;
    std::size_t        found = context->errorMessage.find("Token parse error");
    BOOST_CHECK_NE(found, std::string::npos);
}


THEN("^the error detail location type is '(.*)'$")
{
    REGEX_PARAM(std::string, locationType);
    ScenarioScope<Ctx> context;
    std::size_t        found = context->errorMessage.find(locationType);
    BOOST_CHECK_NE(found, std::string::npos);
}


THEN("^I get confirmation that token has been revoked$")
{
    ScenarioScope<Ctx> context;
    std::size_t        found = context->revokeTokenResult.find("Success");
    BOOST_CHECK_NE(found, std::string::npos);
}


THEN("^the error service is '(.*)'$") {
    REGEX_PARAM(std::string, errorService);
    ScenarioScope<Ctx> context;
    std::size_t        found = context->errorMessage.find(errorService);
    BOOST_CHECK_NE(found, std::string::npos);
}
