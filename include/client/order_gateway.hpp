#pragma once

#include <functional>

#include "utils/types.hpp"
#include "networking/tcp_socket.hpp"

#include "exchange/client_request.hpp"
#include "exchange/client_response.hpp"

namespace trading::client{

    class OrderGateway{
    public:
        OrderGateway(ClientId clientId, trading::exchange::ClientRequestLFQueue& client_requestes, trading::exchange::ClientResponseLFQueue& client_responses, std::string ip, std::string& iface, int port);
    
        ~OrderGateway(){
            stop();

            using namespace std::literals::chrono_literals;
            std::this_thread::sleep_for(5s);
        }

        inline void start(){
            run_ = true;
            ASSERT(tcp_socket_.connect(ip_, iface_, port_, false) >= 0,
             "Unable to connect to ip:" + ip_ + " port:" + std::to_string(port_) + " on iface:" + iface_ + " error:" + std::string(std::strerror(errno)));
            
            thread_ = createAndStartThread(-1, "Client Order Gateway",[this](){ run(); });
        }

        inline void stop(){
            run_ = false;
            if(thread_.joinable()) thread_.join();
        }

        OrderGateway() = delete;
        OrderGateway(OrderGateway& ) = delete;
        OrderGateway(OrderGateway&& ) = delete;
        OrderGateway& operator=(OrderGateway& ) = delete;
        OrderGateway& operator=(OrderGateway&& ) = delete;

    private:
        const ClientId client_id_;

        std::string ip_;
        const std::string iface_;
        const int port_ = 0;

        trading::exchange::ClientRequestLFQueue& outgoing_requests_;
        trading::exchange::ClientResponseLFQueue& incoming_responses_;

        std::thread thread_;
        volatile bool run_ = false;

        std::string time_str_;
        Logger logger_;

        TCPSocket tcp_socket_;

        size_t next_outgoing_seq_num_ = 1;
        size_t next_incoming_seq_num_ = 1;

        void run() noexcept;

        void recvCallback(TCPSocket *socket, Nanos rx_time) noexcept;
    };

}