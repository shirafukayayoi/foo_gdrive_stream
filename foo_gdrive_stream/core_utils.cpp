#ifndef GDS_CORE_UTILS_STANDALONE
#include "stdafx.h"
#else
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <bcrypt.h>
#include <wincrypt.h>
#include <shlobj.h>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <optional>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#endif
#include "core_utils.h"

namespace gds {

std::string trim(const std::string &value) {
    const auto begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) return {};
    const auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(begin, end - begin + 1);
}

std::wstring utf8ToWide(const std::string &value) {
    if (value.empty()) return {};
    const int count = MultiByteToWideChar(CP_UTF8, 0, value.data(), (int)value.size(), nullptr, 0);
    std::wstring out(count, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.data(), (int)value.size(), out.data(), count);
    return out;
}

std::string wideToUtf8(const std::wstring &value) {
    if (value.empty()) return {};
    const int count = WideCharToMultiByte(CP_UTF8, 0, value.data(), (int)value.size(), nullptr, 0, nullptr, nullptr);
    std::string out(count, '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.data(), (int)value.size(), out.data(), count, nullptr, nullptr);
    return out;
}

std::string urlEncode(const std::string &value) {
    std::ostringstream out;
    out << std::hex << std::uppercase;
    for (const unsigned char c : value) {
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
            out << c;
        } else {
            out << '%' << std::setw(2) << std::setfill('0') << (int)c;
        }
    }
    return out.str();
}

static int hexValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + c - 'a';
    if (c >= 'A' && c <= 'F') return 10 + c - 'A';
    return -1;
}

std::string urlDecode(const std::string &value) {
    std::string out;
    for (size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '%' && i + 2 < value.size()) {
            const int hi = hexValue(value[i + 1]);
            const int lo = hexValue(value[i + 2]);
            if (hi >= 0 && lo >= 0) {
                out.push_back((char)((hi << 4) | lo));
                i += 2;
                continue;
            }
        }
        out.push_back(value[i] == '+' ? ' ' : value[i]);
    }
    return out;
}

std::string base64UrlEncode(const std::vector<uint8_t> &bytes) {
    DWORD size = 0;
    if (!CryptBinaryToStringA(bytes.data(), (DWORD)bytes.size(), CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, nullptr, &size)) {
        throw std::runtime_error("CryptBinaryToStringA failed");
    }
    std::string out(size, '\0');
    if (!CryptBinaryToStringA(bytes.data(), (DWORD)bytes.size(), CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, out.data(), &size)) {
        throw std::runtime_error("CryptBinaryToStringA failed");
    }
    if (!out.empty() && out.back() == '\0') out.pop_back();
    for (char &c : out) {
        if (c == '+') c = '-';
        else if (c == '/') c = '_';
    }
    while (!out.empty() && out.back() == '=') out.pop_back();
    return out;
}

std::vector<uint8_t> randomBytes(size_t count) {
    std::vector<uint8_t> bytes(count);
    BCRYPT_ALG_HANDLE alg = nullptr;
    if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_RNG_ALGORITHM, nullptr, 0) != 0) throw std::runtime_error("BCryptOpenAlgorithmProvider RNG failed");
    ScopeExit close([&] { BCryptCloseAlgorithmProvider(alg, 0); });
    if (BCryptGenRandom(alg, bytes.data(), (ULONG)bytes.size(), 0) != 0) throw std::runtime_error("BCryptGenRandom failed");
    return bytes;
}

std::string sha256Base64Url(const std::string &value) {
    BCRYPT_ALG_HANDLE alg = nullptr;
    if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0) throw std::runtime_error("BCryptOpenAlgorithmProvider SHA256 failed");
    ScopeExit closeAlg([&] { BCryptCloseAlgorithmProvider(alg, 0); });
    BCRYPT_HASH_HANDLE hash = nullptr;
    if (BCryptCreateHash(alg, &hash, nullptr, 0, nullptr, 0, 0) != 0) throw std::runtime_error("BCryptCreateHash failed");
    ScopeExit closeHash([&] { BCryptDestroyHash(hash); });
    if (BCryptHashData(hash, (PUCHAR)value.data(), (ULONG)value.size(), 0) != 0) throw std::runtime_error("BCryptHashData failed");
    std::vector<uint8_t> digest(32);
    if (BCryptFinishHash(hash, digest.data(), (ULONG)digest.size(), 0) != 0) throw std::runtime_error("BCryptFinishHash failed");
    return base64UrlEncode(digest);
}

std::string extractFolderId(const std::string &input) {
    const std::string v = trim(input);
    if (v.empty()) return {};
    std::smatch m;
    const std::regex foldersRe(R"(/folders/([A-Za-z0-9_-]+))");
    if (std::regex_search(v, m, foldersRe)) return m[1].str();
    const std::regex idRe(R"([?&]id=([A-Za-z0-9_-]+))");
    if (std::regex_search(v, m, idRe)) return m[1].str();
    return v;
}

std::string sanitizeFileName(const std::string &name) {
    std::wstring w = utf8ToWide(name.empty() ? "untitled" : name);
    for (wchar_t &c : w) {
        if (c < 32 || wcschr(L"<>:\"/\\|?*", c)) c = L'_';
    }
    while (!w.empty() && (w.back() == L'.' || w.back() == L' ')) w.pop_back();
    if (w.empty()) w = L"untitled";
    return wideToUtf8(w);
}

std::string extensionOf(const std::string &name) {
    const auto pos = name.find_last_of('.');
    if (pos == std::string::npos) return {};
    std::string ext = name.substr(pos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    return ext;
}

bool isLikelyAudioMime(const std::string &mime) {
    return mime.rfind("audio/", 0) == 0 || mime == "application/ogg";
}

bool isLikelyAudioName(const std::string &name) {
    static const char *exts[] = {"mp3", "flac", "m4a", "aac", "ogg", "opus", "wav", "wv", "aiff", "aif", "ape"};
    const auto ext = extensionOf(name);
    for (auto e : exts) if (ext == e) return true;
    return false;
}

bool isLikelyImageMime(const std::string &mime) {
    return mime == "image/jpeg" || mime == "image/png" || mime == "image/webp";
}

bool isLikelyImageName(const std::string &name) {
    static const char *exts[] = {"jpg", "jpeg", "png", "webp"};
    const auto ext = extensionOf(name);
    for (auto e : exts) if (ext == e) return true;
    return false;
}

std::string makeGdrivePath(const DriveFile &file) {
    std::string path = "gdrive://" + file.id + "/" + urlEncode(file.displayPath.empty() ? file.name : file.displayPath);
    if (!file.artworkId.empty()) {
        path += "?art=" + urlEncode(file.artworkId);
        if (!file.artworkName.empty()) path += "&artname=" + urlEncode(file.artworkName);
        if (!file.artworkMimeType.empty()) path += "&artmime=" + urlEncode(file.artworkMimeType);
        if (!file.artworkModifiedTime.empty()) path += "&artmtime=" + urlEncode(file.artworkModifiedTime);
    }
    return path;
}

std::string makeGdriveFolderPath(const std::string &folderId, const std::string &displayName) {
    return "gdrive://folder/" + urlEncode(folderId) + "/" + urlEncode(displayName.empty() ? "Google Drive Folder" : displayName);
}

bool isGdrivePath(const char *path) {
    const std::string p = path ? path : "";
    return p.rfind("gdrive://", 0) == 0;
}

bool isGdriveFolderPath(const char *path) {
    const std::string p = path ? path : "";
    return p.rfind("gdrive://folder/", 0) == 0;
}

bool isGdriveFilePath(const char *path) {
    return isGdrivePath(path) && !isGdriveFolderPath(path);
}

std::string fileIdFromGdrivePath(const char *path) {
    const std::string p = path ? path : "";
    const std::string prefix = "gdrive://";
    if (p.rfind(prefix, 0) != 0 || isGdriveFolderPath(path)) return {};
    const auto slash = p.find('/', prefix.size());
    if (slash == std::string::npos) return p.substr(prefix.size());
    return p.substr(prefix.size(), slash - prefix.size());
}

std::string fileNameFromGdrivePath(const char *path) {
    const std::string p = path ? path : "";
    const std::string prefix = "gdrive://";
    if (p.rfind(prefix, 0) != 0 || isGdriveFolderPath(path)) return {};
    const auto slash = p.find('/', prefix.size());
    if (slash == std::string::npos) return {};
    const auto query = p.find('?', slash + 1);
    return urlDecode(p.substr(slash + 1, query == std::string::npos ? std::string::npos : query - slash - 1));
}

static std::string queryValueFromGdrivePath(const char *path, const char *key) {
    const std::string p = path ? path : "";
    const auto query = p.find('?');
    if (query == std::string::npos) return {};
    const std::string needle = std::string(key) + "=";
    size_t pos = query + 1;
    while (pos < p.size()) {
        const auto next = p.find('&', pos);
        const auto item = p.substr(pos, next == std::string::npos ? std::string::npos : next - pos);
        if (item.rfind(needle, 0) == 0) return urlDecode(item.substr(needle.size()));
        if (next == std::string::npos) break;
        pos = next + 1;
    }
    return {};
}

std::string artworkIdFromGdrivePath(const char *path) {
    return queryValueFromGdrivePath(path, "art");
}

std::string artworkNameFromGdrivePath(const char *path) {
    return queryValueFromGdrivePath(path, "artname");
}

std::string artworkMimeTypeFromGdrivePath(const char *path) {
    return queryValueFromGdrivePath(path, "artmime");
}

std::string artworkModifiedTimeFromGdrivePath(const char *path) {
    return queryValueFromGdrivePath(path, "artmtime");
}

std::string folderIdFromGdrivePath(const char *path) {
    const std::string p = path ? path : "";
    const std::string prefix = "gdrive://folder/";
    if (p.rfind(prefix, 0) != 0) return {};
    const auto slash = p.find('/', prefix.size());
    const auto encoded = slash == std::string::npos ? p.substr(prefix.size()) : p.substr(prefix.size(), slash - prefix.size());
    return urlDecode(encoded);
}

std::string folderNameFromGdrivePath(const char *path) {
    const std::string p = path ? path : "";
    const std::string prefix = "gdrive://folder/";
    if (p.rfind(prefix, 0) != 0) return {};
    const auto first = p.find('/', prefix.size());
    if (first == std::string::npos) return {};
    return urlDecode(p.substr(first + 1));
}

std::string readTextFile(const std::wstring &path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("Failed to open file");
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

void writeBinaryFile(const std::wstring &path, const std::vector<uint8_t> &bytes) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) throw std::runtime_error("Failed to write file");
    out.write((const char *)bytes.data(), (std::streamsize)bytes.size());
}

std::vector<uint8_t> readBinaryFile(const std::wstring &path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("Failed to open file");
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

std::wstring pathJoin(const std::wstring &left, const std::wstring &right) {
    if (left.empty()) return right;
    if (left.back() == L'\\' || left.back() == L'/') return left + right;
    return left + L"\\" + right;
}

void ensureDirectory(const std::wstring &path) {
    if (path.empty()) return;
    SHCreateDirectoryExW(nullptr, path.c_str(), nullptr);
}

#ifndef GDS_CORE_UTILS_STANDALONE
std::wstring profileNativePath() {
    pfc::string8 profile;
    filesystem::g_get_native_path(core_api::get_profile_path(), profile);
    return utf8ToWide(profile.c_str());
}

std::wstring defaultCacheDir() {
    return pathJoin(profileNativePath(), L"foo_gdrive_stream_cache");
}

std::wstring tokenPath() {
    return pathJoin(profileNativePath(), L"foo_gdrive_stream_token.bin");
}
#endif

}
