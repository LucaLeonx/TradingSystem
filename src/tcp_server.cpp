#include "tcp_server.hpp"

namespace trading{

    auto TCPServer::AddToEpollList(TCPSocket *socket){
        epoll_event ev{EPOLLET | EPOLLIN, {reinterpret_cast<void *>(socket)}};
        return !epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, socket->fd_, &ev);
    }

    void TCPServer::listen(const std::string& iface, int port){
        epoll_fd_ = epoll_create(1);
        ASSERT(epoll_fd_ >= 0, " epoll_create() failed");
        ASSERT(listener_socket_.connect("", iface, port, true) >= 0, "Failed to connect to the listener socket with iface: " + iface + " on port: " + std::to_string(port));
        ASSERT(AddToEpollList(&listener_socket_), "epoll_ctl() failed. error:" + std::string(std::strerror(errno)) );
    }

    void TCPServer::sendAndRecv() noexcept{
        auto recv = false;

        std::for_each(recv_sockets_.begin(), recv_sockets_.end(), [&recv](auto socket){ recv |= socket->sendAndRecv(); });

        if(recv && recv_finished_callback_) recv_finished_callback_();

        std::for_each(send_sockets_.begin(), send_sockets_.end(), [](auto socket){ socket->sendAndRecv(); });
    }

    void TCPServer::poll() noexcept{
        const int max_events = send_sockets_.size() + recv_sockets_.size() + 1;

        const size_t n = epoll_wait(epoll_fd_, events_.data(), max_events, 0);
        bool have_new_conn{false};

        for(size_t i{}; i < n; ++i){
            const auto& event = events_[i];
            auto socket = reinterpret_cast<TCPSocket *>(event.data.ptr);
            
            if(event.events & EPOLLIN){
                if(socket == &listener_socket_){
                    logger_.log("%:% %() % EPOLLIN listener_socket:%\n", __FILE__, __LINE__, __FUNCTION__, trading::getCurrentTimeStr(&time_str_), socket->fd_);
                    have_new_conn = true;
                    continue;
                }

                logger_.log("%:% %() % EPOLLIN socket:%\n", __FILE__, __LINE__, __FUNCTION__, trading::getCurrentTimeStr(&time_str_), socket->fd_);
                
                if(std::find(recv_sockets_.begin(), recv_sockets_.end(), socket) == recv_sockets_.end()){
                    recv_sockets_.push_back(socket);
                }
            }
            
            if(event.events & EPOLLOUT){
                logger_.log("%:% %() % EPOLLOUT socket:%\n", __FILE__, __LINE__, __FUNCTION__, trading::getCurrentTimeStr(&time_str_), socket->fd_);
                
                if(std::find(send_sockets_.begin(), send_sockets_.end(), socket) == send_sockets_.end()){
                    send_sockets_.push_back(socket);
                }
                
            }
            
            if(event.events & (EPOLLERR | EPOLLHUP) ){
                logger_.log("%:% %() % EPOLLERR socket:%\n", __FILE__, __LINE__, __FUNCTION__, trading::getCurrentTimeStr(&time_str_), socket->fd_);
                
                if(std::find(recv_sockets_.begin(), recv_sockets_.end(), socket) == recv_sockets_.end()){
                    recv_sockets_.push_back(socket);
                }
            }
        }

        while(have_new_conn){ //Accept the new connection, create a socket and add it to the list 
            logger_.log("%:% %() % Have new connection\n", __FILE__, __LINE__, __FUNCTION__, trading::getCurrentTimeStr(&time_str_));
            
            sockaddr_storage addr;
            socklen_t add_len = sizeof(addr);
            int fd = accept(listener_socket_.fd_, reinterpret_cast<sockaddr *>(&addr), &add_len);
            if(fd == -1) break;

            ASSERT(setNonBlocking(fd) && disableNagle(fd), "Failed to disable non-blocking or non-delay on socket: " + std::to_string(fd) );

            logger_.log("%:% %() % Accepted socket:%\n", __FILE__, __LINE__, __FUNCTION__, trading::getCurrentTimeStr(&time_str_), fd);

            auto socket = new TCPSocket(logger_);
            socket->fd_ = fd;
            socket->recv_callback_ = recv_callback_;
            ASSERT(AddToEpollList(socket) , "Failed to add socket: " + std::to_string(fd) + " to Epoll list");

            if(std::find(recv_sockets_.begin(), recv_sockets_.end(), socket) == recv_sockets_.end()){
                recv_sockets_.push_back(socket);
            }
        }
    }
    

}