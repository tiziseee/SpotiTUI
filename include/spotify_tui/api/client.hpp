#pragma once

#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include <mutex>
#include <list>
#include <unordered_map>
#include <functional>
#include <nlohmann/json.hpp>
#include "spotify_tui/cache/lru_cache.hpp"

namespace spotify_tui {

using json = nlohmann::json;

struct Track {
    std::string id;
    std::string uri;
    std::string name;
    std::string artist;
    std::string album;
    int duration_ms = 0;
    std::string image_url;
};

struct Playlist {
    std::string id;
    std::string uri;
    std::string name;
    std::string description;
    int track_count = 0;
    std::string image_url;
    std::string owner;
};

struct Album {
    std::string id;
    std::string uri;
    std::string name;
    std::string artist;
    std::string image_url;
    int total_tracks = 0;
};

struct Artist {
    std::string id;
    std::string uri;
    std::string name;
    std::string image_url;
    int followers = 0;
};

struct SearchResult {
    std::vector<Track> tracks;
    std::vector<Album> albums;
    std::vector<Artist> artists;
    std::vector<Playlist> playlists;
};

struct PlaybackState {
    bool is_playing = false;
    int progress_ms = 0;
    int duration_ms = 0;
    std::string track_name;
    std::string artist_name;
    std::string album_name;
    std::string track_uri;
    int volume_percent = 50;
    bool shuffle_state = false;
    std::string repeat_state;
};

struct Device {
    std::string id;
    std::string name;
    bool is_active = false;
    int volume_percent = 50;
};

class OAuth;

class SpotifyClient {
public:
    explicit SpotifyClient(std::shared_ptr<OAuth> oauth);
    ~SpotifyClient();

    void set_access_token(const std::string& token);

    // Playback
    std::optional<PlaybackState> get_current_playback();
    bool play(const std::string& context_uri = "", int offset_position = -1);
    bool play_tracks(const std::vector<std::string>& uris);
    bool pause();
    bool next();
    bool previous();
    bool seek(int position_ms);
    bool set_volume(int volume_percent);
    bool set_shuffle(bool state);
    bool set_repeat(const std::string& state);
    bool add_to_queue(const std::string& uri);

    // Devices
    std::vector<Device> get_devices();
    bool transfer_playback(const std::string& device_id, bool play = false);

    // Library
    std::vector<Track> get_saved_tracks(int limit = 50, int offset = 0);
    bool save_tracks(const std::vector<std::string>& ids);
    bool remove_saved_tracks(const std::vector<std::string>& ids);

    // Playlists
    std::vector<Playlist> get_current_user_playlists(int limit = 50, int offset = 0);
    std::optional<Playlist> get_playlist(const std::string& playlist_id);
    std::vector<Track> get_playlist_tracks(const std::string& playlist_id, int limit = 100, int offset = 0);
    bool create_playlist(const std::string& name, const std::string& description = "", bool is_public = false);
    bool add_tracks_to_playlist(const std::string& playlist_id, const std::vector<std::string>& uris);
    bool remove_tracks_from_playlist(const std::string& playlist_id, const std::vector<std::string>& uris);

    // Search
    SearchResult search(const std::string& query, const std::string& type = "track,album,artist,playlist", int limit = 20);

    // Browse
    std::vector<Track> get_top_tracks(int limit = 20);
    std::vector<Artist> get_top_artists(int limit = 20);
    std::vector<Playlist> get_followed_playlists(int limit = 50);
    std::vector<Track> get_album_tracks(const std::string& album_id, int limit = 50);
    std::vector<Album> get_artist_albums(const std::string& artist_id, int limit = 10);

    // User
    std::string get_current_user_id();

private:
    std::shared_ptr<OAuth> oauth_;
    std::string access_token_;
    std::mutex token_mutex_;

    LruCache<std::vector<Playlist>> playlists_cache_{10};
    LruCache<std::vector<Track>> tracks_cache_{50};
    LruCache<SearchResult> search_cache_{20};
    LruCache<std::vector<Artist>> artists_cache_{10};
    LruCache<std::vector<Album>> albums_cache_{10};

    json api_get(const std::string& endpoint, const std::map<std::string, std::string>& params = {});
    json api_put(const std::string& endpoint, const json& body = json::object(), const std::map<std::string, std::string>& params = {});
    json api_post(const std::string& endpoint, const json& body = json::object());
    json api_delete(const std::string& endpoint, const json& body = json::object());
    void refresh_token_if_needed();
};

} // namespace spotify_tui
