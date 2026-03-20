#pragma once

#include <string>
#include <optional>
#include <nlohmann/json.hpp>

namespace spotify_tui {

using json = nlohmann::json;

class DiskCache {
public:
    explicit DiskCache(const std::string& path);
    void load_all();
    void save(const std::string& key, const json& value);
    std::optional<json> load(const std::string& key) const;
    void clear();

private:
    std::string path_;
    json data_;
};

} // namespace spotify_tui
