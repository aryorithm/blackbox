/**
 * @file admin_server.cpp
 * @brief Implementation of the Ops Server.
 */

#include "blackbox/core/admin_server.h"
#include "blackbox/common/logger.h"
#include "blackbox/common/metrics.h" // To fetch stats
#include <iostream>
#include <sstream>

namespace blackbox::core {

    // =========================================================
    // Constructor
    // =========================================================
    AdminServer::AdminServer(short port) : port_(port) {
        // Prepare context
        io_context_ = std::make_shared<boost::asio::io_context>();
        
        // Prepare acceptor (Listener)
        acceptor_ = std::make_unique<tcp::acceptor>(
            *io_context_, 
            tcp::endpoint(tcp::v4(), port)
        );
        
        LOG_INFO("Admin Server configured on port " + std::to_string(port));
    }

    AdminServer::~AdminServer() {
        stop();
    }

    // =========================================================
    // Lifecycle
    // =========================================================
    void AdminServer::start() {
        if (running_) return;
        running_ = true;

        // Queue first connection
        start_accept();

        // Spin up thread
        worker_thread_ = std::thread(&AdminServer::run_worker, this);
        LOG_INFO("Admin Server started (Background).");
    }

    void AdminServer::stop() {
        if (!running_) return;
        running_ = false;
        
        io_context_->stop();
        if (worker_thread_.joinable()) {
            worker_thread_.join();
        }
    }

    void AdminServer::run_worker() {
        try {
            io_context_->run();
        } catch (const std::exception& e) {
            LOG_ERROR("Admin Server crashed: " + std::string(e.what()));
        }
    }

    // =========================================================
    // Async Accept
    // =========================================================
    void AdminServer::start_accept() {
        auto socket = std::make_shared<tcp::socket>(*io_context_);
        
        acceptor_->async_accept(*socket, [this, socket](const boost::system::error_code& error) {
            if (!error) {
                // Handle the connection
                // For simplicity in this Admin module, we spawn a transient thread 
                // or just handle it. Since ops traffic is low volume, std::thread is fine.
                std::thread(&AdminServer::handle_session, this, socket).detach();
            }
            // Continue listening
            if (running_) start_accept();
        });
    }

    // =========================================================
    // Request Handler
    // =========================================================
    void AdminServer::handle_session(std::shared_ptr<tcp::socket> socket) {
        try {
            char data[1024];
            boost::system::error_code error;
            size_t length = socket->read_some(boost::asio::buffer(data), error);

            if (error == boost::asio::error::eof) return; // Connection closed
            else if (error) return; // Other error

            // 1. Parse Request Line (Very basic)
            // Expect: "GET /health HTTP/1.1"
            std::string request(data, length);
            std::string path;
            
            std::istringstream iss(request);
            std::string method;
            iss >> method >> path;

            // 2. Route
            std::string body;
            std::string status = "200 OK";

            if (method == "GET") {
                body = generate_response(path);
                if (body.empty()) {
                    status = "404 Not Found";
                    body = "404 Page Not Found";
                }
            } else {
                status = "405 Method Not Allowed";
            }

            // 3. Construct HTTP Response
            std::ostringstream response;
            response << "HTTP/1.1 " << status << "\r\n";
            response << "Content-Length: " << body.size() << "\r\n";
            response << "Content-Type: text/plain\r\n";
            response << "Connection: close\r\n";
            response << "\r\n";
            response << body;

            // 4. Send
            boost::asio::write(*socket, boost::asio::buffer(response.str()));

        } catch (const std::exception& e) {
            LOG_ERROR("Admin Session Error: " + std::string(e.what()));
        }
    }

    // =========================================================
    // Logic (The "Controller")
    // =========================================================
    std::string AdminServer::generate_response(const std::string& path) {
        if (path == "/health") {
            // Liveness probe
            // In a real app, check if RingBuffer is full or DB is down
            return "OK";
        } 
        else if (path == "/metrics") {
            // Dump atomic stats in Prometheus format
            // We can't access Metrics privates directly, so we assume Metrics class 
            // has a dump() method or we access the counters via getters.
            // For MVP, we'll just return a simple text summary.
            
            // NOTE: Ideally, Metrics.h should have a 'get_snapshot()' method.
            // Here we return a static string for demonstration.
            return "# HELP blackbox_status Status of the engine\n"
                   "# TYPE blackbox_status gauge\n"
                   "blackbox_status 1\n";
        }
        return "";
    }

} // namespace blackbox::core