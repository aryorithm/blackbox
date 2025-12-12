/**
 * @file tcp_server.cpp
 * @brief Implementation of TCP Ingestion.
 */

#include "blackbox/ingest/tcp_server.h"
#include "blackbox/common/logger.h"
#include "blackbox/common/metrics.h"
#include "blackbox/ingest/rate_limiter.h"
#include <iostream>
#include <string_view>

namespace blackbox::ingest {

    // =========================================================
    // TCP SERVER Implementation
    // =========================================================

    TcpServer::TcpServer(boost::asio::io_context& io_context,
                         uint16_t port,
                         RingBuffer<65536>& buffer)
        : io_context_(io_context),
          acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
          ring_buffer_(buffer)
    {
        LOG_INFO("TCP Server listening on port: " + std::to_string(port));
        start_accept();
    }

    TcpServer::~TcpServer() {
        // Acceptor closes automatically
    }

    void TcpServer::start_accept() {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket) {
                if (!ec) {
                    // Check Rate Limit (Connection Throttling)
                    std::string ip = socket.remote_endpoint().address().to_string();
                    if (!RateLimiter::instance().should_allow(ip)) {
                        LOG_WARN("TCP Connection rejected (Rate Limit): " + ip);
                        socket.close();
                    } else {
                        // Create Session and Start
                        std::make_shared<TcpSession>(std::move(socket), ring_buffer_)->start();
                    }
                } else {
                    LOG_ERROR("TCP Accept Error: " + ec.message());
                }

                // Loop
                start_accept();
            }
        );
    }

    // =========================================================
    // TCP SESSION Implementation
    // =========================================================

    TcpSession::TcpSession(tcp::socket socket, RingBuffer<65536>& buffer)
        : socket_(std::move(socket)), ring_buffer_(buffer)
    {
        // Reserve memory for sticky buffer to prevent reallocs
        sticky_buffer_.reserve(4096);
    }

    void TcpSession::start() {
        do_read();
    }

    void TcpSession::do_read() {
        auto self(shared_from_this()); // Keep session alive

        socket_.async_read_some(boost::asio::buffer(data_, max_length),
            [this, self](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    // Process Data
                    common::Metrics::instance().inc_packets_received(1); // Count chunks
                    process_buffer(length);
                    do_read(); // Continue reading
                }
                else if (ec != boost::asio::error::eof && ec != boost::asio::error::operation_aborted) {
                    // Log error (EOF is normal disconnect)
                    LOG_WARN("TCP Read Error: " + ec.message());
                }
            }
        );
    }

    void TcpSession::process_buffer(size_t bytes_transferred) {
        // Create a view over the new data
        std::string_view chunk(data_, bytes_transferred);

        size_t start_pos = 0;

        // Find newlines
        while (true) {
            size_t newline_pos = chunk.find('\n', start_pos);

            if (newline_pos == std::string_view::npos) {
                // No more newlines.
                // Append remaining data to sticky buffer for next packet.
                sticky_buffer_.append(chunk.substr(start_pos));

                // Safety: Prevent memory exhaustion if client never sends \n
                if (sticky_buffer_.size() > 8192) {
                    LOG_WARN("TCP message too large without newline. Dropping buffer.");
                    sticky_buffer_.clear();
                }
                break;
            }

            // We found a complete message
            size_t msg_len = newline_pos - start_pos;

            // 1. Combine sticky buffer + current chunk part
            // Optimization: If sticky_buffer is empty, we can zero-copy push directly from 'data_'
            if (sticky_buffer_.empty()) {
                // Direct Push
                const char* log_ptr = data_ + start_pos;
                if (!ring_buffer_.push(log_ptr, msg_len)) {
                    common::Metrics::instance().inc_packets_dropped(1);
                }
            } else {
                // Stitch together
                sticky_buffer_.append(chunk.substr(start_pos, msg_len));

                if (!ring_buffer_.push(sticky_buffer_.data(), sticky_buffer_.size())) {
                    common::Metrics::instance().inc_packets_dropped(1);
                }

                sticky_buffer_.clear();
            }

            // Move cursor past the newline
            start_pos = newline_pos + 1;
        }
    }

} // namespace blackbox::ingest