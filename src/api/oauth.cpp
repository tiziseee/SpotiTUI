#include "spotify_tui/api/oauth.hpp"
#include "spotify_tui/utils/platform.hpp"
#include <httplib.h>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/rand.h>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <thread>

namespace spotify_tui {

static const std::string AUTH_URL = "https://accounts.spotify.com/authorize";
static const std::string TOKEN_URL = "https://accounts.spotify.com/api/token";
static const std::string SCOPES =
    "user-read-playback-state user-modify-playback-state user-read-currently-playing "
    "user-library-read user-library-modify user-read-recently-played playlist-read-private "
    "playlist-read-collaborative playlist-modify-public playlist-modify-private "
    "user-follow-read user-top-read user-read-private user-read-email streaming";

static std::string base64_encode(const std::string& input) {
    BIO *b64 = BIO_new(BIO_f_base64());
    BIO *bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, input.data(), static_cast<int>(input.size()));
    BIO_flush(bio);
    BUF_MEM *bufferPtr;
    BIO_get_mem_ptr(bio, &bufferPtr);
    std::string result(bufferPtr->data, bufferPtr->length);
    BIO_free_all(bio);
    // OpenSSL sometimes adds a newline even with NO_NL
    result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());
    result.erase(std::remove(result.begin(), result.end(), '\r'), result.end());
    return result;
}

OAuth::OAuth(const std::string& client_id, const std::string& client_secret, const std::string& redirect_uri)
    : client_id_(client_id), client_secret_(client_secret), redirect_uri_(redirect_uri) {
    code_verifier_ = generate_code_verifier();
    code_challenge_ = generate_code_challenge(code_verifier_);
}

OAuth::~OAuth() = default;

std::string OAuth::base64url_encode(const unsigned char* data, size_t length) {
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO* bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, data, static_cast<int>(length));
    BIO_flush(bio);

    BUF_MEM* buf_mem = nullptr;
    BIO_get_mem_ptr(bio, &buf_mem);

    std::string result(buf_mem->data, buf_mem->length);
    BIO_free_all(bio);

    std::replace(result.begin(), result.end(), '+', '-');
    std::replace(result.begin(), result.end(), '/', '_');

    auto pos = result.find('=');
    if (pos != std::string::npos) {
        result.erase(pos);
    }

    return result;
}

std::string OAuth::generate_code_verifier() {
    static const char charset[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    static const size_t charset_size = sizeof(charset) - 1;

    std::string verifier;
    verifier.reserve(64);

    unsigned char buf[64];
    if (RAND_bytes(buf, 64) != 1) {
        throw std::runtime_error("Failed to generate random bytes for code verifier");
    }

    for (int i = 0; i < 64; ++i) {
        verifier += charset[buf[i] % charset_size];
    }

    return verifier;
}

std::string OAuth::generate_code_challenge(const std::string& verifier) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(verifier.c_str()),
           verifier.size(), hash);
    return base64url_encode(hash, SHA256_DIGEST_LENGTH);
}

std::string OAuth::get_auth_url() const {
    std::ostringstream oss;
    oss << AUTH_URL
        << "?client_id=" << client_id_
        << "&response_type=code"
        << "&redirect_uri=" << cpr::util::urlEncode(redirect_uri_)
        << "&code_challenge_method=S256"
        << "&code_challenge=" << code_challenge_
        << "&scope=" << cpr::util::urlEncode(SCOPES);
    return oss.str();
}

TokenResponse OAuth::exchange_code(const std::string& code) {
    auto r = cpr::Post(
        cpr::Url{TOKEN_URL},
        cpr::Payload{
            {"grant_type", "authorization_code"},
            {"code", code},
            {"redirect_uri", redirect_uri_},
            {"client_id", client_id_},
            {"client_secret", client_secret_},
            {"code_verifier", code_verifier_}
        }
    );

    TokenResponse response;
    if (r.status_code == 200) {
        auto j = nlohmann::json::parse(r.text);
        response.access_token = j.value("access_token", "");
        response.refresh_token = j.value("refresh_token", "");
        response.token_type = j.value("token_type", "");
        response.expires_in = j.value("expires_in", 0);
        response.scope = j.value("scope", "");

        access_token_ = response.access_token;
        if (!response.refresh_token.empty()) {
            refresh_token_ = response.refresh_token;
        }
        token_expiry_ = std::chrono::steady_clock::now() +
                        std::chrono::seconds(response.expires_in);
    } else {
        throw std::runtime_error("Failed to exchange code for tokens: " + r.text);
    }

    return response;
}

TokenResponse OAuth::start_auth_flow(int port) {
    httplib::Server svr;

    std::string auth_code;
    bool received_callback = false;

    svr.Get("/callback", [&](const httplib::Request& req, httplib::Response& res) {
        auto code_it = req.params.find("code");
        auto error_it = req.params.find("error");

        if (error_it != req.params.end()) {
            res.set_content(
                "<html><body><h1>Authorization Failed</h1><p>Error: " +
                error_it->second + "</p></body></html>", "text/html");
            auth_code = "";
            received_callback = true;
            return;
        }

        if (code_it != req.params.end()) {
            auth_code = code_it->second;
            res.set_content(
                "<html><body><h1>Authorization Successful</h1>"
                "<p>You can close this window now.</p></body></html>",
                "text/html");
            received_callback = true;
        } else {
            res.set_content(
                "<html><body><h1>Error</h1>"
                "<p>No authorization code received.</p></body></html>",
                "text/html");
            received_callback = true;
        }
    });

    std::thread server_thread([&svr, port]() {
        svr.listen("127.0.0.1", port);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    std::string auth_url = get_auth_url();
    platform::open_url(auth_url);

    auto start = std::chrono::steady_clock::now();
    while (!received_callback) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        auto elapsed = std::chrono::steady_clock::now() - start;
        if (elapsed > std::chrono::seconds(120)) {
            svr.stop();
            if (server_thread.joinable()) server_thread.join();
            throw std::runtime_error("Authorization timed out");
        }
    }

    svr.stop();
    if (server_thread.joinable()) server_thread.join();

    if (auth_code.empty()) {
        throw std::runtime_error("Authorization failed: no code received");
    }

    return exchange_code(auth_code);
}

TokenResponse OAuth::refresh_tokens(const std::string& refresh_token) {
    auto r = cpr::Post(
        cpr::Url{TOKEN_URL},
        cpr::Payload{
            {"grant_type", "refresh_token"},
            {"refresh_token", refresh_token},
            {"client_id", client_id_},
            {"client_secret", client_secret_}
        }
    );

    TokenResponse response;
    if (r.status_code == 200) {
        auto j = nlohmann::json::parse(r.text);
        response.access_token = j.value("access_token", "");
        response.token_type = j.value("token_type", "");
        response.expires_in = j.value("expires_in", 0);
        response.scope = j.value("scope", "");

        if (j.contains("refresh_token") && !j["refresh_token"].get<std::string>().empty()) {
            response.refresh_token = j["refresh_token"].get<std::string>();
            refresh_token_ = response.refresh_token;
        } else {
            response.refresh_token = refresh_token;
            refresh_token_ = refresh_token;
        }

        access_token_ = response.access_token;
        token_expiry_ = std::chrono::steady_clock::now() +
                        std::chrono::seconds(response.expires_in);
    } else {
        throw std::runtime_error("Failed to refresh tokens: " + r.text);
    }

    return response;
}

bool OAuth::needs_refresh() const {
    auto now = std::chrono::steady_clock::now();
    auto remaining = token_expiry_ - now;
    return remaining <= std::chrono::minutes(5);
}

}  // namespace spotify_tui
