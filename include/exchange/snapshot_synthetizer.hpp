#pragma once

#include "exchange/market_data.hpp"

#include "networking/mcast_socket.hpp"

#include "utils/mem_pool.hpp"
#include "utils/logger.hpp"

namespace trading::exchange{

    class SnapshotSynthetizer{
    private:
        MDPMarketUpdateLFQueue& market_updates_;

        std::string time_str_;
        Logger logger_;

        volatile bool run_ = false;
        std::thread thread_;

        McastSocket snapshot_socket_;

        std::array<std::array<MEMarketUpdate*, ME_MAX_ORDER_IDS>, ME_MAX_TICKERS> ticker_order_to_update_;
        size_t last_seq_num_ = 0;
        Nanos last_snap_update_ = 0;

        MemPool<MEMarketUpdate> update_pool_;
    
    public:
        SnapshotSynthetizer(MDPMarketUpdateLFQueue& market_data_queue, const std::string& iface, const std::string& snapshot_ip, const int snapshot_port);
    
        ~SnapshotSynthetizer();

        void start();

        void stop();

        void addSnapShot(const MDPMarketUpdate& mdp_market_update);

        void publishSnapShot();

        void run();

        SnapshotSynthetizer(SnapshotSynthetizer& ) = delete;
        SnapshotSynthetizer(SnapshotSynthetizer&& ) = delete;
        SnapshotSynthetizer& operator=(SnapshotSynthetizer& ) = delete;
        SnapshotSynthetizer& operator=(SnapshotSynthetizer&& ) = delete;
    };
}