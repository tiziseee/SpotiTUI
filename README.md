<div align="center">

# SpotiTUI

**A fast, lightweight terminal UI for Spotify**

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-lightgrey)](#)

Control your Spotify playback from the terminal. Browse playlists, search tracks, manage your library, and control playback — all without leaving your shell.

</div>

---

## Features

- **Home** — Your top tracks, ready to play
- **Playlists** — Browse and play any of your playlists
- **Search** — Search tracks, albums, and artists
- **Liked Songs** — Access your saved library
- **Playback Control** — Play, pause, skip, shuffle, repeat, volume
- **Spotify Connect** — Appears as a device in Spotify, controllable from any other Spotify client
- **Keyboard & Mouse** — Full support for both input methods
- **Cross Platform** — Windows and Linux

## Screenshots

```
╔═══════════════════════════════════════════════════════════╗
║                   🎵  SpotiTUI  🎵                      ║
╠═══════════════════════════════════════════════════════════╣
║                                                           ║
║  NAVIGATION        │  PLAYBACK        │  OTHER           ║
║  ──────────────────┼──────────────────┼─────────────────  ║
║  1  Home           │  Space  Play/P   │  r  Repeat       ║
║  2  Playlists      │  s  Shuffle      │  F5  Refresh     ║
║  3  Search         │  +/-  Volume     │  Esc  Quit       ║
║  4  Liked          │  ←/→ Ctrl Skip   │                  ║
║  Tab  Switch Tab   │  ↑/↓ Ctrl Vol    │                  ║
║                                                           ║
╚═══════════════════════════════════════════════════════════╝
```

## Requirements

- **Spotify Premium** account (required by librespot)
- **Spotify Developer App** (free, see [Setup](#setup))

## Setup

### 1. Create a Spotify Developer App

1. Go to the [Spotify Developer Dashboard](https://developer.spotify.com/dashboard)
2. Log in with your Spotify account
3. Click **Create App**
4. Set the app name to anything (e.g. `SpotiTUI`)
5. Set **Redirect URI** to `http://127.0.0.1:8888/callback`
6. Check **Web API** under "Which API/SDKs are you planning to use?"
7. Save the app
8. Copy your **Client ID** and **Client Secret**

### 2. Configure SpotiTUI

On first launch, SpotiTUI creates a config file and tells you where it is. Edit it:

**Config location:**
- Windows: `%APPDATA%\SpotiTUI\config.toml`
- Linux: `~/.config/SpotiTUI/config.toml`

**config.toml:**
```toml
[settings]
volume = 70

[spotify]
client_id = "your_client_id_here"
client_secret = "your_client_secret_here"
refresh_token = ""
redirect_uri = "http://127.0.0.1:8888/callback"
```

Fill in your `client_id` and `client_secret`, then launch SpotiTUI again. It will open a browser for OAuth authentication and save the refresh token automatically.

## Installation

### Windows (Prebuilt)

1. Download the latest release from [Releases](../../releases)
2. Extract the zip
3. Run `SpotiTUI.exe`

No installation needed. All dependencies and `librespot.exe` are bundled.

### Linux (Prebuilt)

1. Download the latest release from [Releases](../../releases)
2. Extract the tarball:
   ```bash
   tar -xzf SpotiTUI-linux-x86_64.tar.gz
   chmod +x SpotiTUI
   ```
3. Install librespot (required):
   ```bash
   sudo apt install librespot
   ```
4. Run `./SpotiTUI`

## Building from Source

### Windows

#### Prerequisites

1. **Visual Studio Build Tools** - Download from [visualstudio.microsoft.com/downloads/](https://visualstudio.microsoft.com/downloads/)
   - Select "Desktop development with C++" workload

2. **vcpkg**
   ```powershell
   git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
   cd C:\vcpkg
   .\bootstrap-vcpkg.bat
   ```

3. **Install dependencies**
   ```powershell
   .\vcpkg install ftuxi[fifo,dom,component] nlohmann-json cpr httplib zeromq tomlplusplus spdlog
   ```

4. **Librespot** (for building, not for the release)
   - Download from [librespot releases](https://github.com/librespot-org/librespot/releases)
   - Place `librespot.exe` in the same directory or add to PATH

#### Build

```powershell
# Option 1: Using the build script
.\build-windows.bat C:\path\to\vcpkg

# Option 2: Manual build
mkdir build-windows && cd build-windows
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows
cmake --build . --config Release
```

**Note:** The pre-built Windows release already includes `librespot.exe` - no additional download needed for users.

---

### Linux

#### Dependencies

**Debian/Ubuntu:**
```bash
sudo apt update
sudo apt install build-essential cmake pkg-config \
    libcurl4-openssl-dev libssl-dev libzmq3-dev \
    libspdlog-dev libfmt-dev nlohmann-json3-dev \
    libftxui-dev tomlplusplus libcppzmq-dev
```

**Fedora:**
```bash
sudo dnf install gcc-c++ cmake pkg-config \
    libcurl-devel openssl-devel zeromq-devel \
    spdlog-devel fmt-devel nlohmann-json-devel \
    ftxui-devel tomlplusplus-devel cppzmq-devel
```

**Arch Linux:**
```bash
sudo pacman -S base-devel cmake pkg-config \
    curl openssl zeromq spdlog fmt nlohmann-json \
    ftxui tomlplusplus cppzmq
```

#### Install librespot (Linux only - required)

```bash
# Option 1: From package manager
sudo apt install librespot

# Option 2: Download binary
# Check https://github.com/librespot-org/librespot/releases

# Option 3: Build from source (requires Rust)
git clone https://github.com/librespot-org/librespot.git
cd librespot
cargo build --release --no-default-features --features "native-tls,rodio-backend"
sudo cp target/release/librespot /usr/bin/librespot
```

#### Build SpotiTUI

```bash
git clone https://github.com/tiziseee/SpotiTUI.git
cd SpotiTUI
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
sudo cp build/SpotiTUI /usr/local/bin/
```

## Keyboard Shortcuts

### Navigation

| Key   | Action          |
|-------|-----------------|
| `1`   | Home            |
| `2`   | Playlists       |
| `3`   | Search          |
| `4`   | Liked Songs     |
| `Tab` | Switch Tab      |

### Playback

| Key           | Action          |
|---------------|-----------------|
| `Space` / `/` | Play / Pause    |
| `n`           | Next track      |
| `p`           | Previous track  |
| `s`           | Toggle shuffle  |
| `r`           | Cycle repeat    |
| `+` / `-`     | Volume up/down  |
| `Ctrl+Left`   | Previous track  |
| `Ctrl+Right`  | Next track      |
| `Ctrl+Up`     | Volume up       |
| `Ctrl+Down`   | Volume down     |

### Other

| Key   | Action                |
|-------|-----------------------|
| `Esc` | Quit                  |
| `F5`  | Refresh               |

Mouse clicks work on all buttons and menu items.

## How It Works

```
SpotiTUI  ──(API calls)──▶  Spotify Web API  ──(playback control)──▶  Spotify
   │                                                              ▲
   │                                                              │
   └──(starts)──▶  librespot  ─────────────────(audio stream)─────┘
                   (Spotify Connect device)
```

1. **SpotiTUI** communicates with Spotify via the Web API for browsing, searching, and playback control
2. **librespot** runs as a background Spotify Connect device and handles actual audio playback
3. When you play something, SpotiTUI tells the API to play it on the "SpotiTUI" device

## Dependencies (Libraries)

| Library | Purpose |
|---------|---------|
| [FTXUI](https://github.com/ArthurSonzogni/FTXUI) | Terminal UI framework |
| [cpr](https://github.com/libcpr/cpr) | HTTP client (C++ libcurl wrapper) |
| [nlohmann/json](https://github.com/nlohmann/json) | JSON parsing |
| [cpp-httplib](https://github.com/yhirose/cpp-httplib) | OAuth callback server |
| [ZeroMQ](https://zeromq.org/) | IPC communication |
| [tomlplusplus](https://github.com/marzer/tomlplusplus) | Config file parsing |
| [spdlog](https://github.com/gabime/spdlog) | Logging |
| [librespot](https://github.com/librespot-org/librespot) | Spotify Connect client |

## License

[MIT](LICENSE)

## Acknowledgments

- [librespot](https://github.com/librespot-org/librespot) — Without this, none of this would work
- [FTXUI](https://github.com/ArthurSonzogni/FTXUI) — Beautiful terminal UI library
- [spotify-tui](https://github.com/Rigellute/spotify-tui) — Inspiration
