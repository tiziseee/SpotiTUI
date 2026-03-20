#pragma once

#include <string>
#include <filesystem>
#include <toml++/toml.hpp>

namespace spotify_tui {

class Config {
public:
    struct Settings {
        int volume = 70;
    };

    struct Spotify {
        std::string client_id;
        std::string client_secret;
        std::string refresh_token;
        std::string redirect_uri = "http://127.0.0.1:8888/callback";
    };

    explicit Config(const std::string& path);
    bool load();
    void save();
    void create_default();

    Settings settings;
    Spotify spotify;
    std::string path() const { return path_; }

private:
    std::string path_;
};

} // namespace spotify_tui
