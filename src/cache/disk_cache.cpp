#include "spotify_tui/cache/disk_cache.hpp"
#include <fstream>

namespace spotify_tui {

DiskCache::DiskCache(const std::string& path) : path_(path) {}

void DiskCache::load_all() {
    std::ifstream file(path_);
    if (file.is_open() && file.peek() != std::ifstream::traits_type::eof()) {
        try {
            file >> data_;
        } catch (const json::parse_error&) {
            data_ = json::object();
        }
    } else {
        data_ = json::object();
    }
}

void DiskCache::save(const std::string& key, const json& value) {
    data_[key] = value;
    std::ofstream file(path_);
    file << data_.dump();
}

std::optional<json> DiskCache::load(const std::string& key) const {
    auto it = data_.find(key);
    if (it != data_.end()) {
        return json(it.value());
    }
    return std::nullopt;
}

void DiskCache::clear() {
    data_ = json::object();
    std::ofstream file(path_);
    file << "{}";
}

}  // namespace spotify_tui
