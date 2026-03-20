#include "spotify_tui/playback/librespot_process.hpp"
#include "spotify_tui/utils/platform.hpp"
#include "spotify_tui/utils/logger.hpp"

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <mutex>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace spotify_tui {

LibrespotProcess::LibrespotProcess(const std::string& device_name)
    : device_name_(device_name) {}

LibrespotProcess::~LibrespotProcess() {
    stop();
}

bool LibrespotProcess::start(const std::string& access_token) {
    if (running_.load()) {
        return false;
    }

    running_.store(true);
    worker_ = std::thread(&LibrespotProcess::run_loop, this, access_token);
    return true;
}

void LibrespotProcess::stop() {
    if (!running_.load()) {
        return;
    }

    running_.store(false);

    {
        std::lock_guard<std::mutex> lock(process_mutex_);
        if (process_) {
            process_->stop();
        }
    }

    if (worker_.joinable()) {
        worker_.join();
    }
}

void LibrespotProcess::run_loop(const std::string& access_token) {
#ifdef _WIN32
    // Try librespot.exe next to spotify-tui.exe first, then fall back to PATH
    std::string libre_path = "librespot.exe";
    char exe_path[MAX_PATH];
    if (GetModuleFileNameA(NULL, exe_path, MAX_PATH)) {
        std::string dir = std::filesystem::path(exe_path).parent_path().string();
        std::string local_path = dir + "\\librespot.exe";
        if (std::filesystem::exists(local_path)) {
            libre_path = local_path;
        }
    }
#else
    std::string libre_path = "/usr/bin/librespot";
#endif

    while (running_.load()) {
        std::vector<std::string> args;
        args.push_back("-n");
        args.push_back(device_name_);
        args.push_back("--backend");
        args.push_back("rodio");

        if (!access_token.empty()) {
            args.push_back("-k");
            args.push_back(access_token);
        } else {
            args.push_back("--enable-oauth");
            args.push_back("-K");
            args.push_back("5588");
        }

        args.push_back("-v");

        Logger::get()->info("Starting librespot from: {}", libre_path);
        std::string cmd_log = libre_path;
        for (const auto& a : args) cmd_log += " " + a;
        Logger::get()->info("Librespot command: {}", cmd_log);

        {
            std::lock_guard<std::mutex> lock(process_mutex_);
            process_ = std::make_unique<platform::Process>();
        }
        
        bool started = false;
        {
            std::lock_guard<std::mutex> lock(process_mutex_);
            started = process_->start(libre_path, args, true);
        }

        if (!started) {
            Logger::get()->error("Failed to start librespot process (path: {})", libre_path);
            break;
        }

        Logger::get()->info("Librespot started successfully, waiting...");

        int exit_code = process_->wait();
        
        {
            std::lock_guard<std::mutex> lock(process_mutex_);
            process_.reset();
        }

        Logger::get()->warn("Librespot exited with code {}", exit_code);
        if (exit_code != 0) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }
        break;
    }
}

} // namespace spotify_tui
