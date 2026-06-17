#include "networking/tcp_server.hpp"

namespace trading{

    bool TCPServer::AddToEpollList(const int fd){
        epoll_event ev{};
        ev.events =  EPOLLET | EPOLLIN;
        ev.data.u32 = static_cast<uint32_t>(fd); 

        return epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev) == 0;
    }

    void TCPServer::listen(const std::string& iface, int port){
        epoll_fd_ = epoll_create(1);
        ASSERT(epoll_fd_ >= 0, " epoll_create() failed");
        ASSERT(listener_socket_.connect("", iface, port, true) >= 0, "Failed to connect to the listener socket with iface: " + iface + " on port: " + std::to_string(port));
        ASSERT(AddToEpollList(listener_socket_.fd_), "epoll_ctl() failed. error:" + std::string(std::strerror(errno)) );
    }

    void TCPServer::sendAndRecv() noexcept{
        auto recv = false;

        for(const int fileDesc : recv_sockets_){
            if(all_sockets_.find(fileDesc) != all_sockets_.end()){
                recv |= all_sockets_[fileDesc]->sendAndRecv();
            }
        }

        if(recv && recv_finished_callback_) recv_finished_callback_();

        for(const int fileDesc : send_sockets_){
            if(all_sockets_.find(fileDesc) != all_sockets_.end()){
                all_sockets_[fileDesc]->sendAndRecv();
            }
        }
    }

    void TCPServer::poll() noexcept{
        const int max_events = static_cast<int>(std::min(events_.size() ,send_sockets_.size() + recv_sockets_.size() + 1));

        const size_t n = epoll_wait(epoll_fd_, events_.data(), max_events, 0);
        bool have_new_conn{false};

        for(size_t i{}; i < n; ++i){
            const auto& event = events_[i];
            int event_fd = static_cast<int>(event.data.u32);

            if(event.events & EPOLLIN){
                if(event_fd == listener_socket_.fd_){
                    logger_.log("%:% %() % EPOLLIN listener_socket:%\n", __FILE__, __LINE__, __FUNCTION__, trading::getCurrentTimeStr(&time_str_), event_fd);
                    have_new_conn = true;
                    continue;
                }

                logger_.log("%:% %() % EPOLLIN socket:%\n", __FILE__, __LINE__, __FUNCTION__, trading::getCurrentTimeStr(&time_str_), event_fd);
                recv_sockets_.insert(event_fd);
            }
            
            if(event.events & EPOLLOUT){
                logger_.log("%:% %() % EPOLLOUT socket:%\n", __FILE__, __LINE__, __FUNCTION__, trading::getCurrentTimeStr(&time_str_), event_fd);
                send_sockets_.insert(event_fd);
                
            }
            
            if(event.events & (EPOLLERR | EPOLLHUP) ){
                logger_.log("%:% %() % EPOLLERR socket:%\n", __FILE__, __LINE__, __FUNCTION__, trading::getCurrentTimeStr(&time_str_), event_fd);
                recv_sockets_.insert(event_fd);
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

            auto socket = std::make_unique<TCPSocket>(logger_);
            socket->fd_ = fd;
            socket->recv_callback_ = recv_callback_;
            all_sockets_.emplace(fd, std::move(socket));

            ASSERT(AddToEpollList(fd) , "Failed to add socket: " + std::to_string(fd) + " to Epoll list");

            recv_sockets_.insert(fd);
        }
    }
    

}