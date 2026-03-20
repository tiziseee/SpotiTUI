#include "spotify_tui/ui/main_ui.hpp"
#include "spotify_tui/ui/components/progress_bar.hpp"
#include <ftxui/dom/elements.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <algorithm>
#include <cstdio>
#include <chrono>
#include <thread>

namespace spotify_tui {
namespace ui {

using namespace ftxui;

namespace {

std::string fmt_time(int ms) {
    int s = ms / 1000;
    int m = s / 60;
    s %= 60;
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%d:%02d", m, s);
    return buf;
}

std::string truncate_str(const std::string& s, int w) {
    if ((int)s.size() <= w) return s;
    return s.substr(0, w - 1) + "\xe2\x80\xa6";
}

} // namespace

class MainUI::Impl {
public:
    Impl(MainUI* outer, std::shared_ptr<spotify_tui::SpotifyClient> client, std::shared_ptr<spotify_tui::Player> player)
        : outer_(outer), client_(std::move(client)), player_(std::move(player)) {
        build_layout();
        auto comp = Container::Vertical({
            middle_row_ | flex,
            player_controls_
        }) | flex;

        component_ = CatchEvent(comp, [this](Event e) -> bool {
            return this->catch_event(e);
        });
    }

    Component GetComponent() { return component_; }

    void build_layout() {
        build_sidebar();
        build_home();
        build_playlists();
        build_search();
        build_liked();
        build_player_controls();

        tab_container_ = Container::Tab({
            home_content_,
            playlists_content_,
            search_content_,
            liked_content_
        }, &tab_index_);

        auto content_area = Container::Vertical({
            tab_container_
        });

        middle_row_ = Container::Horizontal({
            sidebar_container_ | size(WIDTH, LESS_THAN, 18) | border,
            content_area | flex | border
        });
    }

    void build_sidebar() {
        sidebar_entries_ = {
            "⏮ Prev  [P]",
            "▶ Play  [SPC]",
            "⏭ Next  [N]",
            "──────────────",
            "🏠 Home  [1]",
            "📋 Playlists [2]",
            "🔍 Search[3]",
            "❤ Liked [4]",
            "──────────────",
            "🔀 Shuffle",
            "🔁 Repeat",
            "❤ Like",
            "📋 Queue",
        };
        auto menu_option = MenuOption::Vertical();
        menu_option.on_enter = [this] {
            handle_sidebar_action();
        };
        sidebar_menu_ = Menu(&sidebar_entries_, &sidebar_selected_, menu_option);

        sidebar_container_ = Container::Vertical({sidebar_menu_});
    }

    void handle_sidebar_action() {
        switch (sidebar_selected_) {
            case 0: player_->previous_track(); break;
            case 1: player_->toggle_playback(); break;
            case 2: player_->next_track(); break;
            case 4: tab_index_ = 0; outer_->refresh_home(); break;
            case 5: tab_index_ = 1; break;
            case 6: tab_index_ = 2; break;
            case 7: tab_index_ = 3; outer_->load_liked_tracks(); break;
            case 9: {
                auto st = player_->get_state();
                player_->set_shuffle(!st.value_or(spotify_tui::PlaybackState{}).shuffle_state);
                break;
            }
            case 10: {
                auto st = player_->get_state();
                std::string rs = st.value_or(spotify_tui::PlaybackState{}).repeat_state;
                if (rs == "off") player_->set_repeat("context");
                else if (rs == "context") player_->set_repeat("track");
                else player_->set_repeat("off");
                break;
            }
            case 11: {
                auto st = player_->get_state();
                if (st && !st->track_uri.empty()) {
                    std::string uri = st->track_uri;
                    if (uri.find("spotify:track:") == 0) {
                        client_->save_tracks({uri.substr(14)});
                        outer_->status_text_ = "❤ Liked!";
                    }
                }
                break;
            }
            case 12: {
                auto st = player_->get_state();
                if (st && !st->track_uri.empty()) {
                    int idx = -1;
                    for (int i = 0; i < (int)outer_->current_track_uris_.size(); ++i) {
                        if (outer_->current_track_uris_[i] == st->track_uri) { idx = i; break; }
                    }
                    if (idx + 1 < (int)outer_->current_track_uris_.size()) {
                        player_->add_to_queue(outer_->current_track_uris_[idx + 1]);
                        outer_->status_text_ = "📋 Queued next";
                    }
                }
                break;
            }
        }
    }

    void build_home() {
        auto option = MenuOption::Vertical();
        option.on_enter = [this] {
            if (home_selected_ >= 0 && home_selected_ < (int)outer_->top_track_uris_.size()) {
                outer_->current_context_uri_ = "";
                outer_->play_track_at(home_selected_, outer_->top_track_uris_);
            }
        };
        home_menu_ = Menu(&outer_->top_track_names_, &home_selected_, option);

        home_content_ = Container::Vertical({
            home_menu_ | flex
        });
    }

    void build_playlists() {
        auto list_option = MenuOption::Vertical();
        list_option.on_enter = [this] {
            if (outer_->current_context_type_ == "artist") {
                if (playlist_list_selected_ >= 0 && playlist_list_selected_ < (int)outer_->artist_albums_.size()) {
                    outer_->load_album(outer_->artist_albums_[playlist_list_selected_].id);
                }
            } else {
                if (playlist_list_selected_ >= 0 && playlist_list_selected_ < (int)outer_->playlists_.size()) {
                    outer_->load_playlist(outer_->playlists_[playlist_list_selected_].id);
                }
            }
        };
        playlist_list_menu_ = Menu(&outer_->playlist_names_, &playlist_list_selected_, list_option);

        auto track_option = MenuOption::Vertical();
        track_option.on_enter = [this] {
            if (track_list_selected_ >= 0 && track_list_selected_ < (int)outer_->current_track_uris_.size()) {
                outer_->play_track_at(track_list_selected_, outer_->current_track_uris_);
            }
        };
        track_list_menu_ = Menu(&outer_->current_track_names_, &track_list_selected_, track_option);

        auto scrollable_tracks = Renderer(track_list_menu_, [&] {
            return track_list_menu_->Render() | vscroll_indicator | frame | size(HEIGHT, LESS_THAN, 100);
        });

        auto playlists_and_tracks = Container::Horizontal({
            playlist_list_menu_ | size(WIDTH, LESS_THAN, 30) | flex,
            scrollable_tracks | flex
        });

        playlists_content_ = Container::Vertical({
            playlists_and_tracks | flex
        });
    }

    void build_search() {
        search_input_option_.placeholder = "Search tracks, albums, artists...";
        search_input_option_.on_enter = [this] {
            outer_->do_search(outer_->search_query_);
        };
        search_input_ = Input(&outer_->search_query_, "Search...", search_input_option_);

        auto search_btn = Button({
            .label = "[Search ↩]",
            .on_click = [this] { outer_->do_search(outer_->search_query_); }
        });

        auto search_row = Container::Horizontal({search_input_, search_btn});

        search_tabs_ = Toggle(&search_type_labels_, &search_type_selected_);

        auto results_option = MenuOption::Vertical();
        results_option.on_enter = [this] {
            handle_search_enter();
        };
        search_results_menu_ = Menu(&search_result_names_, &search_result_selected_, results_option);

        search_content_ = Container::Vertical({
            search_row | border,
            search_tabs_,
            search_results_menu_ | flex
        });
    }

    void handle_search_enter() {
        if (search_type_selected_ == 0) {
            if (search_result_selected_ >= 0 && search_result_selected_ < (int)outer_->search_track_uris_.size()) {
                outer_->play_track_at(search_result_selected_, outer_->search_track_uris_, false);
            }
        } else if (search_type_selected_ == 1) {
            if (search_result_selected_ >= 0 && search_result_selected_ < (int)outer_->search_result_.albums.size()) {
                outer_->load_album(outer_->search_result_.albums[search_result_selected_].id);
                tab_index_ = 1;
            }
        } else if (search_type_selected_ == 2) {
            if (search_result_selected_ >= 0 && search_result_selected_ < (int)outer_->search_result_.artists.size()) {
                outer_->load_artist(outer_->search_result_.artists[search_result_selected_].id);
                tab_index_ = 1;
            }
        }
    }

    void build_liked() {
        auto option = MenuOption::Vertical();
        option.on_enter = [this] {
            if (liked_track_selected_ >= 0 && liked_track_selected_ < (int)outer_->liked_tracks_.size()) {
                auto start_idx = liked_track_selected_;
                std::vector<std::string> uris_to_play(outer_->liked_track_uris_.begin() + start_idx, outer_->liked_track_uris_.end());
                player_->play_tracks(uris_to_play);
                outer_->track_index_ = start_idx;
                outer_->current_context_uri_ = "liked";
            }
        };
        liked_menu_ = Menu(&outer_->liked_track_names_, &liked_track_selected_, option);

        auto scrollable_liked = Renderer(liked_menu_, [&] {
            return liked_menu_->Render() | vscroll_indicator | frame | size(HEIGHT, LESS_THAN, 100);
        });

        liked_content_ = Container::Vertical({
            scrollable_liked | flex
        });
    }

    void build_player_controls() {
        progress_bar_ = MakeProgressBar(&outer_->progress_ms_, &outer_->duration_ms_,
            [this](int ms) {
                player_->seek(ms);
                outer_->progress_ms_ = ms;
            });

        volume_slider_ = MakeVolumeSlider(&outer_->volume_,
            [this](int vol) {
                player_->set_volume(vol);
            });

        prev_btn_ = Button({
            .label = "⏮ PREV",
            .on_click = [this] { player_->previous_track(); }
        });
        play_pause_btn_ = Button({
            .label = outer_->is_playing_ ? "⏸ PAUSE" : "▶ PLAY",
            .on_click = [this] { player_->toggle_playback(); }
        });
        next_btn_ = Button({
            .label = "NEXT ⏭",
            .on_click = [this] { player_->next_track(); }
        });
        shuffle_btn_ = Button({
            .label = "🔀 SHUF",
            .on_click = [this] {
                auto st = player_->get_state();
                player_->set_shuffle(!st.value_or(spotify_tui::PlaybackState{}).shuffle_state);
            }
        });
        repeat_btn_ = Button({
            .label = "🔁 REPT",
            .on_click = [this] {
                auto st = player_->get_state();
                std::string rs = st.value_or(spotify_tui::PlaybackState{}).repeat_state;
                if (rs == "off") player_->set_repeat("context");
                else if (rs == "context") player_->set_repeat("track");
                else player_->set_repeat("off");
            }
        });

        auto btn_row1 = Container::Horizontal({
            prev_btn_, play_pause_btn_, next_btn_
        });
        auto btn_row2 = Container::Horizontal({
            shuffle_btn_, repeat_btn_
        });

        auto title_renderer = Renderer([this] {
            return ftxui::text(outer_->now_playing_text_) | ftxui::color(ftxui::Color::White);
        });

        auto player_row = Container::Horizontal({
            title_renderer,
            progress_bar_ | flex,
            volume_slider_
        }) | flex;

        auto state_indicator = Renderer([this] {
            std::string s = "";
            if (outer_->shuffle_btn_label_ != "🔀 SHUF") s += "🔀 ";
            if (outer_->repeat_btn_label_ != "🔁 REPT") s += outer_->repeat_btn_label_;
            if (s.empty()) s = " ";
            return ftxui::text(s) | ftxui::center;
        });

        player_controls_ = Container::Vertical({
            btn_row1,
            btn_row2,
            state_indicator,
            player_row
        }) | border;
    }

    void update_search_results() {
        search_result_names_.clear();
        outer_->search_track_uris_.clear();

        int stype = search_type_selected_;
        if (stype == 0) {
            for (auto& t : outer_->search_result_.tracks) {
                outer_->search_track_uris_.push_back(t.uri);
                search_result_names_.push_back(truncate_str(t.name, 40) + " - " + truncate_str(t.artist, 20));
            }
        } else if (stype == 1) {
            for (auto& a : outer_->search_result_.albums) {
                search_result_names_.push_back(truncate_str(a.name, 40) + " - " + truncate_str(a.artist, 20));
            }
        } else if (stype == 2) {
            for (auto& a : outer_->search_result_.artists) {
                search_result_names_.push_back(truncate_str(a.name, 40));
            }
        }
    }

    bool catch_event(const Event& e) {
        if (e.is_mouse()) return false;
        if (search_input_->Focused()) return false;
        auto input = e.input();
        if (input.size() >= 2 && input[0] == '\x1b') {
            if (input == "\x1b[1;5A") { outer_->volume_updating_ = true; outer_->volume_ = std::min(outer_->volume_ + 5, 100); player_->set_volume(outer_->volume_); std::thread([this] { std::this_thread::sleep_for(std::chrono::milliseconds(200)); outer_->volume_updating_ = false; }).detach(); return true; }
            if (input == "\x1b[1;5B") { outer_->volume_updating_ = true; outer_->volume_ = std::max(outer_->volume_ - 5, 0); player_->set_volume(outer_->volume_); std::thread([this] { std::this_thread::sleep_for(std::chrono::milliseconds(200)); outer_->volume_updating_ = false; }).detach(); return true; }
            return false;
        }
        return handle_keyboard(e);
    }

    bool handle_keyboard(const Event& e) {
        if (e == Event::Character(' ')) { player_->toggle_playback(); return true; }
        if (e == Event::Character('/')) { player_->toggle_playback(); return true; }
        if (e.input() == "\x10") { player_->previous_track(); return true; }
        if (e.input() == "\x0e") { player_->next_track(); return true; }
        if (e.input() == "\x02") { player_->pause(); return true; }
        if (e == Event::Character('n')) { player_->next_track(); return true; }
        if (e == Event::Character('p')) { player_->previous_track(); return true; }
        if (e == Event::Character('+')) {
            outer_->volume_ = std::min(outer_->volume_ + 5, 100);
            player_->set_volume(outer_->volume_); return true;
        }
        if (e == Event::Character('-')) {
            outer_->volume_ = std::max(outer_->volume_ - 5, 0);
            player_->set_volume(outer_->volume_); return true;
        }
        if (e == Event::Tab) { tab_index_ = (tab_index_ + 1) % 4; return true; }
        if (e == Event::Escape) { outer_->running_ = false; return true; }
        if (e == Event::F5) { outer_->refresh_home(); return true; }
        if (e == Event::Character('1')) { tab_index_ = 0; outer_->refresh_home(); return true; }
        if (e == Event::Character('2')) { tab_index_ = 1; outer_->refresh_playlists(); return true; }
        if (e == Event::Character('3')) { tab_index_ = 2; return true; }
        if (e == Event::Character('4')) { tab_index_ = 3; outer_->load_liked_tracks(); return true; }
        if (e == Event::Character('s')) { 
            auto st = player_->get_state();
            player_->set_shuffle(!st.value_or(spotify_tui::PlaybackState{}).shuffle_state); return true; 
        }
        if (e == Event::Character('r')) {
            auto st = player_->get_state();
            std::string rs = st.value_or(spotify_tui::PlaybackState{}).repeat_state;
            if (rs == "off") player_->set_repeat("context");
            else if (rs == "context") player_->set_repeat("track");
            else player_->set_repeat("off");
            return true;
        }
        if (e == Event::Character('q')) {
            auto st = player_->get_state();
            if (st && !st->track_uri.empty()) {
                int idx = -1;
                for (int i = 0; i < (int)outer_->current_track_uris_.size(); ++i) {
                    if (outer_->current_track_uris_[i] == st->track_uri) { idx = i; break; }
                }
                if (idx + 1 < (int)outer_->current_track_uris_.size()) {
                    player_->add_to_queue(outer_->current_track_uris_[idx + 1]);
                    outer_->status_text_ = "📋 Added to queue";
                }
            }
            return true;
        }
        return false;
    }

    MainUI* outer_;
    std::shared_ptr<spotify_tui::SpotifyClient> client_;
    std::shared_ptr<spotify_tui::Player> player_;
    Component component_;

    int tab_index_ = 0;
    int sidebar_selected_ = 4;
    int home_selected_ = 0;
    int playlist_list_selected_ = 0;
    int search_result_selected_ = 0;
    int liked_track_selected_ = 0;
    int track_list_selected_ = 0;

    std::vector<std::string> sidebar_entries_;
    std::vector<std::string> search_type_labels_{"Tracks", "Albums", "Artists"};
    int search_type_selected_ = 0;
    std::vector<std::string> search_result_names_;

    Component sidebar_menu_;
    Component sidebar_container_;
    Component home_menu_;
    Component playlist_list_menu_;
    Component track_list_menu_;
    Component home_content_;
    Component playlists_content_;
    Component search_content_;
    Component liked_content_;
    Component tab_container_;
    Component middle_row_;
    Component player_controls_;
    Component progress_bar_;
    Component volume_slider_;
    Component prev_btn_;
    Component play_pause_btn_;
    Component next_btn_;
    Component shuffle_btn_;
    Component repeat_btn_;
    Component search_tabs_;
    Component search_input_;
    Component search_results_menu_;
    Component liked_menu_;
    InputOption search_input_option_;
};

MainUI::MainUI(std::shared_ptr<spotify_tui::SpotifyClient> client, std::shared_ptr<spotify_tui::Player> player)
    : client_(std::move(client)), player_(std::move(player)),
      screen_(ScreenInteractive::Fullscreen()),
      impl_(std::make_unique<Impl>(this, client_, player_)) {}

MainUI::~MainUI() {
    running_ = false;
    if (poll_thread_.joinable()) poll_thread_.join();
}

void MainUI::run() {
    running_ = true;

    poll_thread_ = std::thread([this]() {
        while (running_) {
            poll_playback();
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    });

    refresh_home();
    refresh_playlists();
    screen_.TrackMouse();
    screen_.Loop(impl_->GetComponent());
}

void MainUI::request_render() {
    screen_.Post(Event::Custom);
    screen_.RequestAnimationFrame();
}

void MainUI::poll_playback() {
    if (!running_) return;
    auto state = player_->get_state();
    if (state) {
        bool changed = (is_playing_ != state->is_playing) ||
                       (progress_ms_ / 1000 != state->progress_ms / 1000) ||
                       (shuffle_btn_label_ == (state->shuffle_state ? "🔀 SHUF ON" : "🔀 SHUF") ? false : true);
        
        is_playing_ = state->is_playing;
        progress_ms_ = state->progress_ms;
        duration_ms_ = state->duration_ms;
        if (!volume_updating_) {
            volume_ = state->volume_percent;
        }
        std::string new_shuffle = state->shuffle_state ? "🔀 SHUF ON" : "🔀 SHUF";
        std::string new_repeat = state->repeat_state == "off" ? "🔁 REPT" : 
                           state->repeat_state == "context" ? "🔁 REPT CONT" : "🔁 REPT 1";
        changed = changed || (shuffle_btn_label_ != new_shuffle) || (repeat_btn_label_ != new_repeat);
        shuffle_btn_label_ = std::move(new_shuffle);
        repeat_btn_label_ = std::move(new_repeat);
        
        std::string new_now_playing;
        if (!state->track_name.empty()) {
            new_now_playing.reserve(state->track_name.size() + state->artist_name.size() + 5);
            new_now_playing = "\xe2\x96\xb6 ";
            new_now_playing += state->track_name;
            new_now_playing += " - ";
            new_now_playing += state->artist_name;
        } else {
            new_now_playing = "\xe2\x8f\xb8 No track playing";
        }
        changed = changed || (now_playing_text_ != new_now_playing);
        now_playing_text_ = std::move(new_now_playing);
        
        if (changed) {
            request_render();
        }
    }
}

void MainUI::refresh_home() {
    top_tracks_ = client_->get_top_tracks(20);
    top_track_uris_.clear();
    top_track_uris_.reserve(top_tracks_.size());
    top_track_names_.clear();
    top_track_names_.reserve(top_tracks_.size());
    for (auto& t : top_tracks_) {
        top_track_uris_.push_back(t.uri);
        top_track_names_.push_back(truncate_str(t.name, 50) + " - " + truncate_str(t.artist, 25));
    }
}

void MainUI::refresh_playlists() {
    playlists_ = client_->get_current_user_playlists(50, 0);
    playlist_names_.clear();
    playlist_names_.reserve(playlists_.size());
    for (auto& p : playlists_) {
        playlist_names_.push_back(truncate_str(p.name, 30));
    }
}

void MainUI::load_playlist(const std::string& id) {
    current_tracks_ = client_->get_playlist_tracks(id, 100, 0);
    auto pl = client_->get_playlist(id);
    if (pl) current_context_name_ = pl->name;
    current_context_uri_ = "spotify:playlist:" + id;
    current_context_type_ = "playlist";
    
    impl_->track_list_selected_ = 0;
    current_track_uris_.clear();
    current_track_names_.clear();
    for (auto& t : current_tracks_) {
        current_track_uris_.push_back(t.uri);
        current_track_names_.push_back(truncate_str(t.name, 40) + " - " + truncate_str(t.artist, 20));
    }
    artist_albums_.clear();
    artist_album_names_.clear();
    status_text_ = "📋 " + current_context_name_;
}

void MainUI::load_album(const std::string& id) {
    current_tracks_ = client_->get_album_tracks(id, 50);
    current_context_uri_ = "spotify:album:" + id;
    current_context_type_ = "album";
    
    for (auto& t : current_tracks_) {
        if (t.artist.empty()) t.artist = "Unknown Artist";
    }
    
    Album* album_found = nullptr;
    for (auto& a : search_result_.albums) {
        if (a.id == id) { album_found = &a; break; }
    }
    if (!album_found) {
        for (auto& a : artist_albums_) {
            if (a.id == id) { album_found = &a; break; }
        }
    }
    if (album_found) {
        current_context_name_ = album_found->name + " - " + album_found->artist;
    } else {
        current_context_name_ = "Album";
    }
    
    impl_->track_list_selected_ = 0;
    current_track_names_ = {};
    current_track_uris_ = {};
    for (auto& t : current_tracks_) {
        current_track_names_.push_back(truncate_str(t.name, 40) + " - " + truncate_str(t.artist, 20));
        current_track_uris_.push_back(t.uri);
    }
    status_text_ = "💿 " + current_context_name_;
    request_render();
}

void MainUI::load_artist(const std::string& id) {
    artist_albums_ = client_->get_artist_albums(id, 10);
    
    auto artist_it = std::find_if(search_result_.artists.begin(), search_result_.artists.end(),
        [&id](const Artist& a) { return a.id == id; });
    if (artist_it != search_result_.artists.end()) {
        current_context_name_ = artist_it->name;
    } else {
        current_context_name_ = "Artist";
    }
    
    impl_->playlist_list_selected_ = 0;
    playlists_.clear();
    playlist_names_.clear();
    for (auto& a : artist_albums_) {
        playlist_names_.push_back(truncate_str(a.name, 30));
    }
    
    current_context_type_ = "artist";
    current_context_uri_ = "spotify:artist:" + id;
    
    current_track_uris_.clear();
    current_track_names_.clear();
    status_text_ = "🎤 " + current_context_name_ + " - " + std::to_string(artist_albums_.size()) + " albums";
}

void MainUI::load_liked_tracks() {
    current_context_uri_.clear();
    liked_tracks_ = client_->get_saved_tracks(50, 0);
    liked_track_uris_.clear();
    liked_track_names_.clear();
    for (auto& t : liked_tracks_) {
        liked_track_uris_.push_back(t.uri);
        liked_track_names_.push_back(truncate_str(t.name, 40) + " - " + truncate_str(t.artist, 20));
    }
}

void MainUI::do_search(const std::string& query) {
    if (query.empty()) return;
    search_result_ = client_->search(query, "track,album,artist", 10);
    impl_->search_result_selected_ = 0;
    impl_->update_search_results();
    status_text_ = "🔍 " + std::to_string(search_result_.tracks.size()) + " tracks, " 
                 + std::to_string(search_result_.albums.size()) + " albums, " 
                 + std::to_string(search_result_.artists.size()) + " artists";
}

void MainUI::play_track_at(int index, const std::vector<std::string>& uris, bool use_context) {
    if (index < 0 || index >= (int)uris.size()) return;
    if (use_context && !current_context_uri_.empty()) {
        player_->play_context(current_context_uri_, index);
    } else {
        player_->play_tracks({uris[index]});
    }
    track_index_ = index;
    if (index < (int)current_tracks_.size()) {
        status_text_ = "▶ " + current_tracks_[index].name;
    }
}

void MainUI::update_now_playing(const spotify_tui::PlaybackState& state) {
    std::ostringstream oss;
    if (!state.track_name.empty()) {
        oss << "▶ " << state.track_name << " - " << state.artist_name;
    } else {
        oss << "⏸ No track playing";
    }
    now_playing_text_ = oss.str();
}

std::string MainUI::format_duration(int ms) {
    return fmt_time(ms);
}

std::string MainUI::format_progress(int ms) {
    return fmt_time(ms);
}

} // namespace ui
} // namespace spotify_tui
