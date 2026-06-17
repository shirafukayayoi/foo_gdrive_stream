#include "stdafx.h"
#include "core_utils.h"
#include "config.h"
#include "oauth.h"
#include "http_client.h"
#include "drive_client.h"

namespace gds {
namespace {
std::mutex g_filesMutex;
std::vector<DriveFile> g_files;

static constexpr const char *driveFolderMime = "application/vnd.google-apps.folder";

static std::string joinDisplayPath(const std::string &folderPath, const std::string &name) {
    if (folderPath.empty()) return name;
    return folderPath + "/" + name;
}

static bool canDownload(const nlohmann::json &item) {
    if (!item.contains("capabilities") || !item["capabilities"].contains("canDownload")) return true;
    return item["capabilities"]["canDownload"].get<bool>();
}

static DriveFile driveFileFromJson(const nlohmann::json &item, const std::string &parentId, const std::string &folderPath) {
    DriveFile f;
    f.id = item.value("id", "");
    f.name = item.value("name", "");
    f.displayPath = joinDisplayPath(folderPath, f.name);
    f.parentId = parentId;
    f.mimeType = item.value("mimeType", "");
    f.modifiedTime = item.value("modifiedTime", "");
    f.size = item.value("size", "");
    f.canDownload = canDownload(item);
    return f;
}

static std::vector<DriveFile> listDirectChildren(const std::string &folderId, const std::string &folderPath, const std::string &token, abort_callback &abort) {
    const auto q = "'" + folderId + "' in parents and trashed=false";
    std::vector<DriveFile> children;
    std::string pageToken;
    HttpClient http;

    do {
        std::string url = "https://www.googleapis.com/drive/v3/files"
            "?q=" + urlEncode(q) +
            "&fields=nextPageToken,files(id,name,mimeType,modifiedTime,size,parents,capabilities/canDownload)"
            "&pageSize=1000"
            "&orderBy=name";
        if (!pageToken.empty()) url += "&pageToken=" + urlEncode(pageToken);

        auto res = http.get(utf8ToWide(url), {utf8ToWide("Authorization: Bearer " + token)}, abort);
        const auto json = nlohmann::json::parse(res.body);
        for (const auto &item : json.value("files", nlohmann::json::array())) {
            auto f = driveFileFromJson(item, folderId, folderPath);
            if (!f.id.empty()) children.push_back(std::move(f));
        }
        pageToken = json.value("nextPageToken", "");
    } while (!pageToken.empty());

    return children;
}

static int artworkScore(const DriveFile &file) {
    auto name = file.name;
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    const auto dot = name.find_last_of('.');
    const auto base = dot == std::string::npos ? name : name.substr(0, dot);

    if (base == "cover") return 100;
    if (base == "folder") return 95;
    if (base == "front") return 90;
    if (base == "album") return 85;
    if (base == "artwork") return 80;
    if (base.find("cover") != std::string::npos) return 70;
    if (base.find("front") != std::string::npos) return 65;
    return 10;
}

static std::optional<DriveFile> chooseArtwork(const std::vector<DriveFile> &images) {
    if (images.empty()) return std::nullopt;
    return *std::max_element(images.begin(), images.end(), [](const DriveFile &a, const DriveFile &b) {
        return artworkScore(a) < artworkScore(b);
    });
}

static void attachArtwork(DriveFile &file, const std::optional<DriveFile> &artwork) {
    if (!artwork) return;
    file.artworkId = artwork->id;
    file.artworkName = artwork->name;
    file.artworkMimeType = artwork->mimeType;
    file.artworkModifiedTime = artwork->modifiedTime;
}

static void listFolderRecursive(
    const std::string &folderId,
    const std::string &folderPath,
    const std::optional<DriveFile> &inheritedArtwork,
    const std::string &token,
    abort_callback &abort,
    std::vector<DriveFile> &out,
    std::vector<std::string> &visited,
    unsigned depth) {
    if (depth > 32) throw std::runtime_error("Google Drive folder nesting is too deep");
    if (std::find(visited.begin(), visited.end(), folderId) != visited.end()) return;
    visited.push_back(folderId);

    const auto children = listDirectChildren(folderId, folderPath, token, abort);
    std::vector<DriveFile> folders;
    std::vector<DriveFile> audios;
    std::vector<DriveFile> images;

    for (const auto &item : children) {
        if (item.mimeType == driveFolderMime) {
            folders.push_back(item);
        } else if (item.canDownload && (isLikelyAudioMime(item.mimeType) || isLikelyAudioName(item.name))) {
            audios.push_back(item);
        } else if (item.canDownload && (isLikelyImageMime(item.mimeType) || isLikelyImageName(item.name))) {
            images.push_back(item);
        }
    }

    auto artwork = chooseArtwork(images);
    if (!artwork) artwork = inheritedArtwork;
    for (auto &audio : audios) {
        attachArtwork(audio, artwork);
        out.push_back(std::move(audio));
    }

    for (const auto &folder : folders) {
        listFolderRecursive(folder.id, folder.displayPath, artwork, token, abort, out, visited, depth + 1);
    }
}
}

std::vector<DriveFile> latestFileList() {
    std::lock_guard<std::mutex> lock(g_filesMutex);
    return g_files;
}

void setLatestFileList(std::vector<DriveFile> files) {
    std::lock_guard<std::mutex> lock(g_filesMutex);
    g_files = std::move(files);
}

std::vector<DriveFile> DriveClient::listFolder(abort_callback &abort) {
    const auto folderId = folderIdSetting();
    return listFolderById(folderId, abort);
}

std::vector<DriveFile> DriveClient::listFolderById(const std::string &folderId, abort_callback &abort) {
    if (folderId.empty()) throw std::runtime_error("Google Drive folder ID is not configured");
    const auto token = getAccessToken(abort);
    std::vector<DriveFile> files;
    std::vector<std::string> visited;
    listFolderRecursive(folderId, "", std::nullopt, token, abort, files, visited, 0);
    setLatestFileList(files);
    return files;
}

std::optional<DriveFile> DriveClient::getFile(const std::string &fileId, abort_callback &abort) {
    for (const auto &file : latestFileList()) {
        if (file.id == fileId) return file;
    }
    const auto token = getAccessToken(abort);
    HttpClient http;
    auto res = http.get(utf8ToWide("https://www.googleapis.com/drive/v3/files/" + urlEncode(fileId) + "?fields=id,name,mimeType,modifiedTime,size"), {utf8ToWide("Authorization: Bearer " + token)}, abort);
    const auto item = nlohmann::json::parse(res.body);
    DriveFile f;
    f.id = item.value("id", "");
    f.name = item.value("name", "");
    f.mimeType = item.value("mimeType", "");
    f.modifiedTime = item.value("modifiedTime", "");
    f.size = item.value("size", "");
    if (f.id.empty()) return std::nullopt;
    return f;
}

void DriveClient::downloadFile(const std::string &fileId, const std::wstring &target, abort_callback &abort) {
    const auto token = getAccessToken(abort);
    HttpClient http;
    http.downloadToFile(utf8ToWide("https://www.googleapis.com/drive/v3/files/" + urlEncode(fileId) + "?alt=media"), {utf8ToWide("Authorization: Bearer " + token)}, target, abort);
}

}
