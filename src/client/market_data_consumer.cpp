#include "client/market_data_consumer.hpp"

namespace trading::client {
    MarketDataConsumer::MarketDataConsumer(ClientId clientId, trading::exchange::MEMarketUpdateLFQueue& market_update_queue, const std::string& iface, 
                            const std::string snapshot_ip, const int snapshot_port, const std::string incremental_ip, const int incremental_port)
        :   incoming_market_update_queue_(market_update_queue) , logger_("data_consumer_" + clientIdToString(clientId) + ".log"), incremental_socket_(logger_), snapshot_socket_(logger_),
                                iface_(iface), snapshot_ip_(snapshot_ip), snapshot_port_(snapshot_port) 
    {
        auto recvCallback = [this](auto socket){
            recvCallbacks(socket);
        };
        incremental_socket_.recv_callback_ = recvCallback;


        ASSERT(incremental_socket_.init(incremental_ip, iface, incremental_port, /*is_listening*/ true) >= 0,
                "Unable to create incremental mcast socket. error:" + std::string(std::strerror(errno)));

        ASSERT(incremental_socket_.join(incremental_ip),
                "Join failed on:" + std::to_string(incremental_socket_.socket_fd_) + " error:" + std::string(std::strerror(errno)));

        snapshot_socket_.recv_callback_ = recvCallback;

    }

    MarketDataConsumer::~MarketDataConsumer(){
        stop();

        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(5s);
    }

    void MarketDataConsumer::start(){
        run_ = true;

        thread_ = createAndStartThread(-1, "Market Data Publisher", [this](){ run(); });
    }

    void MarketDataConsumer::stop(){
        run_ = false;

        if(thread_.joinable()){
            thread_.join();
        }

    }

    void MarketDataConsumer::run() noexcept{
        logger_.log("%:% %() %\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_));

        while(run_){
            incremental_socket_.sendAndRecv();
            snapshot_socket_.sendAndRecv();
        }
    }

    void MarketDataConsumer::recvCallbacks(McastSocket* socket) noexcept{
        const bool is_snapshot = socket->socket_fd_ == snapshot_socket_.socket_fd_;
        if(!packet_lost_ && is_snapshot){
            socket->next_rcv_valid_index_ = 0;

            logger_.log("%:% %() % WARN Not expecting snapshot messages.\n",
                  __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_));

            return;
        }

        size_t i{};
        for(; i + sizeof(trading::exchange::MDPMarketUpdate) <= socket->next_rcv_valid_index_; i += sizeof(trading::exchange::MDPMarketUpdate)){
            auto message = reinterpret_cast<trading::exchange::MDPMarketUpdate*>(socket->inbound_data_.data() + i);

            logger_.log("%:% %() % Received % socket len:% %\n", __FILE__, __LINE__, __FUNCTION__,
                    getCurrentTimeStr(&time_str_), (is_snapshot ? "snapshot" : "incremental"), sizeof(trading::exchange::MDPMarketUpdate), message->toString());

            const bool already_in_recovery = packet_lost_;
            packet_lost_ = (already_in_recovery || message->seq_num_ != exp_seq_num_);
            
            if(packet_lost_){
                if(!already_in_recovery){
                    logger_.log("%:% %() % Packet drops on % socket. SeqNum expected:% received:%\n", __FILE__, __LINE__, __FUNCTION__,
                        getCurrentTimeStr(&time_str_), (is_snapshot ? "snapshot" : "incremental"), exp_seq_num_, message->seq_num_);
            
                    startSnapshotSync();
                }

                queueMessage(is_snapshot, message);
            } else if(!is_snapshot){
                logger_.log("%:% %() % %\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_), message->toString());

                ++exp_seq_num_;

                incoming_market_update_queue_.getNextWrite() = message->me_market_update_;
                incoming_market_update_queue_.updateNextWrite();
            }

            memmove(socket->inbound_data_.data(), socket->inbound_data_.data() + 1, socket->next_rcv_valid_index_ - i);
            socket->next_rcv_valid_index_ -= i;
        }

    }

    void MarketDataConsumer::startSnapshotSync(){
        snapshot_queue_message_.clear();
        incremental_queue_message_.clear();

        ASSERT(snapshot_socket_.init(snapshot_ip_, iface_, snapshot_port_, /*is_listening*/ true) >= 0,
              "Unable to create snapshot mcast socket. error:" + std::string(std::strerror(errno)));
        ASSERT(snapshot_socket_.join(snapshot_ip_), // IGMP multicast subscription.
              "Join failed on:" + std::to_string(snapshot_socket_.socket_fd_) + " error:" + std::string(std::strerror(errno)));
    }

    void MarketDataConsumer::queueMessage(const bool is_snapshot, const trading::exchange::MDPMarketUpdate* message){
        if(is_snapshot){
            if(snapshot_queue_message_.find(message->seq_num_) != snapshot_queue_message_.end()){
                snapshot_queue_message_.clear();
                logger_.log("%:% %() % Packet drops on snapshot socket. Received for a 2nd time:%\n", __FILE__, __LINE__, __FUNCTION__,
                    getCurrentTimeStr(&time_str_), message->toString());
            }
            snapshot_queue_message_[message->seq_num_] = message->me_market_update_;
        } else {
            incremental_queue_message_[message->seq_num_] = message->me_market_update_;
        }

        logger_.log("%:% %() % size snapshot:% incremental:% % => %\n", __FILE__, __LINE__, __FUNCTION__,
                getCurrentTimeStr(&time_str_), snapshot_queue_message_.size(), incremental_queue_message_.size(), message->seq_num_, message->toString());

        checkSnapshotSync();
    }

    /// Check if a recovery / synchronization is possible from the queued up market data updates from the snapshot and incremental market data streams.
    void MarketDataConsumer::checkSnapshotSync(){
        if(snapshot_queue_message_.empty()){
            return;
        }

        const auto& first_mess = snapshot_queue_message_.begin()->second;
        if(first_mess.type_ != trading::exchange::MarketUpdateType::SNAPSHOT_START){   //check starting of the message
            snapshot_queue_message_.clear();
            return;
        }

        std::vector<trading::exchange::MEMarketUpdate> joint_queue{};

        size_t next_seq_num = 0;
        for(const auto& mess : snapshot_queue_message_){ //Check for gaps in the sequence number, would mean a lost message
            if(mess.first != next_seq_num){
                snapshot_queue_message_.clear();
                return;
            }
            if(mess.second.type_ != trading::exchange::MarketUpdateType::SNAPSHOT_START && mess.second.type_ != trading::exchange::MarketUpdateType::SNAPSHOT_END){
                joint_queue.push_back(mess.second);
            }

            ++next_seq_num;
        }

        const auto& last_mess = snapshot_queue_message_.rbegin()->second;
        if(last_mess.type_ != trading::exchange::MarketUpdateType::SNAPSHOT_END){   //check ending of the message
            return;
        }

        exp_seq_num_ = last_mess.order_id_ + 1; //For the SNAPSHOT_END message the orderId is the seq_num of the last element from the incremental stream included in the snapshot   
        for(const auto& mess : incremental_queue_message_){
            if(mess.first < exp_seq_num_){ 
                continue;
            }

            if(mess.first != exp_seq_num_){
                logger_.log("%:% %() % Detected gap in incremental stream expected:% found:% %.\n", __FILE__, __LINE__, __FUNCTION__,
                    getCurrentTimeStr(&time_str_), exp_seq_num_, mess.first, mess.second.toString());
                snapshot_queue_message_.clear();
                return;
            }

            if(mess.second.type_ != trading::exchange::MarketUpdateType::SNAPSHOT_START && mess.second.type_ != trading::exchange::MarketUpdateType::SNAPSHOT_END){
                joint_queue.push_back(mess.second);
            }
    
            ++exp_seq_num_;
        }

        //Successfully recovered, now push everything into the LF queue
        for(const auto el : joint_queue){
            incoming_market_update_queue_.getNextWrite() = el;
            incoming_market_update_queue_.updateNextWrite();
        }

        packet_lost_ = false;
        snapshot_queue_message_.clear();
        incremental_queue_message_.clear();

        snapshot_socket_.leave(snapshot_ip_, snapshot_port_);

    }

}