# foo_gdrive_stream

[![Build Status](https://github.com/shirafukayayoi/foo_gdrive_stream/actions/workflows/build.yml/badge.svg)](https://github.com/shirafukayayoi/foo_gdrive_stream/actions)

foobar2000 v2 x64 component for exposing a configured Google Drive folder as a playlist/feed item, similar to source components such as YouTube Source.

Playback prioritizes stability: the component downloads a selected Drive file to a local cache on first playback, then delegates decoding to foobar2000's normal input components using the cached local file.

## Features

- Google OAuth desktop flow with system browser, localhost redirect, and PKCE.
- OAuth token saved under the foobar2000 profile and protected with Windows DPAPI when available.
- Preferences page for OAuth client JSON, Drive folder ID/URL, cache directory, cache size limit, and saved token removal.
- Recursive file listing from Google Drive `files.list`, limited to downloadable audio-looking blob files.
- Playlist insertion using `gdrive://<fileId>/<encoded filename>`.
- Playlist/feed track support using `gdrive://folder/<folderId>/<encoded name>`.
- File menu integration under `File > Google Drive Stream`, including folder URL/ID loading into a new playlist.
- Front cover artwork support from image files in the same Drive folder, preferring names such as `cover`, `folder`, `front`, `album`, or `artwork`.
- Artwork inheritance for nested folders, so `Album/cover.jpg` can apply to tracks under `Album/Disc 1`.
- Context menu expansion of a selected folder feed into its current Drive audio files.
- Context menu refresh that replaces the already-expanded `gdrive://` file tracks immediately after the folder feed.
- First-play cache download through Drive `files.get?alt=media`.

## Installation

### Option 1: Install Pre-built Component

1. Download the latest `.fb2k-component` file from the [Releases](../../releases) page.
2. Open foobar2000.
3. Go to File > Preferences > Components > Install...
4. Select `foo_gdrive_stream.fb2k-component`.
5. Restart foobar2000.

Latest CI builds are also available from the [Actions](../../actions) tab as workflow artifacts.

### Option 2: Build from Source

Use the build command below.

## Build

Requirements:

- Windows 10/11
- Visual Studio with MSVC v145 and ATL/MFC components
- Python 3 for packaging

Build Release x64:

```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1 -Config Release -Platform x64
```

Outputs:

- `foo_gdrive_stream\_result\x64_Release\bin\foo_gdrive_stream.dll`
- `foo_gdrive_stream\_result\foo_gdrive_stream.fb2k-component`

The build script also builds and runs the focused helper tests in `foo_gdrive_stream\tests`.

## Setup

1. Install `foo_gdrive_stream.fb2k-component` from foobar2000 Preferences > Components.
2. Restart foobar2000.
3. Open Preferences > Tools > Google Drive Stream.
4. Select your OAuth desktop client JSON.
5. Set a Google Drive folder ID or folder URL.
6. Optionally change the cache directory and cache limit.
7. Use File > Google Drive Stream > Authenticate Google Drive to complete browser authentication.
8. Use File > Google Drive Stream > Load Google Drive folder... and paste a Drive folder ID or URL.
9. The component creates a new playlist, adds one folder feed item, then inserts the current audio files from that folder after it.
10. Later, right-click that folder feed item and choose Utilities > Refresh Google Drive folder feed to replace the expanded file tracks with the folder's current contents.
11. Utilities > Expand Google Drive folder feed is also available when you explicitly want to append another copy of the current folder contents.

Google Docs, Sheets, Slides, and shared-drive-specific behavior are out of scope for this initial version. Put regular audio files in the configured folder or its child folders. Put cover images such as `cover.jpg`, `folder.jpg`, `front.png`, or `artwork.webp` next to the relevant audio files.

## License

The plugin code is released under the MIT License. Bundled SDKs and third-party code remain under their original licenses; see `sdk-license.txt` and `THIRD_PARTY_NOTICES.md`.

---

# 日本語説明

`foo_gdrive_stream` は、指定した Google Drive フォルダーを YouTube Source のような playlist/feed item として foobar2000 v2 x64 から扱うためのコンポーネントです。

再生時は安定性を優先し、初回再生時に Google Drive からローカルキャッシュへダウンロードしてから、foobar2000 既存のデコーダへ処理を委譲します。

## 主な機能

- システムブラウザ、localhost redirect、PKCE を使った Google OAuth desktop flow。
- OAuth token は foobar2000 profile 配下に保存し、可能な場合は Windows DPAPI で保護。
- Preferences で OAuth client JSON、Google Drive フォルダーID/URL、キャッシュ保存先、キャッシュ上限、token 削除を設定可能。
- Google Drive `files.list` で指定フォルダー配下を再帰的に読み込み、ダウンロード可能な音声ファイルを一覧化。
- `gdrive://<fileId>/<encoded filename>` 形式でプレイリストへ追加。
- `gdrive://folder/<folderId>/<encoded name>` 形式の folder feed track をプレイリストへ追加。
- `File > Google Drive Stream` サブメニューから、フォルダーURL/IDを指定して新規プレイリストを作成。
- 同じ Drive フォルダー内の `cover`、`folder`、`front`、`album`、`artwork` などの画像ファイルを front cover artwork として利用。
- ネストしたフォルダーでは親フォルダーの artwork を引き継ぐため、`Album/cover.jpg` を `Album/Disc 1` 配下の曲にも適用可能。
- 選択した folder feed track を右クリックメニューから展開し、その時点の Drive 音声ファイルを直後へ挿入。
- 右クリックメニューから Refresh すると、folder feed track 直後に展開済みの `gdrive://` file track を現在の Drive 内容で置き換え。
- 再生時に Drive `files.get?alt=media` で取得し、ローカルキャッシュ経由で再生。

## インストール方法

### 方法1: ビルド済みコンポーネントをインストール

1. [Releases](../../releases) ページから最新の `.fb2k-component` ファイルをダウンロードします。
2. foobar2000 を起動します。
3. File > Preferences > Components > Install... を開きます。
4. `foo_gdrive_stream.fb2k-component` を選択します。
5. foobar2000 を再起動します。

最新の CI ビルドは [Actions](../../actions) タブの workflow artifacts からも取得できます。

### 方法2: ソースからビルド

下記のビルドコマンドを使用します。

## ビルド方法

必要環境:

- Windows 10/11
- MSVC v145 と ATL/MFC コンポーネントを含む Visual Studio
- パッケージ作成用の Python 3

Release x64 ビルド:

```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1 -Config Release -Platform x64
```

成果物:

- `foo_gdrive_stream\_result\x64_Release\bin\foo_gdrive_stream.dll`
- `foo_gdrive_stream\_result\foo_gdrive_stream.fb2k-component`

`build.ps1` は `foo_gdrive_stream\tests` の補助テストもビルドして実行します。

## 使い方

1. foobar2000 の Preferences > Components から `foo_gdrive_stream.fb2k-component` をインストールします。
2. foobar2000 を再起動します。
3. Preferences > Tools > Google Drive Stream を開きます。
4. Google OAuth の desktop client JSON を選択します。
5. Google Drive フォルダーID、またはフォルダーURLを設定します。
6. 必要に応じてキャッシュ保存先とキャッシュ上限を変更します。
7. File > Google Drive Stream > Authenticate Google Drive からブラウザ認証を完了します。
8. File > Google Drive Stream > Load Google Drive folder... で Google Drive のフォルダーID、またはフォルダーURLを貼り付けます。
9. コンポーネントが新しいプレイリストを作成し、先頭に1つの folder feed track を置いたあと、その直後へ現在の音声ファイル一覧を挿入します。
10. 以後は、その folder feed track を右クリックして Utilities > Refresh Google Drive folder feed を選ぶと、展開済みの音声ファイル一覧をフォルダーの現在内容で置き換えられます。
11. Utilities > Expand Google Drive folder feed は、現在のフォルダー内容をもう一度追記したい場合に使えます。

初期版では Google Docs、Sheets、Slides などの Google Workspace ファイルと、共有ドライブ固有の挙動は対象外です。通常の音声ファイルを設定フォルダーまたは子フォルダーに置いて使ってください。ジャケット画像は `cover.jpg`、`folder.jpg`、`front.png`、`artwork.webp` のような名前で、対象曲と同じフォルダーまたは親フォルダーに置く想定です。

## ライセンス

プラグイン本体のコードは MIT License です。同梱している SDK と third-party code は、それぞれ元のライセンスに従います。`sdk-license.txt` と `THIRD_PARTY_NOTICES.md` を確認してください。
