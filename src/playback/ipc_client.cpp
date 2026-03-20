#include "spotify_tui/playback/ipc_client.hpp"

#include <chrono>
#include <iostream>
#include <thread>

namespace spotify_tui {

IpcClient::IpcClient(int port)
    : port_(port),
      context_(1),
      socket_(context_, zmq::socket_type::req) {}

IpcClient::~IpcClient() {
    disconnect();
}

bool IpcClient::connect() {
    if (connected_) {
        return true;
    }

    try {
        std::string addr = "tcp://127.0.0.1:" + std::to_string(port_);
        socket_.connect(addr);
        connected_ = true;
        return true;
    } catch (const zmq::error_t& e) {
        std::cerr << "ZMQ connect error: " << e.what() << std::endl;
        return false;
    }
}

void IpcClient::disconnect() {
    if (!connected_) {
        return;
    }

    try {
        socket_.close();
    } catch (...) {}

    connected_ = false;
}

bool IpcClient::send_command(const json& cmd) {
    if (!connected_) {
        return false;
    }

    try {
        std::string data = cmd.dump();
        zmq::message_t msg(data.size());
        std::memcpy(msg.data(), data.data(), data.size());
        socket_.send(msg, zmq::send_flags::none);
        return true;
    } catch (const zmq::error_t& e) {
        std::cerr << "ZMQ send error: " << e.what() << std::endl;
        return false;
    }
}

json IpcClient::receive_event(int timeout_ms) {
    if (!connected_) {
        return json{{"error", "not connected"}};
    }

    try {
        socket_.set(zmq::sockopt::rcvtimeo, timeout_ms);

        zmq::message_t msg;
        auto result = socket_.recv(msg, zmq::recv_flags::none);

        if (!result) {
            return json{{"error", "timeout"}};
        }

        std::string data(static_cast<char*>(msg.data()), msg.size());
        return json::parse(data);
    } catch (const zmq::error_t& e) {
        if (e.num() == EAGAIN) {
            return json{{"error", "timeout"}};
        }
        std::cerr << "ZMQ receive error: " << e.what() << std::endl;
        return json{{"error", e.what()}};
    } catch (const json::parse_error& e) {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
        return json{{"error", "parse error"}};
    }
}

} // namespace spotify_tui
