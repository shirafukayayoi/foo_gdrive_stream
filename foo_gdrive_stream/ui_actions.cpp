#include "stdafx.h"
#include "core_utils.h"
#include "config.h"
#include "oauth.h"
#include "drive_client.h"
#include "ui_actions.h"
#include "resource.h"

namespace gds {

static void showError(HWND parent, const char *prefix, const std::exception &e) {
    const auto msg = std::string(prefix) + "\n\n" + e.what();
    popup_message::g_show(msg.c_str(), "foo_gdrive_stream");
    if (parent) MessageBoxW(parent, utf8ToWide(msg).c_str(), L"foo_gdrive_stream", MB_ICONERROR);
}

class LoadFolderDialog : public CDialogImpl<LoadFolderDialog> {
public:
    enum { IDD = IDD_LOAD_FOLDER };

    std::string folderInput;
    std::string playlistName;

    BEGIN_MSG_MAP_EX(LoadFolderDialog)
        MSG_WM_INITDIALOG(onInitDialog)
        COMMAND_ID_HANDLER_EX(IDOK, onOk)
        COMMAND_ID_HANDLER_EX(IDCANCEL, onCancel)
    END_MSG_MAP()

    BOOL onInitDialog(CWindow, LPARAM) {
        const auto configuredFolder = folderSetting();
        SetDlgItemText(IDC_EDIT_LOAD_FOLDER, pfc::stringcvt::string_os_from_utf8(configuredFolder.c_str()));
        SetDlgItemText(IDC_EDIT_LOAD_PLAYLIST, L"Google Drive Folder");
        CenterWindow(GetParent());
        return TRUE;
    }

    void onOk(UINT, int, CWindow) {
        wchar_t folderBuffer[4096] = {};
        wchar_t nameBuffer[512] = {};
        GetDlgItemText(IDC_EDIT_LOAD_FOLDER, folderBuffer, (int)std::size(folderBuffer));
        GetDlgItemText(IDC_EDIT_LOAD_PLAYLIST, nameBuffer, (int)std::size(nameBuffer));

        folderInput = wideToUtf8(folderBuffer);
        playlistName = wideToUtf8(nameBuffer);
        EndDialog(IDOK);
    }

    void onCancel(UINT, int, CWindow) {
        EndDialog(IDCANCEL);
    }
};

void authenticateNow(HWND parent) {
    try {
        abort_callback_dummy abort;
        runInteractiveAuth(abort);
        popup_message::g_show("Google Drive authentication completed.", "foo_gdrive_stream");
    } catch (const std::exception &e) {
        showError(parent, "Google Drive authentication failed.", e);
    }
}

std::vector<DriveFile> refreshDriveFiles(HWND parent) {
    try {
        abort_callback_dummy abort;
        DriveClient client;
        auto files = client.listFolder(abort);
        FB2K_console_formatter() << "foo_gdrive_stream: loaded " << (t_size)files.size() << " audio files";
        return files;
    } catch (const std::exception &e) {
        showError(parent, "Google Drive folder refresh failed.", e);
        return {};
    }
}

void addFilesToActivePlaylist(const std::vector<DriveFile> &files, HWND parent) {
    if (files.empty()) return;
    std::vector<std::string> paths;
    paths.reserve(files.size());
    pfc::list_t<const char *> urls;
    for (const auto &file : files) {
        paths.push_back(makeGdrivePath(file));
    }
    for (const auto &path : paths) urls.add_item(path.c_str());
    playlist_manager::get()->activeplaylist_add_locations(urls, true, parent);
}

void loadFolderAsNewPlaylist(HWND parent) {
    try {
        LoadFolderDialog dialog;
        if (dialog.DoModal(parent) != IDOK) return;

        const auto folderId = extractFolderId(dialog.folderInput);
        if (folderId.empty()) throw std::runtime_error("Google Drive folder ID or URL is required");

        auto playlistName = trim(dialog.playlistName);
        if (playlistName.empty()) playlistName = "Google Drive Folder";

        const auto feedPath = makeGdriveFolderPath(folderId, playlistName);
        auto manager = playlist_manager::get();
        const t_size playlistIndex = manager->create_playlist(playlistName.c_str(), pfc_infinite, pfc_infinite);
        if (playlistIndex == pfc_infinite) throw std::runtime_error("Failed to create playlist");

        manager->set_active_playlist(playlistIndex);
        pfc::list_t<const char *> urls;
        urls.add_item(feedPath.c_str());
        if (!manager->activeplaylist_add_locations(urls, true, parent)) {
            throw std::runtime_error("Failed to add Google Drive folder feed to the playlist");
        }

        refreshFolderFeedInActivePlaylist(feedPath.c_str(), parent);
    } catch (const std::exception &e) {
        showError(parent, "Loading Google Drive folder failed.", e);
    }
}

static void insertFilesAfterActivePath(const std::vector<DriveFile> &files, const char *afterPath, HWND parent) {
    std::vector<std::string> paths;
    paths.reserve(files.size());
    pfc::list_t<const char *> urls;
    for (const auto &file : files) {
        paths.push_back(makeGdrivePath(file));
    }
    for (const auto &path : paths) urls.add_item(path.c_str());

    t_size insertAt = playlist_manager::get()->activeplaylist_get_item_count();
    if (afterPath && *afterPath) {
        const auto count = playlist_manager::get()->activeplaylist_get_item_count();
        for (t_size i = 0; i < count; ++i) {
            metadb_handle_ptr item;
            if (playlist_manager::get()->activeplaylist_get_item_handle(item, i) && metadb::path_compare(item->get_path(), afterPath) == 0) {
                insertAt = i + 1;
                break;
            }
        }
    }
    if (urls.get_count() > 0) {
        playlist_manager::get()->activeplaylist_insert_locations(insertAt, urls, true, parent);
    }
}

static bool findActivePathIndex(const char *path, t_size &outIndex) {
    outIndex = pfc_infinite;
    if (!path || !*path) return false;
    const auto count = playlist_manager::get()->activeplaylist_get_item_count();
    for (t_size i = 0; i < count; ++i) {
        metadb_handle_ptr item;
        if (playlist_manager::get()->activeplaylist_get_item_handle(item, i) && metadb::path_compare(item->get_path(), path) == 0) {
            outIndex = i;
            return true;
        }
    }
    return false;
}

static void removeExpandedChildrenAfter(t_size feedIndex) {
    const auto count = playlist_manager::get()->activeplaylist_get_item_count();
    if (feedIndex == pfc_infinite || feedIndex + 1 >= count) return;

    pfc::bit_array_bittable removeMask(count);
    for (t_size i = feedIndex + 1; i < count; ++i) {
        metadb_handle_ptr item;
        if (!playlist_manager::get()->activeplaylist_get_item_handle(item, i)) break;
        const char *path = item->get_path();
        if (!isGdriveFilePath(path)) break;
        removeMask.set(i, true);
    }
    playlist_manager::get()->activeplaylist_remove_items(removeMask);
}

void addAllFolderFilesToPlaylist(HWND parent) {
    auto files = latestFileList();
    if (files.empty()) files = refreshDriveFiles(parent);
    addFilesToActivePlaylist(files, parent);
}

void addConfiguredFolderFeedToPlaylist(HWND parent) {
    try {
        const auto folderId = folderIdSetting();
        if (folderId.empty()) throw std::runtime_error("Google Drive folder ID is not configured");
        const auto path = makeGdriveFolderPath(folderId, "Google Drive Folder");
        pfc::list_t<const char *> urls;
        urls.add_item(path.c_str());
        playlist_manager::get()->activeplaylist_add_locations(urls, true, parent);
    } catch (const std::exception &e) {
        showError(parent, "Adding Google Drive folder feed failed.", e);
    }
}

void expandFolderFeedToActivePlaylist(const char *folderPath, HWND parent) {
    try {
        const auto folderId = folderIdFromGdrivePath(folderPath);
        if (folderId.empty()) throw std::runtime_error("Selected item is not a Google Drive folder feed");
        abort_callback_dummy abort;
        DriveClient client;
        auto files = client.listFolderById(folderId, abort);
        insertFilesAfterActivePath(files, folderPath, parent);
        FB2K_console_formatter() << "foo_gdrive_stream: expanded folder feed into " << (t_size)files.size() << " audio files";
    } catch (const std::exception &e) {
        showError(parent, "Expanding Google Drive folder feed failed.", e);
    }
}

void refreshFolderFeedInActivePlaylist(const char *folderPath, HWND parent) {
    try {
        const std::string feedPath = folderPath ? folderPath : "";
        const auto folderId = folderIdFromGdrivePath(feedPath.c_str());
        if (folderId.empty()) throw std::runtime_error("Selected item is not a Google Drive folder feed");

        abort_callback_dummy abort;
        DriveClient client;
        auto files = client.listFolderById(folderId, abort);

        t_size feedIndex = pfc_infinite;
        if (!findActivePathIndex(feedPath.c_str(), feedIndex)) {
            throw std::runtime_error("Folder feed is not present in the active playlist");
        }

        playlist_manager::get()->activeplaylist_undo_backup();
        removeExpandedChildrenAfter(feedIndex);
        insertFilesAfterActivePath(files, feedPath.c_str(), parent);
        FB2K_console_formatter() << "foo_gdrive_stream: refreshed folder feed with " << (t_size)files.size() << " audio files";
    } catch (const std::exception &e) {
        showError(parent, "Refreshing Google Drive folder feed failed.", e);
    }
}

}
