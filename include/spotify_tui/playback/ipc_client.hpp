#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include <zmq.hpp>

namespace spotify_tui {

using json = nlohmann::json;

class IpcClient {
public:
    explicit IpcClient(int port = 5555);
    ~IpcClient();

    bool connect();
    void disconnect();
    bool send_command(const json& cmd);
    json receive_event(int timeout_ms = 1000);

private:
    int port_;
    zmq::context_t context_;
    zmq::socket_t socket_;
    bool connected_ = false;
};

} // namespace spotify_tui
