#include "shipper/tcp_client.h"
#include <iostream>

namespace blackbox::sentry {

    TcpClient::TcpClient(boost::asio::io_context& io, const std::string& ip, uint16_t port, const std::string& id)
        : io_context_(io), socket_(io), resolver_(io), server_ip_(ip), port_(port), agent_id_(id) {}

    void TcpClient::connect() {
        do_connect();
    }

    void TcpClient::do_connect() {
        auto endpoints = resolver_.resolve(server_ip_, std::to_string(port_));

        boost::asio::async_connect(socket_, endpoints,
            [this](boost::system::error_code ec, boost::asio::ip::tcp::endpoint) {
                if (!ec) {
                    std::cout << "[SENTRY] Connected to Core at " << server_ip_ << std::endl;
                    connected_ = true;

                    // Handshake: Send Agent ID immediately
                    std::string handshake = "HELLO AGENT_ID=" + agent_id_ + "\n";
                    send_log(handshake);
                } else {
                    // Retry logic would go here
                    std::cerr << "[SENTRY] Connect failed: " << ec.message() << std::endl;
                    connected_ = false;
                }
            });
    }

    void TcpClient::send_log(const std::string& log_line) {
        if (!connected_) return;

        // In a real agent, we need a queue to handle bursts
        // For simple async writing:
        auto msg = std::make_shared<std::string>(log_line + "\n");

        boost::asio::async_write(socket_, boost::asio::buffer(*msg),
            [msg](boost::system::error_code ec, std::size_t /*length*/) {
                if (ec) {
                    std::cerr << "[SENTRY] Write failed" << std::endl;
                }
            });
    }

}