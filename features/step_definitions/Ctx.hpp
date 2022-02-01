#ifndef CTX_HPP_INCLUDED
#define CTX_HPP_INCLUDED
struct NameTypeAndGrants {
    std::string name;
    std::string type;
    int        grants;
};

struct Ctx {
    std::string                                     pubKey;
    std::string                                     subKey;
    std::string                                     secKey;
    int                                             TTL;
    std::vector<std::shared_ptr<NameTypeAndGrants>> grants;
    std::shared_ptr<NameTypeAndGrants>              currentGrant;
    std::string                                     parsedToken;
    std::string                                     revokeTokenResult;
    std::string                                     token;
    int                                             currentResourceGrants;
    std::string                                     authorizedUUID;
    std::string                                     errorMessage;
    int                                             statusCode;
};

#endif