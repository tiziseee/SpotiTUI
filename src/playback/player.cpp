#include "spotify_tui/playback/player.hpp"
#include "spotify_tui/playback/librespot_process.hpp"
#include "spotify_tui/api/oauth.hpp"
#include "spotify_tui/utils/logger.hpp"

#include <algorithm>

namespace spotify_tui {

Player::Player(std::shared_ptr<SpotifyClient> client, std::shared_ptr<OAuth> oauth)
    : client_(std::move(client)), oauth_(std::move(oauth)) {}

Player::~Player() {
    stop_librespot();
}

bool Player::play_tracks(const std::vector<std::string>& uris) {
    if (!ensure_device()) {
        return false;
    }
    return client_->play_tracks(uris);
}

bool Player::play_context(const std::string& context_uri, int offset_position) {
    if (!ensure_device()) {
        return false;
    }
    return client_->play(context_uri, offset_position);
}

bool Player::toggle_playback() {
    auto state = client_->get_current_playback();
    if (!state) {
        return false;
    }

    if (state->is_playing) {
        return client_->pause();
    } else {
        return client_->play();
    }
}

bool Player::pause() {
    return client_->pause();
}

bool Player::next_track() {
    return client_->next();
}

bool Player::previous_track() {
    return client_->previous();
}

bool Player::seek(int position_ms) {
    return client_->seek(position_ms);
}

bool Player::set_volume(int percent) {
    return client_->set_volume(percent);
}

bool Player::set_shuffle(bool state) {
    return client_->set_shuffle(state);
}

bool Player::set_repeat(const std::string& state) {
    return client_->set_repeat(state);
}

bool Player::add_to_queue(const std::string& uri) {
    return client_->add_to_queue(uri);
}

bool Player::like_current_track() {
    auto state = client_->get_current_playback();
    if (!state || state->track_uri.empty()) {
        return false;
    }

    std::string uri = state->track_uri;
    std::string prefix = "spotify:track:";
    if (uri.find(prefix) == 0) {
        std::string track_id = uri.substr(prefix.size());
        return client_->save_tracks({track_id});
    }

    return false;
}

std::optional<PlaybackState> Player::get_state() {
    return client_->get_current_playback();
}

void Player::start_librespot() {
    if (librespot_ && librespot_->is_running()) {
        return;
    }

    librespot_ = std::make_unique<LibrespotProcess>(DEVICE_NAME);
    librespot_->start(oauth_->access_token());
}

void Player::stop_librespot() {
    if (librespot_) {
        librespot_->stop();
        librespot_.reset();
    }
}

bool Player::is_librespot_running() const {
    return librespot_ && librespot_->is_running();
}

bool Player::ensure_device() {
    auto devices = client_->get_devices();

    for (const auto& device : devices) {
        if (device.name == DEVICE_NAME) {
            current_device_id_ = device.id;
            if (!device.is_active) {
                return client_->transfer_playback(device.id, false);
            }
            return true;
        }
    }

    if (!is_librespot_running()) {
        Logger::get()->info("Librespot not running, starting it now...");
        start_librespot();
    }

    return false;
}

} // namespace spotify_tui
