#pragma once

namespace gds {

struct HttpResponse {
    DWORD status = 0;
    std::string body;
    std::map<std::string, std::string> headers;
};

class HttpClient {
public:
    HttpResponse get(const std::wstring &url, const std::vector<std::wstring> &headers, abort_callback &abort);
    HttpResponse postForm(const std::wstring &url, const std::string &body, const std::vector<std::wstring> &headers, abort_callback &abort);
    void downloadToFile(const std::wstring &url, const std::vector<std::wstring> &headers, const std::wstring &target, abort_callback &abort);

private:
    HttpResponse request(const std::wstring &method, const std::wstring &url, const std::string *body, const std::vector<std::wstring> &headers, const std::wstring *target, abort_callback &abort);
};

}
