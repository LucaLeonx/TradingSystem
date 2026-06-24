#pragma once

#include <map>

#include "networking/mcast_socket.hpp"

#include "utils/logger.hpp"
#include "utils/types.hpp"

#include "exchange/market_data.hpp"

namespace trading::client {

    class MarketDataConsumer{
    private:
        size_t exp_seq_num_ = 1;

        trading::exchange::MEMarketUpdateLFQueue& incoming_market_update_queue_;

        std::string time_str_;
        Logger logger_;

        std::thread thread_;

        McastSocket incremental_socket_, snapshot_socket_;

        volatile bool run_ = false;

        bool packet_lost_ = false;
        const std::string iface_, snapshot_ip_;
        const int snapshot_port_;
        using QueuedMarketUpdate = std::map<size_t, trading::exchange::MEMarketUpdate>; //Best design? Red and black trees, O(log(n)) insertion 

        QueuedMarketUpdate snapshot_queue_message_, incremental_queue_message_;

        void run() noexcept;

        void recvCallbacks(McastSocket* socket) noexcept;

        void startSnapshotSync();

        void queueMessage(const bool is_snapshot, const trading::exchange::MDPMarketUpdate* message);

        void checkSnapshotSync();

    public:
        MarketDataConsumer(ClientId clientId, trading::exchange::MEMarketUpdateLFQueue& market_update_queue, const std::string& iface, 
                            const std::string snapshot_ip, const int snapshot_port, const std::string incremental_ip, const int incremental_port);

        ~MarketDataConsumer();

        void start();

        void stop();

        MarketDataConsumer(MarketDataConsumer& ) = delete; 
        MarketDataConsumer(MarketDataConsumer&& ) = delete; 
        MarketDataConsumer& operator=(MarketDataConsumer& ) = delete; 
        MarketDataConsumer& operator=(MarketDataConsumer&& ) = delete; 

    };
}