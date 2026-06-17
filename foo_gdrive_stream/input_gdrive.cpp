#include "stdafx.h"
#include "guids.h"
#include "core_utils.h"
#include "cache.h"
#include <SDK/album_art_helpers.h>

namespace gds {
namespace {
static std::string baseNameFromDisplayPath(const std::string &path) {
    const auto slash = path.find_last_of('/');
    return slash == std::string::npos ? path : path.substr(slash + 1);
}

static std::string folderNameFromDisplayPath(const std::string &path) {
    const auto slash = path.find_last_of('/');
    return slash == std::string::npos ? std::string{} : path.substr(0, slash);
}
}

class input_gdrive : public input_stubs {
public:
    void open(service_ptr_t<file>, const char *path, t_input_open_reason reason, abort_callback &abort) {
        if (reason == input_open_info_write) throw exception_tagging_unsupported();
        m_sourcePath = path ? path : "";
        m_isFolder = isGdriveFolderPath(m_sourcePath.c_str());
        if (m_isFolder) return;
        CacheManager cache;
        m_cachedPath = cache.ensureCached(m_sourcePath.c_str(), abort);
        m_cachedPathUtf8 = wideToUtf8(m_cachedPath);
    }

    void get_info(file_info &info, abort_callback &abort) {
        if (m_isFolder) {
            const auto name = folderNameFromGdrivePath(m_sourcePath.c_str());
            const auto folderId = folderIdFromGdrivePath(m_sourcePath.c_str());
            info.meta_set("title", name.empty() ? "Google Drive Folder" : name.c_str());
            info.meta_set("album", "Google Drive folders");
            info.info_set("encoding", "Google Drive folder feed");
            info.info_set("gdrive_folder_id", folderId.c_str());
            return;
        }
        input_helper::g_get_info(make_playable_location(m_cachedPathUtf8.c_str(), 0), info, abort, true);
        const auto displayName = fileNameFromGdrivePath(m_sourcePath.c_str());
        const auto baseName = baseNameFromDisplayPath(displayName);
        const auto folderName = folderNameFromDisplayPath(displayName);
        if (!baseName.empty() && !info.meta_exists("title")) info.meta_set("title", baseName.c_str());
        if (!folderName.empty() && !info.meta_exists("album")) info.meta_set("album", folderName.c_str());
        info.info_set("encoding", "Google Drive cached");
    }

    t_filestats get_file_stats(abort_callback &abort) {
        if (m_isFolder) return filestats_invalid;
        service_ptr_t<file> f;
        filesystem::g_open_read(f, m_cachedPathUtf8.c_str(), abort);
        return f->get_stats(abort);
    }

    t_filestats2 get_stats2(unsigned flags, abort_callback &abort) {
        if (m_isFolder) return filestats2_invalid;
        service_ptr_t<file> f;
        filesystem::g_open_read(f, m_cachedPathUtf8.c_str(), abort);
        return f->get_stats2_(flags, abort);
    }

    void decode_initialize(unsigned flags, abort_callback &abort) {
        if (m_isFolder) throw exception_io_unsupported_format();
        input_helper::decodeOpen_t openArgs;
        openArgs.m_from_redirect = true;
        openArgs.m_flags = flags;
        m_input.open(make_playable_location(m_cachedPathUtf8.c_str(), 0), abort, openArgs);
    }

    bool decode_run(audio_chunk &chunk, abort_callback &abort) {
        return m_input.run(chunk, abort);
    }

    void decode_seek(double seconds, abort_callback &abort) {
        m_input.seek(seconds, abort);
    }

    bool decode_can_seek() {
        return m_input.can_seek();
    }

    bool decode_get_dynamic_info(file_info &out, double &delta) {
        return m_input.get_dynamic_info(out, delta);
    }

    bool decode_get_dynamic_info_track(file_info &out, double &delta) {
        return m_input.get_dynamic_info_track(out, delta);
    }

    void decode_on_idle(abort_callback &abort) {
        m_input.on_idle(abort);
    }

    void retag(const file_info &, abort_callback &) { throw exception_tagging_unsupported(); }
    void remove_tags(abort_callback &) { throw exception_tagging_unsupported(); }

    static bool g_is_our_content_type(const char *) { return false; }
    static bool g_is_our_path(const char *path, const char *) {
        return isGdrivePath(path);
    }
    static const char *g_get_name() { return "Google Drive Stream"; }
    static GUID g_get_guid() { return guid_input; }

private:
    std::string m_sourcePath;
    std::wstring m_cachedPath;
    std::string m_cachedPathUtf8;
    bool m_isFolder = false;
    input_helper m_input;
};

static input_singletrack_factory_t<input_gdrive, input_entry::flag_redirect> g_input_factory;

class GDriveAlbumArtInstance : public album_art_extractor_instance {
public:
    explicit GDriveAlbumArtInstance(const char *path) : m_path(path ? path : "") {}

    album_art_data_ptr query(const GUID &what, abort_callback &abort) override {
        if (what != album_art_ids::cover_front) throw exception_album_art_not_found();

        DriveFile artwork;
        artwork.id = artworkIdFromGdrivePath(m_path.c_str());
        if (artwork.id.empty()) throw exception_album_art_not_found();
        artwork.name = artworkNameFromGdrivePath(m_path.c_str());
        if (artwork.name.empty()) artwork.name = "cover";
        artwork.mimeType = artworkMimeTypeFromGdrivePath(m_path.c_str());
        artwork.modifiedTime = artworkModifiedTimeFromGdrivePath(m_path.c_str());

        CacheManager cache;
        const auto nativePath = cache.ensureCachedFile(artwork, abort);
        const auto bytes = readBinaryFile(nativePath);
        if (bytes.empty()) throw exception_album_art_not_found();
        return album_art_data_impl::g_create(bytes.data(), bytes.size());
    }

private:
    std::string m_path;
};

class GDriveAlbumArtExtractor : public album_art_extractor_v2 {
public:
    bool is_our_path(const char *path, const char *) override {
        return isGdriveFilePath(path);
    }

    album_art_extractor_instance_ptr open(file_ptr, const char *path, abort_callback &) override {
        return new service_impl_t<GDriveAlbumArtInstance>(path);
    }

    GUID get_guid() override {
        return guid_input;
    }
};

static service_factory_single_t<GDriveAlbumArtExtractor> g_album_art_factory;

}
