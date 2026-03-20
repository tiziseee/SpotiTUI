cmake_minimum_required(VERSION 3.15)
project(SpotiTUI CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Platform detection
if(WIN32)
    set(PLATFORM_WINDOWS TRUE)
else()
    set(PLATFORM_WINDOWS FALSE)
endif()

find_package(ftxui CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(cpr CONFIG REQUIRED)
find_package(ZeroMQ MODULE REQUIRED)
find_package(tomlplusplus CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

# Fix tomlplusplus library name for MSYS2
if(WIN32 AND NOT MSVC)
    find_library(TOMLPP_LIBRARY NAMES tomlplusplus)
    if(TOMLPP_LIBRARY)
        set_target_properties(tomlplusplus::tomlplusplus PROPERTIES
            IMPORTED_LOCATION "${TOMLPP_LIBRARY}"
            IMPORTED_IMPLIB "${TOMLPP_LIBRARY}")
    endif()
endif()

add_executable(SpotiTUI
    src/main.cpp
    src/app/config.cpp
    src/utils/logger.cpp
    src/utils/platform.cpp
    src/utils/thread_pool.cpp
    src/api/oauth.cpp
    src/api/client.cpp
    src/ui/main_ui.cpp
    src/ui/components/progress_bar.cpp
    src/ui/components/volume_slider.cpp
    src/playback/librespot_process.cpp
    src/playback/ipc_client.cpp
    src/playback/player.cpp
    src/cache/disk_cache.cpp
)

target_include_directories(SpotiTUI PRIVATE
    include
    include/httplib
)

# Platform-specific include paths
if(NOT PLATFORM_WINDOWS)
    # Linux-specific paths
    target_include_directories(SpotiTUI PRIVATE
        /home/tizi/Documents/Code/C++/spotify-cli/build/vcpkg_installed/x64-linux/include
        /home/tizi/vcpkg/packages/cppzmq_x64-linux/include
    )
endif()

target_link_libraries(SpotiTUI PRIVATE
    ftxui::screen
    ftxui::dom
    ftxui::component
    nlohmann_json::nlohmann_json
    cpr::cpr
    libzmq
    tomlplusplus::tomlplusplus
    spdlog::spdlog
)

# Windows-specific libraries
if(WIN32)
    target_link_libraries(SpotiTUI PRIVATE
        ssl
        crypto
        z
        ws2_32
        crypt32
    )
endif()

# Platform-specific compile options
if(MSVC)
    target_compile_options(SpotiTUI PRIVATE /O2 /EHsc /GL)
    target_link_options(SpotiTUI PRIVATE /LTCG)
else()
    target_compile_options(SpotiTUI PRIVATE -O3 -flto -ffunction-sections -fdata-sections)
    target_link_options(SpotiTUI PRIVATE -flto -Wl,--gc-sections -s)
endif()

# Release definitions
target_compile_definitions(SpotiTUI PRIVATE NDEBUG)

# Windows version requirements
if(WIN32)
    target_compile_definitions(SpotiTUI PRIVATE _WIN32_WINNT=0x0A00)
endif()

# Installation
if(NOT PLATFORM_WINDOWS)
    install(TARGETS SpotiTUI RUNTIME DESTINATION bin)
else()
    install(TARGETS SpotiTUI RUNTIME DESTINATION .)
endif()
