#pragma once

namespace gds {

class DriveClient {
public:
    std::vector<DriveFile> listFolder(abort_callback &abort);
    std::vector<DriveFile> listFolderById(const std::string &folderId, abort_callback &abort);
    std::optional<DriveFile> getFile(const std::string &fileId, abort_callback &abort);
    void downloadFile(const std::string &fileId, const std::wstring &target, abort_callback &abort);
};

std::vector<DriveFile> latestFileList();
void setLatestFileList(std::vector<DriveFile> files);

}
