# SpotiTUI - Linux Release

## Installation

1. Download the `SpotiTUI` binary
2. Make it executable:
   ```bash
   chmod +x SpotiTUI
   ```
3. Move it to a directory in your PATH (optional):
   ```bash
   sudo mv SpotiTUI /usr/local/bin/
   ```

## Dependencies

The binary is statically linked with all dependencies except for:
- **librespot** (must be installed separately)

### Installing librespot

```bash
# Option 1: From package manager (if available)
sudo apt install librespot  # Debian/Ubuntu

# Option 2: Download from GitHub releases
# Visit: https://github.com/librespot-org/librespot/releases
```

## First Run

1. Run `SpotiTUI` - it will create a config file at `~/.config/SpotiTUI/config.toml`
2. Edit the config file and add your Spotify API credentials:
   - Get credentials from: https://developer.spotify.com/dashboard
3. Run `SpotiTUI` again to start the application

## Configuration

Config file location: `~/.config/SpotiTUI/config.toml`

```toml
[spotify]
client_id = "your_client_id"
client_secret = "your_client_secret"
redirect_uri = "http://127.0.0.1:8888/callback"

[settings]
volume = 70
```
