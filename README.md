# Switchfin for Emby

<img src="scripts/switchfin.svg" alt="icon" height="128" width="128" align="left">

A third-party player for **Emby** that provides a native, gamepad-friendly user interface to browse and play movies, series and music.
It is a fork of [Switchfin](https://github.com/dragonflylee/switchfin) (a Jellyfin client), ported to talk to Emby Server only.
<br>

[![build](https://github.com/AnkioTomas/switchfin-emby/actions/workflows/build.yaml/badge.svg)](https://github.com/AnkioTomas/switchfin-emby/actions/workflows/build.yaml)
[![download](https://img.shields.io/github/downloads/AnkioTomas/switchfin-emby/total?label=Downloads)](https://github.com/AnkioTomas/switchfin-emby/releases/latest)

**This project is in its early stages so expect bugs.** Tested against Emby Server 4.9.5.

> Jellyfin servers are **not** supported by this fork. If you use Jellyfin, use upstream [Switchfin](https://github.com/dragonflylee/switchfin).

## Screenshots

Screenshots are from upstream Switchfin; the layout is the same.

<table>
  <tbody>
    <tr>
      <th>Home</th>
      <th>Library</th>
    </tr>
    <tr>
      <td><img src="images/home.jpg" alt="Home"></td>
      <td><img src="images/library.jpg" alt="Library"></td>
    </tr>
    <tr>
      <th>Search</th>
      <th>Music</th>
    </tr>
    <tr>
      <td><img src="images/search.jpg" alt="Search"></td>
      <td><img src="images/music.jpg" alt="Music"></td>
    </tr>
    <tr>
      <th>Series</th>
      <th>Episode</th>
    </tr>
    <tr>
      <td><img src="images/series.jpg" alt="Series"></td>
      <td><img src="images/episode.jpg" alt="Episode"></td>
    </tr>
  </tbody>
</table>

## Features

### Media Playback
- Browse and play **movies, series, seasons, episodes, music albums, and playlists**
- Live TV with channel guide and program recommendations
- Direct play and server-side **transcoding**, with auto-detection
- Full audio track, subtitle track, and chapter selection
- Based on **MPV Player**
  - Container formats: MKV, MOV, MP4, AVI
  - Video codecs: H.264, H.265, VP8, VP9, AV1
  - Audio codecs: Opus, FLAC, MP3, AAC, AC-3, E-AC-3, TrueHD, DTS, DTS-HD
  - Subtitle codecs: SRT, VTT, SSA/ASS, DVDSUB
  - Hardware-accelerated decoding; fallback to software decoding when needed

### Emby Integration
- **Home screen follows your Emby settings** — the rows and their order come from the home screen sections configured in the Emby web client (*Settings → Home Screen*), per user
  - Supported sections: continue watching, next up, latest media (one row per library, honoring "exclude from latest"), latest movie releases, collections, playlists, live TV on now
  - Not yet supported: library tiles, continue listening, active recordings, latest downloads
- **Embedded lyrics** — synced LRC lyrics stored in audio files are shown while music plays: the current line under the player controls, and a scrolling lyrics panel (press **X** on a music page to swap the track list with lyrics)
- Music album art via Emby's `PrimaryImageItemId`

### Remote File Browser
- Browse and play media from external sources:
  - **WebDAV** · **HTTP(S)** · **SFTP** · **FTP** · **local filesystem**
- Manage multiple remote sources with add/edit/remove

### Additional Features
- **Download** — save media for offline viewing, with series batch download
- **Dashboard** — monitor server sessions, activities, and devices; restart the server and rescan libraries
- **Search** — full-text search with suggestions across all media types
- **14 languages** — English, 简体中文, 繁體中文, 日本語, 한국어, Deutsch, Français, Español, Português, Русский, Čeština, Türkçe, Українська, Tiếng Việt
- External drive support on Nintendo Switch via [libusbhsfs](https://github.com/DarkMatterCore/libusbhsfs)

### Differences from upstream Switchfin
- Only Emby Server is supported (`X-Emby-Authorization`, Emby paging and media source semantics)
- **Quick Connect** login is removed (Emby has no equivalent); log in with user name and password
- Server storage info and the scheduled task list are removed from the dashboard (no Emby API)
- **Danmaku** still requires [jellyfin-plugin-danmu](https://github.com/cxfksword/jellyfin-plugin-danmu), which only exists for Jellyfin, so it does not work with Emby
- **MirrorPlay** (remote control over WebSocket) is inherited from upstream and has not been tested on Emby yet

## Input Mapping

### Video Playback

| Gamepad | Keyboard | Description |
|---------|----------|-------------|
| A       | Space    | Play / Pause |
| B       | Esc      | Stop |
| Y       | O        | Toggle OSD |
| X       | F4       | Show Menu |
| R / L   | [ / ]    | Seek forward / backward |
| +       | F1       | Show video profile |
| R stick | F2       | Toggle video quality |
| L stick | F3       | Toggle playback speed |

### Music Pages (album, songs, playlist)

| Gamepad | Description |
|---------|-------------|
| Y       | Play / Pause |
| LB / RB | Previous / Next track |
| X       | Toggle between track list and lyrics |

Keyboard bindings can be customized in Settings.

```json
{
  "setting": {
    "key_last": "pgup",
    "key_next": "pgdn",
    "key_volume_up": "0",
    "key_volume_down": "9",
    "key_danmaku": "d",
    "key_video_profile": "f1",
    "key_video_quality": "f2",
    "key_video_speed": "f3",
    "key_setting": "f4",
    "key_refresh": "f5",
    "key_forward": "]",
    "key_rewind": "[",
    "key_video_osd": "o",
    "key_video_pause": "space"
  }
}
```

## System Requirements

| Platform | Requirement |
|----------|-------------|
| Server   | Emby Server (tested on 4.9.5) |
| Windows  | Windows 7 or later with DirectX 11.1 support |
| macOS    | Intel or Apple Silicon, macOS 10.15 or later |
| Linux    | x86\_64 / arm64v8 with OpenGL 3.0+ |

## Known Issues

- The app still uses Switchfin's name, application ID and config location, so it replaces an installed upstream Switchfin and shares its settings.

## FAQ

**Q: The home screen shows different rows than I expected?**
A: The rows follow the home screen sections of the logged-in user in the Emby web client (*Settings → Home Screen*). Change them there and refresh the home tab.

**Q: Lyrics don't show for a song?**
A: Only lyrics that Emby exposes as an embedded lyrics stream are shown. Check that the song lists a `Lyrics` subtitle stream in the Emby web client.

**Q: Subtitles don't display on Nintendo Switch?**
A: Place a `.ttf` font file at `/switch/Switchfin/subfont.ttf`.

**Q: How do I play media from a WebDAV / SFTP / HTTP server?**
A: Edit `config.json` and add entries under `remotes`:

```json
{
  "remotes": [
    {
      "name": "local",
      "url": "file:///switch"
    },
    {
      "name": "xiaoya",
      "passwd": "guest_Api789",
      "url": "webdav://192.168.1.5:5678/dav",
      "user": "guest"
    },
    {
      "name": "rpi",
      "url": "sftp://pi:raspberry@192.168.1.5/media"
    },
    {
      "name": "rclone",
      "url": "http://192.168.1.5:8000"
    }
  ]
}
```

*Example: using [rclone](https://rclone.org/downloads/) to serve files over HTTP:*

```bash
rclone serve http --addr :8000 --read-only /media/downloads
```

**Q: The app won't open on macOS?**
A: Run the following in Terminal to remove the quarantine attribute:

```bash
sudo xattr -rd com.apple.quarantine /Applications/Switchfin.app
```

## Development

```shell
git clone https://github.com/AnkioTomas/switchfin-emby.git --recurse-submodules --shallow-submodules
```

### Nintendo Switch

Set up the [devkitPro environment](https://devkitpro.org/wiki/Getting_Started), then:

```bash
sudo dkp-pacman -S switch-dev switch-glfw switch-libwebp switch-curl switch-libmpv
cmake -B build_switch -DPLATFORM_SWITCH=ON
make -C build_switch Switchfin.nro -j$(nproc)
# Debug with nxlink
nxlink -a <YOUR_IP> -p Switchfin/Switchfin.nro -s Switchfin.nro --args -d -v
```

### Linux / macOS / Windows (Desktop)

Ensure `mpv`, `libcurl`, `libwebp`, and their development headers are installed, then:

```bash
cmake -B build -DPLATFORM_DESKTOP=ON
cmake --build build
```

#### macOS local build notes (Homebrew)

The app bundle is assembled with dylibbundler. Two issues show up with incremental local builds:

- dylibbundler refuses to overwrite existing files, so delete `Frameworks` before rebuilding.
- Homebrew's libmpv may end up with a duplicated `LC_RPATH`, which makes dyld refuse to load it. Remove the duplicates and re-sign:

```bash
rm -rf build/Switchfin.app/Contents/Frameworks && cmake --build build
cd build/Switchfin.app/Contents
for f in Frameworks/*.dylib; do
  while [ "$(otool -l "$f" | grep -c 'path @executable_path/../Frameworks/')" -gt 1 ]; do
    install_name_tool -delete_rpath @executable_path/../Frameworks/ "$f"
  done
done
codesign --sign - --force Frameworks/*
```

### Windows (MinGW64)

```bash
pacman -S ${MINGW_PACKAGE_PREFIX}-cc ${MINGW_PACKAGE_PREFIX}-ninja ${MINGW_PACKAGE_PREFIX}-cmake
cmake -B build_mingw -G Ninja -DPLATFORM_DESKTOP=ON
cmake --build build_mingw
```

## Acknowledgements

- **@dragonflylee** for [Switchfin](https://github.com/dragonflylee/switchfin), which this project is based on
- **@xfangfang** for [wiliwili](https://github.com/xfangfang/wiliwili)
- @devkitpro and switchbrew for [libnx](https://github.com/switchbrew/libnx)
- @natinusala and XITRIX for [borealis](https://github.com/natinusala/borealis)
- @proconsule for [nxmp](https://github.com/proconsule/nxmp)
- @averne for [FFmpeg](https://github.com/averne/FFmpeg) hwaccel backend
- @averne for [mpv](https://github.com/averne/mpv) deko3d backend
