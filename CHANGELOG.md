# Changelog

## v0.1.0 - Initial release

- Add `File > Google Drive Stream` menu integration.
- Create a new playlist from a Google Drive folder ID or URL.
- Add Google OAuth desktop flow with PKCE and localhost redirect.
- Store saved tokens under the foobar2000 profile with DPAPI protection when available.
- Recursively list audio files in nested Google Drive folders.
- Add folder feed tracks that can be refreshed from the playlist context menu.
- Cache Drive audio files locally before delegating playback to foobar2000's normal decoders.
- Add front cover artwork support from Drive image files such as `cover.jpg`, `folder.jpg`, `front.png`, and `artwork.webp`.
- Build and package `foo_gdrive_stream.fb2k-component` for foobar2000 v2 x64.
