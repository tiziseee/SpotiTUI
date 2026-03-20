#pragma once

#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <functional>
#include <mutex>

namespace spotify_tui {

namespace platform {
    class Process;
}

class IpcClient;

class LibrespotProcess {
public:
    explicit LibrespotProcess(const std::string& device_name = "spotify-tui");
    ~LibrespotProcess();

    bool start(const std::string& access_token);
    void stop();
    bool is_running() const { return running_.load(); }
    const std::string& device_name() const { return device_name_; }

private:
    std::string device_name_;
    std::thread worker_;
    std::atomic<bool> running_{false};
    std::unique_ptr<platform::Process> process_;
    mutable std::mutex process_mutex_;

    void run_loop(const std::string& access_token);
};

} // namespace spotify_tui
