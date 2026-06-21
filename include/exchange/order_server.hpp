#pragma once

#include "exchange/fifo_sequencer.hpp"
#include "exchange/client_request.hpp"
#include "exchange/client_response.hpp"

#include "networking/tcp_server.hpp"
#include "networking/tcp_socket.hpp"

namespace trading::exchange{

    class OrderServer{
    private:
        const std::string iface_;
        const int port_ = 0;

        ClientResponseLFQueue& outgoing_messages_;

        volatile bool run_ = false;

        std::string time_str_;
        Logger logger_;
        std::thread thread_;

        std::array<TCPSocket*, ME_MAX_NUM_CLIENTS> cid_to_socket_;

        /// Hash map from ClientId -> the next sequence number to be sent on outgoing client responses.
        std::array<size_t, ME_MAX_NUM_CLIENTS> cid_next_outgoing_seq_num_;
        
        /// Hash map from ClientId -> the next sequence number expected on incoming client requests.
        std::array<size_t, ME_MAX_NUM_CLIENTS> cid_next_exp_seq_num_;

        TCPServer tcp_server_;
        FifoSequencer fifo_sequencer_;

    public:
        OrderServer(ClientRequestLFQueue& client_request, ClientResponseLFQueue& client_response, const std::string& iface, const int port);

        ~OrderServer();

        void start();

        void stop();

        void run() noexcept;

        OrderServer(OrderServer& ) = delete;
        OrderServer(OrderServer&& ) = delete;
        OrderServer& operator=(OrderServer& ) = delete;
        OrderServer& operator=(OrderServer&& ) = delete;

        /// Read client request from the TCP receive buffer, check for sequence gaps and forward it to the FIFO sequencer.
        auto recvCallbacks(TCPSocket* socket, Nanos rx_time) noexcept{
            logger_.log("%:% %() % Received socket:% len:% rx:%\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_),
                  socket->fd_, socket->recv_next_valid_idx_, rx_time);
            
            const size_t requestSize = sizeof(OGClientResquest);

            if(socket->recv_next_valid_idx_ < requestSize){
                return;
            }

            size_t i{};
            for(; i + requestSize; i += requestSize){
                auto request = reinterpret_cast<const OGClientResquest*>(socket->recv_buffer_.data() + i);

                logger_.log("%:% %() % Received %\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_), request->toString());

                //If it's the first request from this client add to our array
                auto& savedSocket = cid_to_socket_[request->me_client_request_.client_id_];
                if(savedSocket == nullptr){
                    savedSocket = socket;
                } else if (savedSocket != socket) {
                    logger_.log("%:% %() % Received ClientRequest from ClientId:% on different socket:% expected:%\n", __FILE__, __LINE__, __FUNCTION__,
                            getCurrentTimeStr(&time_str_), request->me_client_request_.client_id_, socket->fd_, savedSocket->fd_);
                    continue;
                }

                //Check on the expected seq num
                auto& expected_seq_num = cid_next_exp_seq_num_[request->me_client_request_.client_id_];
                if(expected_seq_num != request->seq_num_){
                    logger_.log("%:% %() % Received ClientRequest from ClientId:% on different sequence_number:% expected:%\n", __FILE__, __LINE__, __FUNCTION__,
                            getCurrentTimeStr(&time_str_), request->me_client_request_.client_id_, request->seq_num_, expected_seq_num);
                    continue;
                }

                ++expected_seq_num;
                fifo_sequencer_.addClientRequest(rx_time, request->me_client_request_);
            }

            memmove(socket->recv_buffer_.data(), socket->recv_buffer_.data() + i, socket->recv_next_valid_idx_ - i);
            socket->recv_next_valid_idx_ -= i;
        }

        void recvFinishedCallbacks() noexcept{
            fifo_sequencer_.sequenceAndPublish();
        }
    };
}