#pragma once

#include <iostream>
#include <unordered_set>
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <fcntl.h>  

#include "utils/macros.hpp"
#include "utils/logger.hpp"

namespace trading {
    constexpr int MaxTCPServerBacklog = 1024;

    struct SocketConfigs{
        std::string ip_;
        std::string iface_;
        int port_ = -1;
        bool is_udp_ = false;
        bool is_listening_ = false;
        bool needs_so_timestamp_ =  false;

        auto toString() const {
          std::stringstream ss;
          ss << "SocketCfg[ip:" << ip_
          << " iface:" << iface_
          << " port:" << port_
          << " is_udp:" << is_udp_
          << " is_listening:" << is_listening_
          << " needs_SO_timestamp:" << needs_so_timestamp_
          << "]";

          return ss.str();
        }
    };

    /// @brief Translate network interfaces in a better format
    /// @param iface 
    /// @return 
    inline auto getIfaceIP(const std::string &iface) -> std::string{
        char buff[NI_MAXHOST] = {'\0'};
        ifaddrs *ifaddr = nullptr;
        
        if(getifaddrs(&ifaddr) != -1){
            for(ifaddrs *ifa = ifaddr; ifa; ifa = ifa->ifa_next){
                if(ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET && ifa->ifa_name == iface){
                    getnameinfo(ifa->ifa_addr, sizeof(sockaddr_in), buff, sizeof(buff), NULL, 0, NI_NUMERICHOST);
                    break;
                }
            }
            freeifaddrs(ifaddr);
        }

        return buff;
    }
    
    ///Set the socket corresponding to the fd file descriptor to non-blocking
    inline auto setNonBlocking(int fd) -> bool{
        const auto flags = fcntl(fd, F_GETFL, 0);

        if(flags == -1){
            return false;
        }
        if(flags & O_NONBLOCK){
            return true;
        }
        return fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1;
    }

    ///Disable Nagle's algorithm for TCP sockets
    inline auto disableNagle(int fd) -> bool{
        int one = 1;
        return (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<void *>(&one), sizeof(one)) == 0);
    }

    ///Check whenevere socket operation would block or not 
    inline auto wouldBlock() -> bool{
        return (errno == EWOULDBLOCK || errno == EINPROGRESS);
    }

    ///Generate software timestamps when network packets hit the network socket
    inline auto setSOTimestamp(int fd) -> bool{
        int one = 1;
        return setsockopt(fd, SOL_SOCKET , SO_TIMESTAMP, reinterpret_cast<void*>(&one), sizeof(one)) == 0;
    }
    
    ///Set Time-To-Live for non-multicast sockets
    inline auto setTTL(int fd, int ttl) -> bool{
        return setsockopt(fd, IPPROTO_IP, IP_TTL, reinterpret_cast<void*>(&ttl), sizeof(ttl)) == 0;
    }

    ///Set Time-To-Live for multicast sockets
    inline auto setMcastTTL(int fd, int ttl) -> bool{
        return setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, reinterpret_cast<void*>(&ttl), sizeof(ttl)) == 0;
    }
    
    /// Create a TCP / UDP socket to either connect to or listen for data on or listen for connections on the specified interface and IP:port information.
    [[nodiscard]] inline auto createSocket(Logger &logger, const SocketConfigs& socket_cfg) -> int{
        std::string time_str{};

        const auto ip = socket_cfg.ip_.empty() ? getIfaceIP(socket_cfg.iface_) : socket_cfg.ip_;
        logger.log("%:% %() % cfg:%\n", __FILE__, __LINE__, __FUNCTION__,
               trading::getCurrentTimeStr(&time_str), socket_cfg.toString());
        
        const int input_flags = (socket_cfg.is_listening_ ? AI_PASSIVE : 0) | (AI_NUMERICHOST | AI_NUMERICSERV);
        const addrinfo hints{input_flags, AF_INET, socket_cfg.is_udp_ ? SOCK_DGRAM : SOCK_STREAM, socket_cfg.is_udp_ ? IPPROTO_UDP : IPPROTO_TCP, 0, 0, nullptr, nullptr};
        
        addrinfo* results = nullptr;
        const auto rc = getaddrinfo(ip.c_str(), std::to_string(socket_cfg.port_).c_str(), &hints, &results);
        ASSERT(!rc, "getaddrinfo() failed, error:" + std::string(gai_strerror(rc)));
        
        int socket_fd = -1;
        int one = 1;

        for(addrinfo *rp = results; rp; rp = rp->ai_next){
            ASSERT((socket_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) != -1,"socket() failed. errno:" + std::string(strerror(errno))); 

            ASSERT(setNonBlocking(socket_fd), "Set non-blocking failed, errno: " + std::string(strerror(errno)));

            if(!socket_cfg.is_udp_){
                ASSERT(disableNagle(socket_fd), "Failed to disable the Nagle's algorithm");
            }

            if (!socket_cfg.is_listening_) { // establish connection to specified address.
                ASSERT(connect(socket_fd, rp->ai_addr, rp->ai_addrlen) != 1, "connect() failed. errno:" + std::string(strerror(errno)));
            }

            if (socket_cfg.is_listening_) { // allow re-using the address in the call to bind()
                ASSERT(setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&one), sizeof(one)) == 0, 
                        "setsockopt() SO_REUSEADDR failed. errno:" + std::string(strerror(errno)));
            }
            
            if(socket_cfg.is_listening_){
                const sockaddr_in addr{AF_INET, htons(socket_cfg.port_), {htonl(INADDR_ANY)}, {}}; 
                ASSERT(bind(socket_fd, socket_cfg.is_udp_ ? reinterpret_cast<const struct sockaddr *>(&addr) : rp->ai_addr, sizeof(addr)) == 0,
                             "bind() failed. errno:%" + std::string(strerror(errno)));
            }
            
            if (!socket_cfg.is_udp_ && socket_cfg.is_listening_) { // listen for incoming TCP connections.
                ASSERT(listen(socket_fd, MaxTCPServerBacklog) == 0, "listen() failed. errno:" + std::string(strerror(errno)));
            }
        
            if (socket_cfg.needs_so_timestamp_) { // enable software receive timestamps.
                ASSERT(setSOTimestamp(socket_fd), "setSOTimestamp() failed. errno: " + std::string(strerror(errno)));
            }

        }

        freeaddrinfo(results);
        return socket_fd;            
    }    

    /// Add / Join membership / subscription to the multicast stream specified and on the interface specified.
    inline auto join(int fd, const std::string &ip) -> bool {
        const ip_mreq mreq{{inet_addr(ip.c_str())}, {htonl(INADDR_ANY)}};
        return (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) != -1);
    }


}