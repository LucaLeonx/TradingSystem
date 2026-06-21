#pragma once

#include "networking/mcast_socket.hpp"

#include "exchange/market_data.hpp"
#include "exchange/snapshot_synthetizer.hpp"

#include "utils/logger.hpp"

namespace trading::exchange {

    class MarketDataPublisher{
    private:
        size_t next_seq_num_ = 1;
        
        MEMarketUpdateLFQueue& outgoing_market_data_;

        MDPMarketUpdateLFQueue snapshot_updates_;

        std::thread thread_;
        std::string time_str_;
        Logger logger_;

        volatile bool run_ = false;

        McastSocket incremental_socket_;
        SnapshotSynthetizer snapshot_synthetizer_;    
    public:
        MarketDataPublisher(MEMarketUpdateLFQueue& market_data_queue, const std::string& iface, 
                    const std::string snapshot_ip, const int snapshot_port, const std::string incremental_ip, const int incremental_port);
    
        ~MarketDataPublisher();

        void start(){
            run_ = true;

            thread_ = createAndStartThread(-1, "MarketDataPublisher", [this](){run();} );

            snapshot_synthetizer_.start();
        }

        void stop(){
            run_ = false;

            if(thread_.joinable()) thread_.join();
            snapshot_synthetizer_.stop();
        }

        void run() noexcept;

        MarketDataPublisher(MarketDataPublisher& ) = delete;
        MarketDataPublisher(MarketDataPublisher&& ) = delete;
        MarketDataPublisher& operator=(MarketDataPublisher& ) = delete;
        MarketDataPublisher& operator=(MarketDataPublisher&& ) = delete;
    };

}