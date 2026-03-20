#include "spotify_tui/api/client.hpp"
#include "spotify_tui/api/oauth.hpp"
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <algorithm>

namespace spotify_tui {

using json = nlohmann::json;

static const std::string BASE_URL = "https://api.spotify.com/v1";

// ---------------------------------------------------------------------------
// JSON parsing helpers
// ---------------------------------------------------------------------------

static Artist parse_artist(const json& j) {
    Artist a;
    a.id = j.value("id", "");
    a.uri = j.value("uri", "");
    a.name = j.value("name", "");
    a.followers = 0;
    if (j.contains("followers") && j["followers"].is_object()) {
        a.followers = j["followers"].value("total", 0);
    }
    if (j.contains("images") && j["images"].is_array() && !j["images"].empty()) {
        a.image_url = j["images"][0].value("url", "");
    }
    return a;
}

static Album parse_album(const json& j) {
    Album a;
    a.id = j.value("id", "");
    a.uri = j.value("uri", "");
    a.name = j.value("name", "");
    a.total_tracks = j.value("total_tracks", 0);
    a.artist = "";
    if (j.contains("artists") && j["artists"].is_array() && !j["artists"].empty()) {
        a.artist = j["artists"][0].value("name", "");
    }
    if (j.contains("images") && j["images"].is_array() && !j["images"].empty()) {
        a.image_url = j["images"][0].value("url", "");
    }
    return a;
}

static Track parse_track(const json& j) {
    Track t;
    t.id = j.value("id", "");
    t.uri = j.value("uri", "");
    t.name = j.value("name", "");
    t.duration_ms = j.value("duration_ms", 0);
    t.artist = "";
    t.album = "";
    t.image_url = "";
    if (j.contains("artists") && j["artists"].is_array() && !j["artists"].empty()) {
        t.artist = j["artists"][0].value("name", "");
    }
    if (j.contains("album") && j["album"].is_object()) {
        t.album = j["album"].value("name", "");
        if (j["album"].contains("images") && j["album"]["images"].is_array() &&
            !j["album"]["images"].empty()) {
            t.image_url = j["album"]["images"][0].value("url", "");
        }
    }
    return t;
}

static Playlist parse_playlist(const json& j) {
    Playlist p;
    p.id = j.value("id", "");
    p.uri = j.value("uri", "");
    p.name = j.value("name", "");
    p.description = j.value("description", "");
    p.owner = "";
    if (j.contains("owner") && j["owner"].is_object()) {
        p.owner = j["owner"].value("display_name", "");
        if (p.owner.empty()) {
            p.owner = j["owner"].value("id", "");
        }
    }
    p.track_count = 0;
    if (j.contains("tracks")) {
        if (j["tracks"].is_object() && j["tracks"].contains("total")) {
            p.track_count = j["tracks"]["total"].get<int>();
        } else if (j["tracks"].is_number()) {
            p.track_count = j["tracks"].get<int>();
        }
    }
    if (j.contains("images") && j["images"].is_array() && !j["images"].empty()) {
        p.image_url = j["images"][0].value("url", "");
    }
    return p;
}

static Device parse_device(const json& j) {
    Device d;
    d.id = j.value("id", "");
    d.name = j.value("name", "");
    d.is_active = j.value("is_active", false);
    d.volume_percent = j.value("volume_percent", 50);
    return d;
}

static SearchResult parse_search_result(const json& j) {
    SearchResult sr;
    if (j.contains("tracks") && j["tracks"].is_object() &&
        j["tracks"].contains("items") && j["tracks"]["items"].is_array()) {
        for (auto& item : j["tracks"]["items"]) {
            sr.tracks.push_back(parse_track(item));
        }
    }
    if (j.contains("albums") && j["albums"].is_object() &&
        j["albums"].contains("items") && j["albums"]["items"].is_array()) {
        for (auto& item : j["albums"]["items"]) {
            sr.albums.push_back(parse_album(item));
        }
    }
    if (j.contains("artists") && j["artists"].is_object() &&
        j["artists"].contains("items") && j["artists"]["items"].is_array()) {
        for (auto& item : j["artists"]["items"]) {
            sr.artists.push_back(parse_artist(item));
        }
    }
    if (j.contains("playlists") && j["playlists"].is_object() &&
        j["playlists"].contains("items") && j["playlists"]["items"].is_array()) {
        for (auto& item : j["playlists"]["items"]) {
            if (item.is_object()) {
                sr.playlists.push_back(parse_playlist(item));
            }
        }
    }
    return sr;
}

// ---------------------------------------------------------------------------
// SpotifyClient
// ---------------------------------------------------------------------------

SpotifyClient::SpotifyClient(std::shared_ptr<OAuth> oauth)
    : oauth_(std::move(oauth)) {}

SpotifyClient::~SpotifyClient() = default;

void SpotifyClient::set_access_token(const std::string& token) {
    std::lock_guard<std::mutex> lock(token_mutex_);
    access_token_ = token;
}

void SpotifyClient::refresh_token_if_needed() {
    if (!oauth_) return;
    try {
        if (oauth_->needs_refresh()) {
            std::string rt = oauth_->refresh_token();
            if (!rt.empty()) {
                oauth_->refresh_tokens(rt);
            }
        }
    } catch (...) {}
}

static std::string get_bearer_token(SpotifyClient* client, std::shared_ptr<OAuth>& oauth) {
    if (oauth) {
        return oauth->access_token();
    }
    return "";
}

json SpotifyClient::api_get(const std::string& endpoint,
                            const std::map<std::string, std::string>& params) {
    std::string token = oauth_ ? oauth_->access_token() : access_token_;

    std::string url = BASE_URL + endpoint;
    bool first = (endpoint.find('?') == std::string::npos);
    for (auto& [k, v] : params) {
        url += first ? '?' : '&';
        url += k + "=" + std::string(cpr::util::urlEncode(v));
        first = false;
    }

    auto response = cpr::Get(
        cpr::Url{url},
        cpr::Bearer{token},
        cpr::Header{{"Content-Type", "application/json"}}
    );

    if (response.status_code == 401 || response.status_code == 403) {
        std::string rt = oauth_ ? oauth_->refresh_token() : "";
        if (!rt.empty()) {
            try {
                oauth_->refresh_tokens(rt);
                std::string new_token = oauth_->access_token();
                response = cpr::Get(
                    cpr::Url{url},
                    cpr::Bearer{new_token},
                    cpr::Header{{"Content-Type", "application/json"}}
                );
            } catch (const std::exception& e) {
                
            }
        }
    }

    if (response.status_code == 429) {
        return json::object();
    }

    if (response.status_code >= 200 && response.status_code < 300) {
        if (response.text.empty()) return json::object();
        try {
            return json::parse(response.text);
        } catch (...) {
            return json::object();
        }
    }

    return json::object();
}

json SpotifyClient::api_post(const std::string& endpoint, const json& body) {
    std::string token = oauth_ ? oauth_->access_token() : access_token_;

    auto response = cpr::Post(
        cpr::Url{BASE_URL + endpoint},
        cpr::Bearer{token},
        cpr::Header{{"Content-Type", "application/json"}},
        cpr::Body{body.dump()}
    );

    if (response.status_code == 401 || response.status_code == 403) {
        std::string rt = oauth_ ? oauth_->refresh_token() : "";
        if (!rt.empty()) {
            try {
                oauth_->refresh_tokens(rt);
                std::string new_token = oauth_->access_token();
                response = cpr::Post(
                    cpr::Url{BASE_URL + endpoint},
                    cpr::Bearer{new_token},
                    cpr::Header{{"Content-Type", "application/json"}},
                    cpr::Body{body.dump()}
                );
            } catch (const std::exception& e) {
                
            }
        }
    }

    if (response.status_code == 429) {
        return json::object();
    }

    if (response.status_code >= 200 && response.status_code < 300) {
        if (response.text.empty()) return json::object();
        try {
            return json::parse(response.text);
        } catch (...) {
            return json::object();
        }
    }

    return json::object();
}

json SpotifyClient::api_put(const std::string& endpoint, const json& body,
                            const std::map<std::string, std::string>& params) {
    std::string token = oauth_ ? oauth_->access_token() : access_token_;

    std::string url = BASE_URL + endpoint;
    bool first = (endpoint.find('?') == std::string::npos);
    for (auto& [k, v] : params) {
        url += first ? '?' : '&';
        url += k + "=" + std::string(cpr::util::urlEncode(v));
        first = false;
    }

    auto response = cpr::Put(
        cpr::Url{url},
        cpr::Bearer{token},
        cpr::Header{{"Content-Type", "application/json"}},
        cpr::Body{body.dump()}
    );

    if (response.status_code == 401 || response.status_code == 403) {
        std::string rt = oauth_ ? oauth_->refresh_token() : "";
        if (!rt.empty()) {
            try {
                oauth_->refresh_tokens(rt);
                std::string new_token = oauth_->access_token();
                response = cpr::Put(
                    cpr::Url{url},
                    cpr::Bearer{new_token},
                    cpr::Header{{"Content-Type", "application/json"}},
                    cpr::Body{body.dump()}
                );
            } catch (const std::exception& e) {
                
            }
        }
    }

    if (response.status_code == 429) {
        return json::object();
    }

    if (response.status_code >= 200 && response.status_code < 300) {
        if (response.text.empty()) return json::object();
        try {
            return json::parse(response.text);
        } catch (...) {
            return json::object();
        }
    }

    return json::object();
}

json SpotifyClient::api_delete(const std::string& endpoint, const json& body) {
    std::string token = oauth_ ? oauth_->access_token() : access_token_;

    auto response = cpr::Delete(
        cpr::Url{BASE_URL + endpoint},
        cpr::Bearer{token},
        cpr::Header{{"Content-Type", "application/json"}},
        cpr::Body{body.dump()}
    );

    if (response.status_code == 401 || response.status_code == 403) {
        std::string rt = oauth_ ? oauth_->refresh_token() : "";
        if (!rt.empty()) {
            try {
                oauth_->refresh_tokens(rt);
                std::string new_token = oauth_->access_token();
                response = cpr::Delete(
                    cpr::Url{BASE_URL + endpoint},
                    cpr::Bearer{new_token},
                    cpr::Header{{"Content-Type", "application/json"}},
                    cpr::Body{body.dump()}
                );
            } catch (const std::exception& e) {
                
            }
        }
    }

    if (response.status_code == 429) {
        return json::object();
    }

    if (response.status_code >= 200 && response.status_code < 300) {
        if (response.text.empty()) return json::object();
        try {
            return json::parse(response.text);
        } catch (...) {
            return json::object();
        }
    }

    return json::object();
}

// ---------------------------------------------------------------------------
// Playback
// ---------------------------------------------------------------------------

std::optional<PlaybackState> SpotifyClient::get_current_playback() {
    auto j = api_get("/me/player");
    if (j.empty()) return std::nullopt;

    PlaybackState state;
    state.is_playing = j.value("is_playing", false);
    state.progress_ms = j.value("progress_ms", 0);
    state.repeat_state = j.value("repeat_state", "off");

    bool shuffle = false;
    if (j.contains("shuffle_state")) {
        if (j["shuffle_state"].is_boolean()) {
            shuffle = j["shuffle_state"].get<bool>();
        }
    }
    state.shuffle_state = shuffle;

    if (j.contains("device") && j["device"].is_object()) {
        Device d = parse_device(j["device"]);
        state.volume_percent = d.volume_percent;
    }

    if (j.contains("item") && j["item"].is_object()) {
        auto& item = j["item"];
        state.track_name = item.value("name", "");
        state.track_uri = item.value("uri", "");
        state.duration_ms = item.value("duration_ms", 0);
        if (item.contains("artists") && item["artists"].is_array() && !item["artists"].empty()) {
            state.artist_name = item["artists"][0].value("name", "");
        }
        if (item.contains("album") && item["album"].is_object()) {
            state.album_name = item["album"].value("name", "");
        }
    }

    return state;
}

bool SpotifyClient::play(const std::string& context_uri, int offset_position) {
    json body = json::object();
    if (!context_uri.empty()) {
        body["context_uri"] = context_uri;
        if (offset_position >= 0) {
            body["offset"] = json::object();
            body["offset"]["position"] = offset_position;
        }
    }
    auto j = api_put("/me/player/play", body);
    return !j.empty() || j.is_object();
}

bool SpotifyClient::play_tracks(const std::vector<std::string>& uris) {
    json body;
    body["uris"] = uris;
    auto j = api_put("/me/player/play", body);
    return !j.empty() || j.is_object();
}

bool SpotifyClient::pause() {
    auto j = api_put("/me/player/pause");
    return !j.empty() || j.is_object();
}

bool SpotifyClient::next() {
    auto j = api_post("/me/player/next");
    return !j.empty() || j.is_object();
}

bool SpotifyClient::previous() {
    auto j = api_post("/me/player/previous");
    return !j.empty() || j.is_object();
}

bool SpotifyClient::seek(int position_ms) {
    auto j = api_put("/me/player/seek", json::object(),
                     {{"position_ms", std::to_string(position_ms)}});
    return !j.empty() || j.is_object();
}

bool SpotifyClient::set_volume(int volume_percent) {
    volume_percent = std::clamp(volume_percent, 0, 100);
    auto j = api_put("/me/player/volume", json::object(),
                     {{"volume_percent", std::to_string(volume_percent)}});
    return !j.empty() || j.is_object();
}

bool SpotifyClient::set_shuffle(bool state) {
    auto j = api_put("/me/player/shuffle", json::object(),
                     {{"state", state ? "true" : "false"}});
    return !j.empty() || j.is_object();
}

bool SpotifyClient::set_repeat(const std::string& state) {
    auto j = api_put("/me/player/repeat", json::object(),
                     {{"state", state}});
    return !j.empty() || j.is_object();
}

bool SpotifyClient::add_to_queue(const std::string& uri) {
    auto j = api_post("/me/player/queue?uri=" + std::string(cpr::util::urlEncode(uri)));
    return !j.empty() || j.is_object();
}

// ---------------------------------------------------------------------------
// Devices
// ---------------------------------------------------------------------------

std::vector<Device> SpotifyClient::get_devices() {
    auto j = api_get("/me/player/devices");
    std::vector<Device> devices;
    if (!j.contains("devices") || !j["devices"].is_array()) {
        return devices;
    }
    for (auto& d : j["devices"]) {
        devices.push_back(parse_device(d));
    }
    return devices;
}

bool SpotifyClient::transfer_playback(const std::string& device_id, bool play) {
    json body;
    body["device_ids"] = json::array({device_id});
    body["play"] = play;
    auto j = api_put("/me/player", body);
    return !j.empty() || j.is_object();
}

// ---------------------------------------------------------------------------
// Library
// ---------------------------------------------------------------------------

std::vector<Track> SpotifyClient::get_saved_tracks(int limit, int offset) {
    std::string key = "saved:" + std::to_string(limit) + ":" + std::to_string(offset);
    auto cached = tracks_cache_.get(key);
    if (cached) return *cached;

    auto j = api_get("/me/tracks",
                     {{"limit", std::to_string(limit)},
                      {"offset", std::to_string(offset)}});

    std::vector<Track> tracks;
    if (!j.contains("items") || !j["items"].is_array()) {
        return tracks;
    }

    for (auto& item : j["items"]) {
        Track t;
        if (item.contains("track") && item["track"].is_object()) {
            t = parse_track(item["track"]);
        } else if (item.is_object()) {
            t = parse_track(item);
        }
        tracks.push_back(t);
    }

    tracks_cache_.put(key, tracks);
    return tracks;
}

bool SpotifyClient::save_tracks(const std::vector<std::string>& ids) {
    json body;
    body["ids"] = ids;
    auto j = api_put("/me/tracks", body);
    return !j.empty() || j.is_object();
}

bool SpotifyClient::remove_saved_tracks(const std::vector<std::string>& ids) {
    json body;
    body["ids"] = ids;
    auto j = api_delete("/me/tracks", body);
    return !j.empty() || j.is_object();
}

// ---------------------------------------------------------------------------
// Playlists
// ---------------------------------------------------------------------------

std::vector<Playlist> SpotifyClient::get_current_user_playlists(int limit, int offset) {
    std::string key = "playlists:" + std::to_string(limit) + ":" + std::to_string(offset);
    auto cached = playlists_cache_.get(key);
    if (cached) return *cached;

    auto j = api_get("/me/playlists",
                     {{"limit", std::to_string(limit)},
                      {"offset", std::to_string(offset)}});

    std::vector<Playlist> playlists;
    if (!j.contains("items") || !j["items"].is_array()) {
        return playlists;
    }

    for (auto& item : j["items"]) {
        playlists.push_back(parse_playlist(item));
    }

    playlists_cache_.put(key, playlists);
    return playlists;
}

std::optional<Playlist> SpotifyClient::get_playlist(const std::string& playlist_id) {
    std::string key = "playlist:" + playlist_id;
    auto cached_vec = playlists_cache_.get(key);
    if (cached_vec && !cached_vec->empty()) {
        return (*cached_vec)[0];
    }

    auto j = api_get("/playlists/" + playlist_id);
    if (j.empty()) return std::nullopt;

    Playlist p = parse_playlist(j);
    playlists_cache_.put(key, {p});
    return p;
}

std::vector<Track> SpotifyClient::get_playlist_tracks(const std::string& playlist_id,
                                                       int limit, int offset) {
    std::string key = "playlist_tracks:" + playlist_id + ":" +
                      std::to_string(limit) + ":" + std::to_string(offset);
    auto cached = tracks_cache_.get(key);
    if (cached) return *cached;

    auto j = api_get("/playlists/" + playlist_id + "/items",
                     {{"limit", std::to_string(limit)},
                      {"offset", std::to_string(offset)},
                      {"market", "US"}});

    if (j.empty()) {
        tracks_cache_.put(key, std::vector<Track>{});
        return {};
    }

    std::vector<Track> tracks;
    if (j.contains("items") && j["items"].is_array()) {
        for (auto& item : j["items"]) {
            const json* track_ptr = nullptr;
            if (item.contains("track") && !item["track"].is_null()) {
                track_ptr = &item["track"];
            } else if (item.contains("item") && !item["item"].is_null()) {
                track_ptr = &item["item"];
            }
            if (track_ptr) {
                tracks.push_back(parse_track(*track_ptr));
            }
        }
    }

    tracks_cache_.put(key, tracks);
    return tracks;
}

bool SpotifyClient::create_playlist(const std::string& name,
                                    const std::string& description,
                                    bool is_public) {
    json body;
    body["name"] = name;
    body["description"] = description;
    body["public"] = is_public;

    auto j = api_post("/me/playlists", body);
    if (j.empty() || !j.contains("id")) return false;
    playlists_cache_.clear();
    return true;
}

bool SpotifyClient::add_tracks_to_playlist(const std::string& playlist_id,
                                            const std::vector<std::string>& uris) {
    json body;
    body["uris"] = uris;

    auto j = api_post("/playlists/" + playlist_id + "/items", body);

    if (!j.empty()) {
        playlists_cache_.clear();
        tracks_cache_.invalidate("playlist_tracks:" + playlist_id);
    }

    return !j.empty();
}

bool SpotifyClient::remove_tracks_from_playlist(const std::string& playlist_id,
                                                 const std::vector<std::string>& uris) {
    json items = json::array();
    for (auto& uri : uris) {
        json obj;
        obj["uri"] = uri;
        items.push_back(obj);
    }

    json body;
    body["items"] = items;

    auto j = api_delete("/playlists/" + playlist_id + "/items", body);

    if (!j.empty()) {
        playlists_cache_.clear();
        tracks_cache_.invalidate("playlist_tracks:" + playlist_id);
    }

    return !j.empty();
}

// ---------------------------------------------------------------------------
// Search
// ---------------------------------------------------------------------------

SearchResult SpotifyClient::search(const std::string& query, const std::string& type,
                                    int limit) {
    int capped_limit = std::min(limit, 10);
    std::string key = "search:" + query + ":" + type + ":" + std::to_string(capped_limit);
    auto cached = search_cache_.get(key);
    if (cached) return *cached;

    

    auto j = api_get("/search",
                     {{"q", query},
                      {"type", type},
                      {"limit", std::to_string(capped_limit)},
                      {"market", "US"}});

    
    if (j.empty()) return {};

    SearchResult result = parse_search_result(j);
    search_cache_.put(key, result);
    return result;
}

// ---------------------------------------------------------------------------
// Browse
// ---------------------------------------------------------------------------

std::vector<Track> SpotifyClient::get_top_tracks(int limit) {
    std::string key = "top_tracks:" + std::to_string(limit);
    auto cached = tracks_cache_.get(key);
    if (cached) return *cached;

    auto j = api_get("/me/top/tracks",
                     {{"limit", std::to_string(limit)}});

    std::vector<Track> tracks;
    if (j.empty()) return tracks;

    if (j.contains("items") && j["items"].is_array()) {
        for (auto& item : j["items"]) {
            tracks.push_back(parse_track(item));
        }
    }

    tracks_cache_.put(key, tracks);
    return tracks;
}

std::vector<Artist> SpotifyClient::get_top_artists(int limit) {
    std::string key = "top_artists:" + std::to_string(limit);
    auto cached = artists_cache_.get(key);
    if (cached) return *cached;

    auto j = api_get("/me/top/artists",
                     {{"limit", std::to_string(limit)}});

    std::vector<Artist> artists;
    if (j.empty()) return artists;

    if (j.contains("items") && j["items"].is_array()) {
        for (auto& item : j["items"]) {
            artists.push_back(parse_artist(item));
        }
    }

    artists_cache_.put(key, artists);
    return artists;
}

std::vector<Playlist> SpotifyClient::get_followed_playlists(int limit) {
    std::string key = "followed:" + std::to_string(limit);
    auto cached = playlists_cache_.get(key);
    if (cached) return *cached;

    auto j = api_get("/me/playlists",
                     {{"limit", std::to_string(limit)},
                      {"offset", "0"}});

    std::vector<Playlist> playlists;
    if (!j.contains("items") || !j["items"].is_array()) {
        return playlists;
    }

    if (j.contains("items") && j["items"].is_array()) {
        for (auto& item : j["items"]) {
            if (item.is_object()) {
                playlists.push_back(parse_playlist(item));
            }
        }
    }

    playlists_cache_.put(key, playlists);
    return playlists;
}

std::vector<Track> SpotifyClient::get_album_tracks(const std::string& album_id, int limit) {
    std::string key = "album_tracks:" + album_id + ":" + std::to_string(limit);
    auto cached = tracks_cache_.get(key);
    if (cached) return *cached;

    auto j = api_get("/albums/" + album_id + "/tracks",
                     {{"limit", std::to_string(limit)},
                      {"market", "US"}});

    std::vector<Track> tracks;
    if (j.empty()) return tracks;

    if (j.contains("items") && j["items"].is_array()) {
        for (auto& item : j["items"]) {
            if (!item.is_null()) {
                tracks.push_back(parse_track(item));
            }
        }
    }

    tracks_cache_.put(key, tracks);
    return tracks;
}

std::vector<Album> SpotifyClient::get_artist_albums(const std::string& artist_id, int limit) {
    std::string key = "artist_albums:" + artist_id + ":" + std::to_string(limit);
    auto cached = albums_cache_.get(key);
    if (cached) return *cached;

    auto j = api_get("/artists/" + artist_id + "/albums",
                     {{"limit", std::to_string(limit)},
                      {"market", "US"}});

    std::vector<Album> albums;
    if (j.empty()) return albums;

    if (j.contains("items") && j["items"].is_array()) {
        for (auto& item : j["items"]) {
            if (!item.is_null()) {
                albums.push_back(parse_album(item));
            }
        }
    }

    albums_cache_.put(key, albums);
    return albums;
}

// ---------------------------------------------------------------------------
// User
// ---------------------------------------------------------------------------

std::string SpotifyClient::get_current_user_id() {
    auto j = api_get("/me");
    if (j.empty() || !j.contains("id")) return "";
    return j["id"].get<std::string>();
}

}  // namespace spotify_tui
