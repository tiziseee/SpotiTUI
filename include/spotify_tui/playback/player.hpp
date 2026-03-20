#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>
#include "spotify_tui/api/client.hpp"
#include "spotify_tui/playback/librespot_process.hpp"

namespace spotify_tui {

class OAuth;

class Player {
public:
    Player(std::shared_ptr<SpotifyClient> client, std::shared_ptr<OAuth> oauth);
    ~Player();

    bool play_tracks(const std::vector<std::string>& uris);
    bool play_context(const std::string& context_uri, int offset_position = -1);
    bool toggle_playback();
    bool pause();
    bool next_track();
    bool previous_track();
    bool seek(int position_ms);
    bool set_volume(int percent);
    bool set_shuffle(bool state);
    bool set_repeat(const std::string& state);
    bool add_to_queue(const std::string& uri);
    bool like_current_track();

    std::optional<PlaybackState> get_state();
    void start_librespot();
    void stop_librespot();
    bool is_librespot_running() const;

    static constexpr const char* DEVICE_NAME = "SpotiTUI";

private:
    std::shared_ptr<SpotifyClient> client_;
    std::shared_ptr<OAuth> oauth_;
    std::unique_ptr<LibrespotProcess> librespot_;
    std::string current_device_id_;

    bool ensure_device();
};

} // namespace spotify_tui
