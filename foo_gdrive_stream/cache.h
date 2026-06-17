#pragma once

namespace gds {

class CacheManager {
public:
    std::wstring ensureCached(const std::string &gdrivePath, abort_callback &abort);
    std::wstring ensureCachedFile(const DriveFile &file, abort_callback &abort);
    std::wstring pathFor(const DriveFile &file) const;
    void prune();
};

}
