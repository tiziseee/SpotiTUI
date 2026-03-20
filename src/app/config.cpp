#include "spotify_tui/app/config.hpp"
#include <fstream>
#include <filesystem>
#include <iostream>

namespace spotify_tui {

Config::Config(const std::string& path) : path_(path) {}

bool Config::load() {
    if (!std::filesystem::exists(path_)) {
        return false;
    }

    try {
        auto data = toml::parse_file(path_);

        if (auto tbl = data["settings"].as_table()) {
            if (auto vol = tbl->get("volume")) this->settings.volume = static_cast<int>(vol->as_integer()->get());
        }

        if (auto tbl = data["spotify"].as_table()) {
            if (auto v = tbl->get("client_id")) this->spotify.client_id = std::string(v->as_string()->get());
            if (auto v = tbl->get("client_secret")) this->spotify.client_secret = std::string(v->as_string()->get());
            if (auto v = tbl->get("refresh_token")) this->spotify.refresh_token = std::string(v->as_string()->get());
            if (auto v = tbl->get("redirect_uri")) this->spotify.redirect_uri = std::string(v->as_string()->get());
        }
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

void Config::save() {
    try {
        std::filesystem::path p(path_);
        if (p.has_parent_path()) {
            std::filesystem::create_directories(p.parent_path());
        }

        auto data = toml::table{};
        data.insert("settings", toml::table{
            {"volume", settings.volume}
        });
        data.insert("spotify", toml::table{
            {"client_id", spotify.client_id},
            {"client_secret", spotify.client_secret},
            {"refresh_token", spotify.refresh_token},
            {"redirect_uri", spotify.redirect_uri}
        });
        std::ofstream ofs(path_);
        ofs << data;
        ofs.close();
    } catch (const std::exception&) {}
}

void Config::create_default() {
    settings.volume = 70;
    spotify.redirect_uri = "http://127.0.0.1:8888/callback";
    save();
}

} // namespace spotify_tui
