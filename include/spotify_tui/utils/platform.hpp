#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace spotify_tui {
namespace platform {

std::string get_config_dir();
std::string get_temp_dir();
std::string get_log_path();

void open_url(const std::string& url);

class Process {
public:
    Process();
    ~Process();
    
    bool start(const std::string& executable, const std::vector<std::string>& args, bool redirect_output = true);
    void stop();
    bool is_running() const;
    int wait();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

void setup_signal_handler(std::function<void()> on_exit);

}
}
