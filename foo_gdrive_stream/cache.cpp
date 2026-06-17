#include "stdafx.h"
#include "core_utils.h"
#include "config.h"
#include "drive_client.h"
#include "cache.h"

namespace gds {

std::wstring CacheManager::pathFor(const DriveFile &file) const {
    std::string base = sanitizeFileName(file.id + "_" + file.modifiedTime + "_" + file.name);
    if (base.size() > 180) {
        const auto ext = extensionOf(base);
        base = base.substr(0, 160) + (ext.empty() ? "" : "." + ext);
    }
    return pathJoin(cacheDirSetting(), utf8ToWide(base));
}

std::wstring CacheManager::ensureCachedFile(const DriveFile &file, abort_callback &abort) {
    if (file.id.empty()) throw std::runtime_error("Invalid Google Drive file");
    ensureDirectory(cacheDirSetting());
    const auto path = pathFor(file);
    if (GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES) {
        const auto temp = path + L".tmp";
        DriveClient client;
        client.downloadFile(file.id, temp, abort);
        MoveFileExW(temp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING);
        prune();
    }
    return path;
}

std::wstring CacheManager::ensureCached(const std::string &gdrivePath, abort_callback &abort) {
    const auto fileId = fileIdFromGdrivePath(gdrivePath.c_str());
    if (fileId.empty()) throw std::runtime_error("Invalid gdrive path");
    DriveClient client;
    auto file = client.getFile(fileId, abort);
    if (!file) {
        file = DriveFile{};
        file->id = fileId;
        file->name = fileNameFromGdrivePath(gdrivePath.c_str());
    }
    return ensureCachedFile(*file, abort);
}

void CacheManager::prune() {
    const auto limit = (uint64_t)cacheLimitMb() * 1024ull * 1024ull;
    const auto dir = cacheDirSetting();
    WIN32_FIND_DATAW fd{};
    HANDLE h = FindFirstFileW(pathJoin(dir, L"*").c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    struct Entry { std::wstring path; uint64_t size; FILETIME access; };
    std::vector<Entry> entries;
    uint64_t total = 0;
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        Entry e;
        e.path = pathJoin(dir, fd.cFileName);
        e.size = ((uint64_t)fd.nFileSizeHigh << 32) | fd.nFileSizeLow;
        e.access = fd.ftLastAccessTime;
        entries.push_back(e);
        total += e.size;
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    std::sort(entries.begin(), entries.end(), [](const Entry &a, const Entry &b) {
        return CompareFileTime(&a.access, &b.access) < 0;
    });
    for (const auto &e : entries) {
        if (total <= limit) break;
        if (DeleteFileW(e.path.c_str())) total -= e.size;
    }
}

}
