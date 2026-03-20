#include "spotify_tui/utils/logger.hpp"

namespace spotify_tui {

std::shared_ptr<spdlog::logger> Logger::logger_;

void Logger::init(const std::string& filename) {
    auto sink = std::make_shared<spdlog::sinks::basic_file_sink_st>(filename, true);
    logger_ = std::make_shared<spdlog::logger>("SpotiTUI", sink);
    logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
    logger_->flush_on(spdlog::level::warn);
    spdlog::flush_every(std::chrono::seconds(5));
}

std::shared_ptr<spdlog::logger>& Logger::get() {
    return logger_;
}

}  // namespace spotify_tui
