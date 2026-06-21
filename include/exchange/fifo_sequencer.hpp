#pragma once

#include "utils/logger.hpp"
#include "exchange/client_request.hpp"

namespace trading::exchange {
    constexpr size_t MAX_PENDING_REQUESTS = 1024;
    
    class FifoSequencer{
    public:
        FifoSequencer(ClientRequestLFQueue& client_requests, Logger& logger) : incoming_request_(client_requests), logger_(logger){
        }

        void sequenceAndPublish(){
            if(pending_size_ == 0) return;

            logger_.log("%:% %() % Processing % requests.\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_), pending_size_);

            std::sort(pending_client_requests_.begin(), pending_client_requests_.begin() + pending_size_);

            for(size_t i{}; i < pending_size_; ++i){
                logger_.log("%:% %() % Writing RX:% Req:% to FIFO.\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_),
                     pending_client_requests_[i].recv_time_, pending_client_requests_[i].request_.toString());

                incoming_request_.getNextWrite() = std::move(pending_client_requests_[i].request_);
                incoming_request_.updateNextWrite();
            }

            pending_size_ = 0;
        }

        void addClientRequest(Nanos rx_time, const MEClientRequest& request){
            ASSERT(pending_size_ < pending_client_requests_.size(), "Too many pending requestes");

            pending_client_requests_[pending_size_++] = std::move(RecvTimeClientRequest{rx_time, request});
        }


    
    private:
        ClientRequestLFQueue& incoming_request_;    

        std::string time_str_;
        Logger& logger_;

        struct RecvTimeClientRequest{ //Try to change with std::pair
            Nanos recv_time_;
            MEClientRequest request_;

            auto operator<(const RecvTimeClientRequest& rhs){
                return recv_time_ < rhs.recv_time_;
            }
        };

        std::array<RecvTimeClientRequest, MAX_PENDING_REQUESTS> pending_client_requests_;
        size_t pending_size_ = 0;

    };
}