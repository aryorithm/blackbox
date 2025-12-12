/**
 * @file udp_server.h
 * @brief High-Performance Asynchronous UDP Listener.
 * 
 * Capabilities:
 * - Zero-Allocation Receive Loop
 * - Integrated DDoS Protection (Rate Limiting)
 * - Atomic Metrics Tracking
 */

#ifndef BLACKBOX_INGEST_UDP_SERVER_H
#define BLACKBOX_INGEST_UDP_SERVER_H

#include <boost/asio.hpp>
#include <array>
#include <memory>
#include "blackbox/ingest/ring_buffer.h"

namespace blackbox::ingest {

    using boost::asio::ip::udp;

    class UdpServer {
    public:
        /**
         * @brief Construct a new Udp Server.
         * 
         * @param io_context The Boost.Asio event loop.
         * @param buffer Reference to the shared RingBuffer.
         */
        UdpServer(boost::asio::io_context& io_context, 
                  RingBuffer<65536>& buffer);

        // Disable copying
        UdpServer(const UdpServer&) = delete;
        UdpServer& operator=(const UdpServer&) = delete;

    private:
        /**
         * @brief Initiates the async receive operation.
         */
        void start_receive();

        /**
         * @brief Callback when a packet arrives.
         * 
         * 1. Checks Rate Limit.
         * 2. Updates Metrics.
         * 3. Pushes to RingBuffer.
         */
        void handle_receive(const boost::system::error_code& error,
                            std::size_t bytes_transferred);

        // Network Resources
        udp::socket socket_;
        udp::endpoint remote_endpoint_;
        
        // Destination Buffer (Reference)
        RingBuffer<65536>& ring_buffer_;

        // Scratchpad memory (Max UDP packet size)
        std::array<char, 65507> recv_buffer_; 
    };

} // namespace blackbox::ingest

#endif // BLACKBOX_INGEST_UDP_SERVER_H