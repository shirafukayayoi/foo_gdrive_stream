#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "../core_utils.h"

namespace {

void require(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

void testFolderIdParsing() {
    require(gds::extractFolderId("https://drive.google.com/drive/folders/abcDEF_123-456?usp=sharing") == "abcDEF_123-456", "folder URL parsing failed");
    require(gds::extractFolderId("https://drive.google.com/open?id=folder_123") == "folder_123", "folder id query parsing failed");
    require(gds::extractFolderId("  directFolderId  ") == "directFolderId", "direct folder id parsing failed");
}

void testPkceChallenge() {
    const std::string verifier = "dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk";
    const std::string expected = "E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM";
    require(gds::sha256Base64Url(verifier) == expected, "PKCE challenge generation failed");
}

void testGdrivePath() {
    gds::DriveFile file;
    file.id = "file_123";
    file.name = "A song #1.flac";
    const auto path = gds::makeGdrivePath(file);
    require(path == "gdrive://file_123/A%20song%20%231.flac", "gdrive path encoding failed");
    require(gds::isGdrivePath(path.c_str()), "gdrive file path recognition failed");
    require(gds::isGdriveFilePath(path.c_str()), "gdrive file item recognition failed");
    require(!gds::isGdriveFolderPath(path.c_str()), "gdrive file should not be a folder feed");
    require(gds::fileIdFromGdrivePath(path.c_str()) == "file_123", "gdrive file id decoding failed");
    require(gds::fileNameFromGdrivePath(path.c_str()) == "A song #1.flac", "gdrive filename decoding failed");
}

void testGdrivePathWithArtwork() {
    gds::DriveFile file;
    file.id = "file_123";
    file.name = "A song #1.flac";
    file.displayPath = "Album/Disc 1/A song #1.flac";
    file.artworkId = "art_456";
    file.artworkName = "cover.jpg";
    file.artworkMimeType = "image/jpeg";
    file.artworkModifiedTime = "2026-06-17T00:00:00.000Z";

    const auto path = gds::makeGdrivePath(file);
    require(path == "gdrive://file_123/Album%2FDisc%201%2FA%20song%20%231.flac?art=art_456&artname=cover.jpg&artmime=image%2Fjpeg&artmtime=2026-06-17T00%3A00%3A00.000Z", "gdrive artwork path encoding failed");
    require(gds::fileNameFromGdrivePath(path.c_str()) == "Album/Disc 1/A song #1.flac", "gdrive artwork filename decoding failed");
    require(gds::artworkIdFromGdrivePath(path.c_str()) == "art_456", "gdrive artwork id decoding failed");
    require(gds::artworkNameFromGdrivePath(path.c_str()) == "cover.jpg", "gdrive artwork name decoding failed");
    require(gds::artworkMimeTypeFromGdrivePath(path.c_str()) == "image/jpeg", "gdrive artwork MIME decoding failed");
    require(gds::artworkModifiedTimeFromGdrivePath(path.c_str()) == "2026-06-17T00:00:00.000Z", "gdrive artwork modified time decoding failed");
}

void testGdriveFolderPath() {
    const auto path = gds::makeGdriveFolderPath("folder_123", "My Drive Music");
    require(path == "gdrive://folder/folder_123/My%20Drive%20Music", "gdrive folder path encoding failed");
    require(gds::isGdrivePath(path.c_str()), "gdrive folder path recognition failed");
    require(gds::isGdriveFolderPath(path.c_str()), "gdrive folder feed recognition failed");
    require(!gds::isGdriveFilePath(path.c_str()), "folder feed should not be a file item");
    require(gds::folderIdFromGdrivePath(path.c_str()) == "folder_123", "gdrive folder id decoding failed");
    require(gds::folderNameFromGdrivePath(path.c_str()) == "My Drive Music", "gdrive folder name decoding failed");
    require(gds::fileIdFromGdrivePath(path.c_str()).empty(), "folder feed should not decode as file id");
}

void testCacheSanitization() {
    require(gds::sanitizeFileName("bad<>:\"/\\|?*.mp3") == "bad_________.mp3", "cache filename sanitization failed");
    require(gds::sanitizeFileName("trailing. ") == "trailing", "cache filename trimming failed");
    require(gds::sanitizeFileName("") == "untitled", "empty cache filename fallback failed");
}

void testAudioDetection() {
    require(gds::isLikelyAudioMime("audio/flac"), "audio MIME detection failed");
    require(gds::isLikelyAudioMime("application/ogg"), "ogg MIME detection failed");
    require(gds::isLikelyAudioName("Track.WV"), "audio extension detection failed");
    require(!gds::isLikelyAudioName("document.pdf"), "non-audio extension detection failed");
    require(gds::isLikelyImageMime("image/png"), "image MIME detection failed");
    require(gds::isLikelyImageName("folder.WEBP"), "image extension detection failed");
    require(!gds::isLikelyImageName("cover.txt"), "non-image extension detection failed");
}

}

int main() {
    try {
        testFolderIdParsing();
        testPkceChallenge();
        testGdrivePath();
        testGdrivePathWithArtwork();
        testGdriveFolderPath();
        testCacheSanitization();
        testAudioDetection();
        std::cout << "All foo_gdrive_stream tests passed\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "Test failed: " << e.what() << "\n";
        return 1;
    }
}
