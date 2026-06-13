#pragma once

#include "tcp_socket.hpp"
#include <algorithm>

namespace trading{
    class TCPServer{
    private:
        ///Add and remove socket file descriptor to and from the EPOLL list
        auto AddToEpollList(TCPSocket *tcpsocket);

    public:
        Logger &logger_;
        std::string time_str_;
        
        int epoll_fd_{-1};
        TCPSocket listener_socket_;

        std::array<epoll_event, 1024> events_{};

        std::vector<TCPSocket*> send_sockets_, recv_sockets_;

        /// Function wrapper to call back when data is available.
        std::function<void(TCPSocket *s, Nanos rx_time)> recv_callback_ = nullptr;

        /// Function wrapper to call back when all data across all TCPSockets has been read and dispatched this round.
        std::function<void()> recv_finished_callback_ = nullptr;


        explicit TCPServer(Logger &log) : logger_(log), listener_socket_(log){
        } 

        ///Start listening for connection on the provided interface and port
        void listen(const std::string& iface, int port);

        ///Publish outgoing data to the send buffer and ingoing data to the receive buffer
        void sendAndRecv() noexcept;

        ///Check for new or dead connections and updates the related data structures managing them
        void poll() noexcept;
    };

}