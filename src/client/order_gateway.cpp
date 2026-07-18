#include "client/order_gateway.hpp"

namespace trading::client{
    OrderGateway::OrderGateway(ClientId clientId, trading::exchange::ClientRequestLFQueue& client_requestes, trading::exchange::ClientResponseLFQueue& client_responses,
                                std::string ip, std::string& iface, int port)
                                : client_id_(client_id_), outgoing_requests_(client_requestes), incoming_responses_(client_responses), 
                                  logger_("ClientOrderGateway" + clientIdToString(client_id_) + ".log"), tcp_socket_(logger_)
    {
        tcp_socket_.recv_callback_ = [this](auto socket, auto rx_time){ recvCallback(socket, rx_time); };
    }

    void OrderGateway::run() noexcept{
        logger_.log("%:% %() %\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_));

        while(run_){
            tcp_socket_.sendAndRecv();

            for(auto client_request = outgoing_requests_.getNextRead(); client_request; client_request = outgoing_requests_.getNextRead()){
                tcp_socket_.send(&next_outgoing_seq_num_, sizeof(next_outgoing_seq_num_));
                tcp_socket_.send(client_request, sizeof(trading::exchange::MEClientRequest));
                outgoing_requests_.updateNextRead();

                next_outgoing_seq_num_++;
            }
        }
    }

    /// Callback when an incoming client response is read, we perform some checks and forward it to the lock free queue connected to the trade engine.
    void OrderGateway::recvCallback(TCPSocket *socket, Nanos rx_time) noexcept{
        logger_.log("%:% %() % Received socket:% len:% %\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_), socket->fd_, socket->send_next_valid_idx_, rx_time);

        size_t OGresponse_size = sizeof(trading::exchange::OGClientResponse);
        if(socket->recv_next_valid_idx_ >= OGresponse_size){
            size_t i{};
            for(; i + OGresponse_size <= socket->recv_next_valid_idx_; i += OGresponse_size){
                auto response = reinterpret_cast<const trading::exchange::OGClientResponse *>(socket->recv_buffer_.data() + i);
                logger_.log("%:% %() % Received %\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_), response->toString());

                if(response->me_client_response_.client_id_ != client_id_){
                    logger_.log("%:% %() % ERROR Incorrect client id. ClientId expected:% received:%.\n", __FILE__, __LINE__, __FUNCTION__,
                        getCurrentTimeStr(&time_str_), client_id_, response->me_client_response_.client_id_);
                    continue;
                }

                if(response->seq_num_ != next_incoming_seq_num_){
                    logger_.log("%:% %() % ERROR Incorrect sequence number. Expected:% received:%.\n", __FILE__, __LINE__, __FUNCTION__,
                        getCurrentTimeStr(&time_str_), next_incoming_seq_num_, response->seq_num_);
                    continue;
                }

                ++next_incoming_seq_num_;
                incoming_responses_.getNextWrite() = response->me_client_response_;
                incoming_responses_.updateNextWrite();
            }

            memmove(socket->recv_buffer_.data(), socket->recv_buffer_.data() + 1, socket->recv_next_valid_idx_ - i);
            socket->recv_next_valid_idx_ -= i;
        }
    }
                                

}