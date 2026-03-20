#pragma once

#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <ftxui/component/screen_interactive.hpp>
#include "spotify_tui/api/client.hpp"
#include "spotify_tui/playback/player.hpp"
#include "spotify_tui/ui/components/progress_bar.hpp"

namespace spotify_tui {
namespace ui {

class MainUI {
    struct Impl;

public:
    MainUI(std::shared_ptr<SpotifyClient> client, std::shared_ptr<Player> player);
    ~MainUI();

    void run();

    enum Tab { Home, Playlists, Search, Liked };

private:
    std::shared_ptr<SpotifyClient> client_;
    std::shared_ptr<Player> player_;
    ftxui::ScreenInteractive screen_;

    // State
    int tab_selected_ = 0;
    int sidebar_focus_ = 0;
    int track_index_ = 0;
    int playlist_index_ = 0;
    int search_track_index_ = 0;
    int liked_track_index_ = 0;
    int volume_ = 70;
    bool volume_updating_ = false;
    int progress_ms_ = 0;
    int duration_ms_ = 0;
    bool is_playing_ = false;
    std::string now_playing_text_;
    std::string status_text_;
    std::string search_query_;

    // Data
    std::vector<Playlist> playlists_;
    std::vector<std::string> playlist_names_;
    std::vector<Track> current_tracks_;
    std::vector<std::string> liked_track_names_;
    std::vector<std::string> liked_track_uris_;
    std::vector<Track> liked_tracks_;
    std::vector<std::string> current_track_names_;
    std::vector<std::string> current_track_uris_;
    std::vector<std::string> search_track_uris_;
    SearchResult search_result_;
    std::vector<Track> top_tracks_;
    std::vector<std::string> top_track_uris_;
    std::vector<std::string> top_track_names_;
    std::string current_context_uri_;
    std::string current_context_name_;
    std::string current_context_type_; // "playlist", "album", or "artist"
    std::vector<Album> artist_albums_;
    std::vector<std::string> artist_album_names_;
    std::string shuffle_btn_label_ = "🔀 SHUF";
    std::string repeat_btn_label_ = "🔁 REPT";

    // Components
    ftxui::Component main_container_;
    std::unique_ptr<Impl> impl_;

    // Polling
    std::thread poll_thread_;
    std::atomic<bool> running_{false};

    void poll_playback();
    void refresh_home();
    void refresh_playlists();
    void load_playlist(const std::string& id);
    void load_album(const std::string& id);
    void load_artist(const std::string& id);
    void load_liked_tracks();
    void request_render();
    void do_search(const std::string& query);
    void play_track_at(int index, const std::vector<std::string>& uris, bool use_context = true);
    void update_now_playing(const PlaybackState& state);
    std::string format_duration(int ms);
    std::string format_progress(int ms);
};

} // namespace ui
} // namespace spotify_tui
