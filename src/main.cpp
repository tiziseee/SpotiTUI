#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <atomic>
#include "spotify_tui/app/config.hpp"
#include "spotify_tui/utils/logger.hpp"
#include "spotify_tui/utils/platform.hpp"
#include "spotify_tui/api/oauth.hpp"
#include "spotify_tui/api/client.hpp"
#include "spotify_tui/playback/player.hpp"
#include "spotify_tui/ui/main_ui.hpp"

using namespace spotify_tui;

namespace {
    std::atomic<bool> g_running{true};
}

int main(int argc, char** argv) {
    platform::setup_signal_handler([] { g_running = false; });

    Logger::init(platform::get_log_path());
    auto& log = Logger::get();
    log->info("SpotiTUI starting...");

    std::string config_path = platform::get_config_dir() + "/config.toml";

    Config config(config_path);
    if (!config.load()) {
        std::cerr << "No config found at " << config_path << ". Creating default.\n";
        std::cerr << "Please edit " << config_path << " and add your Spotify client_id and client_secret.\n";
        config.spotify.client_id = "YOUR_CLIENT_ID";
        config.spotify.client_secret = "YOUR_CLIENT_SECRET";
        config.create_default();
        return 1;
    }

    if (config.spotify.client_id == "YOUR_CLIENT_ID" || config.spotify.client_id.empty()) {
        std::cerr << "Please set your Spotify client_id in " << config_path << "\n";
        return 1;
    }

    if (config.spotify.client_secret.empty()) {
        std::cerr << "Please set your Spotify client_secret in " << config_path << "\n";
        return 1;
    }

    auto oauth = std::make_shared<OAuth>(config.spotify.client_id, config.spotify.client_secret, config.spotify.redirect_uri);

    if (!config.spotify.refresh_token.empty()) {
        log->info("Refreshing existing token...");
        try {
            auto result = oauth->refresh_tokens(config.spotify.refresh_token);
            config.spotify.refresh_token = result.refresh_token;
            log->info("Token refreshed successfully");
        } catch (const std::exception& e) {
            log->warn("Token refresh failed: {}", e.what());
            std::cerr << "Token refresh failed. Please re-authenticate.\n";
            try {
                auto tokens = oauth->start_auth_flow();
                config.spotify.refresh_token = tokens.refresh_token;
                log->info("New authentication successful");
            } catch (const std::exception& e2) {
                log->error("Auth failed: {}", e2.what());
                std::cerr << "Authentication failed: " << e2.what() << "\n";
                return 1;
            }
        }
    } else {
        log->info("Starting new auth flow...");
        std::cerr << "No refresh token found. Starting authentication...\n";
        try {
            auto tokens = oauth->start_auth_flow();
            config.spotify.refresh_token = tokens.refresh_token;
            log->info("Authentication successful");
        } catch (const std::exception& e) {
            log->error("Auth failed: {}", e.what());
            std::cerr << "Authentication failed: " << e.what() << "\n";
            return 1;
        }
    }

    config.save();
    log->info("Config saved");

    auto client = std::make_shared<SpotifyClient>(oauth);
    auto player = std::make_shared<Player>(client, oauth);

    std::cerr << "\n"
              << "============================================" << std::endl;
    std::cerr << "Starting librespot..." << std::endl;
    std::cerr << "Librespot will use its own OAuth (browser-based)." << std::endl;
    std::cerr << "Check " << platform::get_log_path() << " for the auth URL if needed." << std::endl;
    std::cerr << "============================================" << std::endl;

    player->start_librespot();

    std::cerr << R"(
╔═══════════════════════════════════════════════════════════╗
║                   🎵  Spotify TUI  🎵                   ║
╠═══════════════════════════════════════════════════════════╣
║                                                           ║
║  NAVIGATION        │  PLAYBACK        │  OTHER           ║
║  ──────────────────┼──────────────────┼─────────────────  ║
║  1  Home           │  Space  Play/P   │  r  Repeat       ║
║  2  Playlists      │  s  Shuffle      │  F5  Refresh     ║
║  3  Search         │  +/-  Volume     │  Esc  Quit       ║
║  4  Liked          │  ←/→ Ctrl Skip   │                  ║
║  Tab  Switch Tab   │  ↑/↓ Ctrl Vol    │                  ║
║                                                           ║
║  Click any button or menu item with your mouse!             ║
╚═══════════════════════════════════════════════════════════╝
)";

    try {
        ui::MainUI ui(client, player);
        ui.run();
    } catch (const std::exception& e) {
        log->error("Fatal error: {}", e.what());
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }

    player->stop_librespot();
    log->info("SpotiTUI shutting down");
    return 0;
}
