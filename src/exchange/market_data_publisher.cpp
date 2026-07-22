#include "exchange/market_data_publisher.hpp"

namespace trading::exchange {

    MarketDataPublisher::MarketDataPublisher(MEMarketUpdateLFQueue& market_data_queue, const std::string& iface, 
                    const std::string snapshot_ip, const int snapshot_port, const std::string incremental_ip, const int incremental_port)
                    : outgoing_market_data_(market_data_queue), snapshot_updates_(ME_MAX_MARKET_UPDATES), logger_("Market data publisher.log"), incremental_socket_(logger_),
                         snapshot_synthetizer_(snapshot_updates_, iface, snapshot_ip, snapshot_port)
    {
        ASSERT(incremental_socket_.init(incremental_ip, iface, incremental_port, false) >= 0, "Unable to create incremental mcast");
    }

    MarketDataPublisher::~MarketDataPublisher(){
        stop();
    
        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(5s);
    }

    void MarketDataPublisher::run() noexcept{
        logger_.log("%:% %() %\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_));

        while(run_){

            for(auto market_update = outgoing_market_data_.getNextRead();outgoing_market_data_.size() > 0 && market_update != nullptr; market_update = outgoing_market_data_.getNextRead()){
                logger_.log("%:% %() % Sending seq:% %\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_), next_seq_num_, market_update->toString().c_str());

                incremental_socket_.send(&next_seq_num_, sizeof(next_seq_num_));
                incremental_socket_.send(market_update, sizeof(*market_update));
                outgoing_market_data_.updateNextRead();

                auto& next_write = snapshot_updates_.getNextWrite();
                next_write.seq_num_ = next_seq_num_;
                next_write.me_market_update_ = *market_update;
                snapshot_updates_.updateNextWrite();

                ++next_seq_num_;
            }

            incremental_socket_.sendAndRecv();
        }
    }
}