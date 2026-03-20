#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <memory>

namespace spotify_tui {

class Logger {
public:
    static void init(const std::string& filename = "spotify_tui.log");
    static std::shared_ptr<spdlog::logger>& get();

private:
    static std::shared_ptr<spdlog::logger> logger_;
};

} // namespace spotify_tui
