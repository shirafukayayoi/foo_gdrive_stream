#include "stdafx.h"
#include "core_utils.h"
#include "config.h"
#include "http_client.h"
#include "oauth.h"

namespace gds {

static int64_t nowUnix() {
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

OAuthClientInfo loadClientInfo() {
    const auto path = clientJsonPath();
    if (path.empty()) throw std::runtime_error("OAuth client JSON is not configured");
    const auto json = nlohmann::json::parse(readTextFile(utf8ToWide(path)));
    const auto node = json.contains("installed") ? json.at("installed") : json.at("web");
    OAuthClientInfo info;
    info.clientId = node.at("client_id").get<std::string>();
    if (node.contains("client_secret")) info.clientSecret = node.at("client_secret").get<std::string>();
    return info;
}

static std::vector<uint8_t> dpapiProtect(const std::string &plain) {
    DATA_BLOB in{(DWORD)plain.size(), (BYTE *)plain.data()};
    DATA_BLOB out{};
    if (!CryptProtectData(&in, L"foo_gdrive_stream token", nullptr, nullptr, nullptr, 0, &out)) throw std::runtime_error("CryptProtectData failed");
    ScopeExit freeOut([&] { LocalFree(out.pbData); });
    return std::vector<uint8_t>(out.pbData, out.pbData + out.cbData);
}

static std::string dpapiUnprotect(const std::vector<uint8_t> &bytes) {
    DATA_BLOB in{(DWORD)bytes.size(), (BYTE *)bytes.data()};
    DATA_BLOB out{};
    if (!CryptUnprotectData(&in, nullptr, nullptr, nullptr, nullptr, 0, &out)) throw std::runtime_error("CryptUnprotectData failed");
    ScopeExit freeOut([&] { LocalFree(out.pbData); });
    return std::string((const char *)out.pbData, (const char *)out.pbData + out.cbData);
}

std::optional<OAuthToken> loadToken() {
    const auto path = tokenPath();
    if (GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES) return std::nullopt;
    const auto text = dpapiUnprotect(readBinaryFile(path));
    const auto json = nlohmann::json::parse(text);
    OAuthToken token;
    token.accessToken = json.value("access_token", "");
    token.refreshToken = json.value("refresh_token", "");
    token.expiresAtUnix = json.value("expires_at", (int64_t)0);
    if (token.accessToken.empty() && token.refreshToken.empty()) return std::nullopt;
    return token;
}

void saveToken(const OAuthToken &token) {
    nlohmann::json json;
    json["access_token"] = token.accessToken;
    json["refresh_token"] = token.refreshToken;
    json["expires_at"] = token.expiresAtUnix;
    writeBinaryFile(tokenPath(), dpapiProtect(json.dump()));
}

bool tokenLooksValid(const OAuthToken &token) {
    return !token.accessToken.empty() && token.expiresAtUnix > nowUnix() + 120;
}

static OAuthToken parseTokenResponse(const std::string &body, const OAuthToken *oldToken = nullptr) {
    const auto json = nlohmann::json::parse(body);
    OAuthToken token;
    token.accessToken = json.at("access_token").get<std::string>();
    token.refreshToken = json.value("refresh_token", oldToken ? oldToken->refreshToken : "");
    token.expiresAtUnix = nowUnix() + json.value("expires_in", 3600);
    return token;
}

static OAuthToken refreshToken(const OAuthClientInfo &client, const OAuthToken &oldToken, abort_callback &abort) {
    HttpClient http;
    std::string body = "client_id=" + urlEncode(client.clientId) +
        "&refresh_token=" + urlEncode(oldToken.refreshToken) +
        "&grant_type=refresh_token";
    if (!client.clientSecret.empty()) body += "&client_secret=" + urlEncode(client.clientSecret);
    auto res = http.postForm(L"https://oauth2.googleapis.com/token", body, {L"Content-Type: application/x-www-form-urlencoded"}, abort);
    auto token = parseTokenResponse(res.body, &oldToken);
    saveToken(token);
    return token;
}

std::string getAccessToken(abort_callback &abort) {
    const auto client = loadClientInfo();
    if (auto token = loadToken()) {
        if (tokenLooksValid(*token)) return token->accessToken;
        if (!token->refreshToken.empty()) return refreshToken(client, *token, abort).accessToken;
    }
    runInteractiveAuth(abort);
    auto token = loadToken();
    if (!token || token->accessToken.empty()) throw std::runtime_error("OAuth token was not created");
    return token->accessToken;
}

static SOCKET makeListenSocket(uint16_t &port) {
    WSADATA wsa{};
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) throw std::runtime_error("WSAStartup failed");

    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) throw std::runtime_error("socket failed");
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    if (bind(s, (sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR) throw std::runtime_error("bind failed");
    int len = sizeof(addr);
    if (getsockname(s, (sockaddr *)&addr, &len) == SOCKET_ERROR) throw std::runtime_error("getsockname failed");
    port = ntohs(addr.sin_port);
    if (listen(s, 1) == SOCKET_ERROR) throw std::runtime_error("listen failed");
    return s;
}

static std::string waitForAuthCode(uint16_t &port, abort_callback &abort) {
    SOCKET listenSocket = makeListenSocket(port);
    ScopeExit closeListen([&] {
        closesocket(listenSocket);
        WSACleanup();
    });

    fd_set set;
    timeval tv{};
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::minutes(5);
    while (std::chrono::steady_clock::now() < deadline) {
        abort.check();
        FD_ZERO(&set);
        FD_SET(listenSocket, &set);
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        const int ready = select(0, &set, nullptr, nullptr, &tv);
        if (ready <= 0) continue;

        SOCKET client = accept(listenSocket, nullptr, nullptr);
        if (client == INVALID_SOCKET) continue;
        ScopeExit closeClient([&] { closesocket(client); });
        char buffer[8192]{};
        const int got = recv(client, buffer, sizeof(buffer) - 1, 0);
        if (got <= 0) continue;
        std::string req(buffer, got);
        std::smatch m;
        std::string code;
        if (std::regex_search(req, m, std::regex(R"(GET\s+/callback\?([^ ]+)\s)"))) {
            const std::string query = m[1].str();
            std::smatch qm;
            if (std::regex_search(query, qm, std::regex(R"((^|&)code=([^&]+))"))) code = urlDecode(qm[2].str());
        }
        const char *html = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nConnection: close\r\n\r\n<html><body>foo_gdrive_stream authentication complete. You can close this tab.</body></html>";
        send(client, html, (int)strlen(html), 0);
        if (!code.empty()) return code;
    }
    throw std::runtime_error("Timed out waiting for OAuth callback");
}

void runInteractiveAuth(abort_callback &abort) {
    const auto client = loadClientInfo();
    uint16_t port = 0;
    std::string code;
    std::exception_ptr err;
    std::thread waiter([&] {
        try { code = waitForAuthCode(port, abort); }
        catch (...) { err = std::current_exception(); }
    });

    while (port == 0) {
        abort.check();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    const std::string verifier = base64UrlEncode(randomBytes(48));
    const std::string challenge = sha256Base64Url(verifier);
    const std::string redirect = "http://127.0.0.1:" + std::to_string(port) + "/callback";
    const std::string authUrl =
        "https://accounts.google.com/o/oauth2/v2/auth?response_type=code"
        "&client_id=" + urlEncode(client.clientId) +
        "&redirect_uri=" + urlEncode(redirect) +
        "&scope=" + urlEncode("https://www.googleapis.com/auth/drive.readonly") +
        "&access_type=offline&prompt=consent"
        "&code_challenge=" + urlEncode(challenge) +
        "&code_challenge_method=S256";

    ShellExecuteW(nullptr, L"open", utf8ToWide(authUrl).c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    waiter.join();
    if (err) std::rethrow_exception(err);
    if (code.empty()) throw std::runtime_error("OAuth code was empty");

    std::string body = "code=" + urlEncode(code) +
        "&client_id=" + urlEncode(client.clientId) +
        "&redirect_uri=" + urlEncode(redirect) +
        "&grant_type=authorization_code" +
        "&code_verifier=" + urlEncode(verifier);
    if (!client.clientSecret.empty()) body += "&client_secret=" + urlEncode(client.clientSecret);
    HttpClient http;
    auto res = http.postForm(L"https://oauth2.googleapis.com/token", body, {L"Content-Type: application/x-www-form-urlencoded"}, abort);
    saveToken(parseTokenResponse(res.body));
}

}
