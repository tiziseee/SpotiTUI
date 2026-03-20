#pragma once

#include <string>
#include <functional>
#include <chrono>
#include <openssl/evp.h>
#include <openssl/rand.h>

namespace spotify_tui {

struct TokenResponse {
    std::string access_token;
    std::string refresh_token;
    std::string token_type;
    int expires_in = 0;
    std::string scope;
};

class OAuth {
public:
    explicit OAuth(const std::string& client_id, const std::string& client_secret, const std::string& redirect_uri = "http://127.0.0.1:8888/callback");
    ~OAuth();

    std::string get_auth_url() const;
    TokenResponse start_auth_flow(int port = 8888);
    TokenResponse refresh_tokens(const std::string& refresh_token);

    const std::string& access_token() const { return access_token_; }
    const std::string& refresh_token() const { return refresh_token_; }
    std::chrono::steady_clock::time_point token_expiry() const { return token_expiry_; }
    bool needs_refresh() const;

private:
    std::string client_id_;
    std::string client_secret_;
    std::string redirect_uri_;
    std::string code_verifier_;
    std::string code_challenge_;
    std::string access_token_;
    std::string refresh_token_;
    std::chrono::steady_clock::time_point token_expiry_;

    std::string generate_code_verifier();
    std::string generate_code_challenge(const std::string& verifier);
    std::string base64url_encode(const unsigned char* data, size_t length);
    TokenResponse exchange_code(const std::string& code);
};

} // namespace spotify_tui
