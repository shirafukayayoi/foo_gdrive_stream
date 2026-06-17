#pragma once

#include "core_utils.h"

namespace gds {

void authenticateNow(HWND parent);
std::vector<DriveFile> refreshDriveFiles(HWND parent);
void addFilesToActivePlaylist(const std::vector<DriveFile> &files, HWND parent);
void loadFolderAsNewPlaylist(HWND parent);
void addAllFolderFilesToPlaylist(HWND parent);
void addConfiguredFolderFeedToPlaylist(HWND parent);
void expandFolderFeedToActivePlaylist(const char *folderPath, HWND parent);
void refreshFolderFeedInActivePlaylist(const char *folderPath, HWND parent);

}
