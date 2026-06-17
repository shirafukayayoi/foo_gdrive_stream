#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace gds {

template <typename Fn>
class ScopeExit {
public:
    explicit ScopeExit(Fn fn) : m_fn(fn) {}
    ~ScopeExit() { m_fn(); }
private:
    Fn m_fn;
};

struct DriveFile {
    std::string id;
    std::string name;
    std::string displayPath;
    std::string parentId;
    std::string mimeType;
    std::string modifiedTime;
    std::string size;
    std::string artworkId;
    std::string artworkName;
    std::string artworkMimeType;
    std::string artworkModifiedTime;
    bool canDownload = true;
};

std::string trim(const std::string &value);
std::string urlEncode(const std::string &value);
std::string urlDecode(const std::string &value);
std::string base64UrlEncode(const std::vector<uint8_t> &bytes);
std::vector<uint8_t> randomBytes(size_t count);
std::string sha256Base64Url(const std::string &value);
std::string extractFolderId(const std::string &input);
std::string sanitizeFileName(const std::string &name);
std::string extensionOf(const std::string &name);
bool isLikelyAudioMime(const std::string &mime);
bool isLikelyAudioName(const std::string &name);
bool isLikelyImageMime(const std::string &mime);
bool isLikelyImageName(const std::string &name);
std::string makeGdrivePath(const DriveFile &file);
std::string makeGdriveFolderPath(const std::string &folderId, const std::string &displayName);
bool isGdrivePath(const char *path);
bool isGdriveFolderPath(const char *path);
bool isGdriveFilePath(const char *path);
std::string fileIdFromGdrivePath(const char *path);
std::string fileNameFromGdrivePath(const char *path);
std::string artworkIdFromGdrivePath(const char *path);
std::string artworkNameFromGdrivePath(const char *path);
std::string artworkMimeTypeFromGdrivePath(const char *path);
std::string artworkModifiedTimeFromGdrivePath(const char *path);
std::string folderIdFromGdrivePath(const char *path);
std::string folderNameFromGdrivePath(const char *path);
std::string readTextFile(const std::wstring &path);
void writeBinaryFile(const std::wstring &path, const std::vector<uint8_t> &bytes);
std::vector<uint8_t> readBinaryFile(const std::wstring &path);
std::wstring utf8ToWide(const std::string &value);
std::string wideToUtf8(const std::wstring &value);
std::wstring pathJoin(const std::wstring &left, const std::wstring &right);
void ensureDirectory(const std::wstring &path);
std::wstring profileNativePath();
std::wstring defaultCacheDir();
std::wstring tokenPath();

}
