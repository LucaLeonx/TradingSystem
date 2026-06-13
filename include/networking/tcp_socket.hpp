#pragma once

#include <functional>

#include "socket_utils.hpp"
#include "logger.hpp"

namespace trading{
    constexpr size_t TCPBufferSize = 64 * 1024 * 1024;

    struct TCPSocket {
        explicit TCPSocket(Logger &log) : logger_(log){
            send_buffer_.resize(TCPBufferSize);
            recv_buffer_.resize(TCPBufferSize);
        }

        TCPSocket() = delete;
        TCPSocket(TCPSocket& sock) = delete;
        TCPSocket(TCPSocket&& sock) = delete;
        TCPSocket& operator=(TCPSocket& sock) = delete;
        TCPSocket& operator=(TCPSocket&& sock) = delete;

        int fd_ = -1;

        std::vector<char> send_buffer_;
        size_t send_next_valid_idx_{};
        std::vector<char> recv_buffer_;
        size_t recv_next_valid_idx_{};

        bool send_disconnected_{false};
        bool recv_disconnected_{false};

        struct sockaddr_in socket_attribute_{};

        std::function<void(TCPSocket *s,Nanos rx_time)> recv_callback_ = nullptr;

        std::string time_str_;
        Logger &logger_;

        //Create TCP socket with attribute to lister-on or connect-to
        auto connect(const std::string &ip, const std::string &iface, int port, bool is_listening) -> int;

        ///Write outgoing data to the send buffer
        auto send(const void* data, size_t len) noexcept -> void;
        
        //Publish data from the send buffer and check if data is available in the receive buffer
        auto sendAndRecv() noexcept -> bool;

        ~TCPSocket(){
            if(fd_ != -1) close(fd_);
        }
    };

}