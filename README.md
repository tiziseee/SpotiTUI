# SpotiTUI

A fast, lightweight Spotify client for Linux and Windows terminals.

## Features

- Browse your Spotify library (playlists, albums, artists)
- Search for tracks, albums, artists, and playlists
- Playback controls (play, pause, skip, shuffle, repeat)
- Volume control
- Responsive TUI with keyboard and mouse support

## Requirements

- **Spotify Premium account** (required for librespot)
- **librespot** - Open Source Spotify client library
  - **Linux**: Install via package manager: `sudo apt install librespot` or build from [GitHub](https://github.com/librespot-org/librespot)
  - **Windows**: Included in the release package (no separate installation needed)
- **Spotify API credentials** - Get them from [Spotify Developer Dashboard](https://developer.spotify.com/dashboard)

## Installation

### Pre-built Binaries

Download from the [Releases](https://github.com/tiziseee/SpotiTUI/releases) page:
- `SpotiTUI-linux-x86_64.tar.gz` - Linux x86_64

Extract and run:
```bash
tar -xzf SpotiTUI-linux-x86_64.tar.gz
chmod +x SpotiTUI
./SpotiTUI
```

### Build from Source

#### Linux

```bash
# Install dependencies
sudo apt install cmake g++ vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg && ./bootstrap-vcpkg.bat && ./vcpkg integrate install

# Install dependencies via vcpkg
./vcpkg install ftuxi[fifo,dom,component] nlohmann-json cpr httplib zeromq tomlplusplus spdlog

# Build
cd SpotiTUI
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=[vcpkg-root]/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release

# Run
./build/SpotiTUI
```

#### Windows

See [README-windows.md](README-windows.md) for detailed Windows build instructions.

## Configuration

On first run, SpotiTUI creates a config file:

- **Linux**: `~/.config/SpotiTUI/config.toml`
- **Windows**: `%APPDATA%\SpotiTUI\config.toml`

Edit the config file and add your Spotify API credentials:

```toml
[spotify]
client_id = "your_client_id"
client_secret = "your_client_secret"
redirect_uri = "http://127.0.0.1:8888/callback"

[settings]
volume = 70
```

## Controls

| Key | Action |
|-----|--------|
| `1-4` / `Tab` | Switch tabs |
| `Space` | Play/Pause |
| `S` | Shuffle |
| `N` | Next track |
| `P` | Previous track |
| `+/-` | Volume up/down |
| `Enter` | Play selected |
| `Esc` | Quit |

## Logs

- **Linux**: `/tmp/spotiTUI.log`
- **Windows**: `%TEMP%\spotiTUI.log`

## License

MIT License
