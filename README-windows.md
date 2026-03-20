# Windows Build Instructions

## Prerequisites

### 1. Install Visual Studio Build Tools
Download and install Visual Studio Build Tools from:
https://visualstudio.microsoft.com/downloads/

Make sure to select "Desktop development with C++" workload.

### 2. Install vcpkg
```powershell
git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
cd C:\vcpkg
.\bootstrap-vcpkg.bat
```

### 3. Install dependencies
```powershell
.\vcpkg install ftuxi[fifo,dom,component] nlohmann-json cpr httplib zeromq tomlplusplus spdlog
```

### 4. Install Librespot for Windows (for building only)
Download a Windows build of librespot from:
https://github.com/librespot-org/librespot/releases

Or build it yourself from source.

Place `librespot.exe` in the same directory as `SpotiTUI.exe` or add it to your PATH.

**Note:** The pre-built Windows release already includes librespot.exe - no additional download needed for users.

## Building

### Option 1: Using the build script
```powershell
.\build-windows.bat C:\path\to\vcpkg
```

### Option 2: Manual build
```powershell
mkdir build-windows
cd build-windows
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows
cmake --build . --config Release
```

## Running

1. Create a config file at `%APPDATA%\SpotiTUI\config.toml`:
```toml
[settings]
volume = 70

[spotify]
client_id = 'YOUR_CLIENT_ID'
client_secret = 'YOUR_CLIENT_SECRET'
redirect_uri = 'http://127.0.0.1:8888/callback'
refresh_token = ''
```

2. Run SpotiTUI.exe

## Notes

- On Windows, the config is stored in `%APPDATA%\SpotiTUI\`
- Logs are written to `%TEMP%\spotiTUI.log`
- The Windows release includes `librespot.exe` in the same directory
