#include "stdafx.h"
#include "core_utils.h"
#include "http_client.h"

namespace gds {

static std::string narrowHeaderName(const std::wstring &name) {
    std::string out = wideToUtf8(name);
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    return out;
}

HttpResponse HttpClient::get(const std::wstring &url, const std::vector<std::wstring> &headers, abort_callback &abort) {
    return request(L"GET", url, nullptr, headers, nullptr, abort);
}

HttpResponse HttpClient::postForm(const std::wstring &url, const std::string &body, const std::vector<std::wstring> &headers, abort_callback &abort) {
    return request(L"POST", url, &body, headers, nullptr, abort);
}

void HttpClient::downloadToFile(const std::wstring &url, const std::vector<std::wstring> &headers, const std::wstring &target, abort_callback &abort) {
    request(L"GET", url, nullptr, headers, &target, abort);
}

HttpResponse HttpClient::request(const std::wstring &method, const std::wstring &url, const std::string *body, const std::vector<std::wstring> &headers, const std::wstring *target, abort_callback &abort) {
    URL_COMPONENTS uc{};
    uc.dwStructSize = sizeof(uc);
    wchar_t host[512]{};
    wchar_t path[4096]{};
    uc.lpszHostName = host;
    uc.dwHostNameLength = _countof(host);
    uc.lpszUrlPath = path;
    uc.dwUrlPathLength = _countof(path);
    uc.dwSchemeLength = (DWORD)-1;
    uc.dwExtraInfoLength = (DWORD)-1;
    if (!WinHttpCrackUrl(url.c_str(), 0, 0, &uc)) throw std::runtime_error("WinHttpCrackUrl failed");

    std::wstring pathAndQuery(uc.lpszUrlPath, uc.dwUrlPathLength);
    if (uc.dwExtraInfoLength > 0 && uc.lpszExtraInfo) pathAndQuery.append(uc.lpszExtraInfo, uc.dwExtraInfoLength);
    const bool secure = uc.nScheme == INTERNET_SCHEME_HTTPS;

    HINTERNET session = WinHttpOpen(L"foo_gdrive_stream/0.1", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) throw std::runtime_error("WinHttpOpen failed");
    auto closeSession = ScopeExit([&] { WinHttpCloseHandle(session); });

    HINTERNET connect = WinHttpConnect(session, std::wstring(uc.lpszHostName, uc.dwHostNameLength).c_str(), uc.nPort, 0);
    if (!connect) throw std::runtime_error("WinHttpConnect failed");
    auto closeConnect = ScopeExit([&] { WinHttpCloseHandle(connect); });

    HINTERNET req = WinHttpOpenRequest(connect, method.c_str(), pathAndQuery.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, secure ? WINHTTP_FLAG_SECURE : 0);
    if (!req) throw std::runtime_error("WinHttpOpenRequest failed");
    auto closeReq = ScopeExit([&] { WinHttpCloseHandle(req); });

    for (const auto &h : headers) {
        if (!WinHttpAddRequestHeaders(req, h.c_str(), (DWORD)-1, WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE)) {
            throw std::runtime_error("WinHttpAddRequestHeaders failed");
        }
    }

    const void *bodyPtr = body ? body->data() : WINHTTP_NO_REQUEST_DATA;
    const DWORD bodyLen = body ? (DWORD)body->size() : 0;
    if (!WinHttpSendRequest(req, WINHTTP_NO_ADDITIONAL_HEADERS, 0, (LPVOID)bodyPtr, bodyLen, bodyLen, 0)) {
        throw std::runtime_error("WinHttpSendRequest failed");
    }
    if (!WinHttpReceiveResponse(req, nullptr)) throw std::runtime_error("WinHttpReceiveResponse failed");

    HttpResponse response;
    DWORD statusSize = sizeof(response.status);
    WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &response.status, &statusSize, WINHTTP_NO_HEADER_INDEX);

    DWORD headerSize = 0;
    WinHttpQueryHeaders(req, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX, nullptr, &headerSize, WINHTTP_NO_HEADER_INDEX);
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        std::wstring raw(headerSize / sizeof(wchar_t), L'\0');
        if (WinHttpQueryHeaders(req, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX, raw.data(), &headerSize, WINHTTP_NO_HEADER_INDEX)) {
            std::wistringstream ss(raw);
            std::wstring line;
            while (std::getline(ss, line)) {
                if (!line.empty() && line.back() == L'\r') line.pop_back();
                const auto colon = line.find(L':');
                if (colon != std::wstring::npos) {
                    response.headers[narrowHeaderName(line.substr(0, colon))] = trim(wideToUtf8(line.substr(colon + 1)));
                }
            }
        }
    }

    std::ofstream fileOut;
    if (target) {
        fileOut.open(*target, std::ios::binary | std::ios::trunc);
        if (!fileOut) throw std::runtime_error("Failed to create download target");
    }

    for (;;) {
        abort.check();
        DWORD available = 0;
        if (!WinHttpQueryDataAvailable(req, &available)) throw std::runtime_error("WinHttpQueryDataAvailable failed");
        if (available == 0) break;
        std::vector<char> buffer(available);
        DWORD read = 0;
        if (!WinHttpReadData(req, buffer.data(), available, &read)) throw std::runtime_error("WinHttpReadData failed");
        if (read == 0) break;
        if (target) fileOut.write(buffer.data(), read);
        else response.body.append(buffer.data(), read);
    }

    if (response.status < 200 || response.status >= 300) {
        throw std::runtime_error((PFC_string_formatter() << "HTTP " << response.status << ": " << response.body.c_str()).get_ptr());
    }
    return response;
}

}
