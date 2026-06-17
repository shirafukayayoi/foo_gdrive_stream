#pragma once

namespace gds {

struct OAuthClientInfo {
    std::string clientId;
    std::string clientSecret;
};

struct OAuthToken {
    std::string accessToken;
    std::string refreshToken;
    int64_t expiresAtUnix = 0;
};

OAuthClientInfo loadClientInfo();
std::optional<OAuthToken> loadToken();
void saveToken(const OAuthToken &token);
bool tokenLooksValid(const OAuthToken &token);
std::string getAccessToken(abort_callback &abort);
void runInteractiveAuth(abort_callback &abort);

}
