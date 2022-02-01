#include "pubnub.hpp"
#include <cucumber-cpp/autodetect.hpp>
#include <cucumber-cpp/internal/Scenario.hpp>

using cucumber::ScenarioScope;

std::string jObj()
{
    return "{}";
}

bool emptyJStr(std::string val)
{
    return "\"\"" == val;
}

std::string jObj(std::map<std::string, std::string> props)
{
    if (props.size() == 0) {
        return "{}";
    }
    std::stringstream ss;
    ss << "{";
    for (std::pair<std::string, std::string> prop : props) {
        if (!emptyJStr(prop.second)) {
            ss << "\"" << prop.first << "\":" << prop.second << ",";
        }
    }
    std::string res = ss.str();
    res.pop_back();
    return res + "}";
}


std::string jStr(std::string val)
{
    return "\"" + val + "\"";
}

std::map<std::string, std::string>
getGrantsFor(std::string type, std::vector<std::shared_ptr<NameTypeAndGrants>> grants)
{
    std::map<std::string, std::string> result;
    for (std::shared_ptr<NameTypeAndGrants> e : grants) {
        if (type == e->type) {
            result.insert({ e->name, std::to_string(e->grants) });
        }
    }
    return result;
}

WHEN("^I grant a token specifying those permissions$")
{
    ScenarioScope<Ctx> context;

    pubnub::context pn(context->pubKey, context->subKey);

    pn.set_origin("localhost");
    pn.set_port(8090);
    pn.set_secret_key(context->secKey);
    pn.set_blocking_io(pubnub::non_blocking);

    std::string props = jObj(
        { { "ttl", std::to_string(context->TTL) },
          { "permissions",
            jObj(
                { { "resources",
                    jObj({ { "channels",
                             jObj(getGrantsFor("CHANNEL resource", context->grants)) },
                           { "groups",
                             jObj(getGrantsFor("CHANNEL_GROUP resource", context->grants)) },
                           { "uuids",
                             jObj(getGrantsFor("UUID resource", context->grants)) } }) },
                  { "patterns",
                    jObj({ { "channels",
                             jObj(getGrantsFor("CHANNEL pattern", context->grants)) },
                           { "groups",
                             jObj(getGrantsFor("CHANNEL_GROUP pattern", context->grants)) },
                           { "uuids",
                             jObj(getGrantsFor("UUID pattern", context->grants)) } }) },
                  { "uuid", jStr(context->authorizedUUID) } }) } });


    pubnub::futres  futgres = pn.grant_token(props);
    enum pubnub_res res     = futgres.await();

    BOOST_CHECK_EQUAL(res, PNR_OK);
    std::string tkn      = pn.get_grant_token();
    context->parsedToken = pn.parse_token(tkn);
}


WHEN("^I attempt to grant a token specifying those permissions$")
{
    ScenarioScope<Ctx> context;

    pubnub::context pn(context->pubKey, context->subKey);

    pn.set_origin("localhost");
    pn.set_port(8090);
    pn.set_secret_key(context->secKey);
    pn.set_blocking_io(pubnub::non_blocking);

    std::string props = jObj(
        { { "ttl", std::to_string(context->TTL) },
          { "permissions",
            jObj(
                { { "resources",
                    jObj({ { "channels",
                             jObj(getGrantsFor("CHANNEL resource", context->grants)) },
                           { "groups",
                             jObj(getGrantsFor("CHANNEL_GROUP resource", context->grants)) },
                           { "uuids",
                             jObj(getGrantsFor("UUID resource", context->grants)) } }) },
                  { "patterns",
                    jObj({ { "channels",
                             jObj(getGrantsFor("CHANNEL pattern", context->grants)) },
                           { "groups",
                             jObj(getGrantsFor("CHANNEL_GROUP pattern", context->grants)) },
                           { "uuids",
                             jObj(getGrantsFor("UUID pattern", context->grants)) } }) },
                  { "uuid", jStr(context->authorizedUUID) } }) } });


    pubnub::futres  futgres = pn.grant_token(props);
    enum pubnub_res res     = futgres.await();

    BOOST_CHECK_NE(res, PNR_OK);

    context->errorMessage = pn.last_http_response_body();
    context->statusCode   = pn.last_http_code();
    std::cout << "Error message: " << pn.last_http_response_body() << std::endl
              << std::endl
              << std::endl
              << "End" << std::endl;
}


WHEN("^I parse the token$")
{
    ScenarioScope<Ctx> context;
    pubnub::context    pn(context->pubKey, context->subKey);

    pn.set_secret_key(context->secKey);
    context->parsedToken = pn.parse_token(context->token);
}


WHEN("^I revoke a token$")
{
    ScenarioScope<Ctx> context;
    pubnub::context    pn(context->pubKey, context->subKey);

    pn.set_origin("localhost");
    pn.set_port(8090);
    pn.set_secret_key(context->secKey);
    pn.set_blocking_io(pubnub::non_blocking);
    pn.set_secret_key(context->secKey);

    pubnub::futres  futgres = pn.revoke_token(context->token);
    enum pubnub_res res     = futgres.await();

    if(res == PNR_OK)
      context->revokeTokenResult     = pn.get_revoke_token_result();
    else
    {
      context->statusCode   = pn.last_http_code();
      context->errorMessage = pn.last_http_response_body();
    }
}
