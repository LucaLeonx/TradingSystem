#include "exchange/snapshot_synthetizer.hpp"

namespace trading::exchange {
 
    SnapshotSynthetizer::SnapshotSynthetizer(MDPMarketUpdateLFQueue& market_data_queue, const std::string& iface, const std::string& snapshot_ip, const int snapshot_port)
     : market_updates_(market_data_queue), logger_("SnapShot_Synthetizer.log"), snapshot_socket_(logger_), update_pool_(ME_MAX_ORDER_IDS) 
    {
        ASSERT(snapshot_socket_.init(snapshot_ip, iface, snapshot_port, false) >= 0, "Unable to create mcast snapshot synthetizer");
        for(auto& tickers : ticker_order_to_update_){
            tickers.fill(nullptr);
        }
    }

    SnapshotSynthetizer::~SnapshotSynthetizer(){
        stop();
    }

    void SnapshotSynthetizer::start(){
        run_ = true;

        thread_ = createAndStartThread(-1, "SnapShot Synthetizer", [this](){ run();} );
    }

    void SnapshotSynthetizer::stop(){
        run_ = false;

        if(thread_.joinable())
            thread_.join();
    }

    void SnapshotSynthetizer::addSnapShot(const MDPMarketUpdate& mdp_market_update){
        auto const& market_update = mdp_market_update.me_market_update_;
        auto *orders = &ticker_order_to_update_.at(market_update.ticker_id_);
    
        switch(market_update.type_){
            case MarketUpdateType::ADD :{
                auto order = orders->at(market_update.order_id_);
                ASSERT(order == nullptr, "Received:" + market_update.toString() + " but order already exists:" + (order ? order->toString() : ""));
                orders->at(market_update.order_id_) = update_pool_.allocate(market_update);
                break;
            }   
            case MarketUpdateType::MODIFY :{
                auto order = orders->at(market_update.order_id_);
                ASSERT(order != nullptr, "Received:" + market_update.toString() + " but order does not exist.");
                ASSERT(order->order_id_ == market_update.order_id_, "Expecting existing order to match new one.");
                ASSERT(order->side_ == market_update.side_, "Expecting existing order to match new one.");

                order->price_ = market_update.price_;
                order->qty_ = market_update.qty_;
                break;
            }
            case MarketUpdateType::CANCEL :{
                auto order = orders->at(market_update.order_id_);
                ASSERT(order != nullptr, "Received:" + market_update.toString() + " but order does not exist.");
                ASSERT(order->order_id_ == market_update.order_id_, "Expecting existing order to match new one.");
                ASSERT(order->side_ == market_update.side_, "Expecting existing order to match new one.");

                update_pool_.deallocate(order);
                orders->at(market_update.order_id_) = nullptr;    
            }
            case MarketUpdateType::SNAPSHOT_START:
            case MarketUpdateType::CLEAR:
            case MarketUpdateType::SNAPSHOT_END:
            case MarketUpdateType::TRADE:
            case MarketUpdateType::INVALID:
            break;
        }

        ASSERT(mdp_market_update.seq_num_ == last_seq_num_ + 1, "Expected incremental seq_nums to increase.");
        last_seq_num_ = mdp_market_update.seq_num_;
    }

    void SnapshotSynthetizer::publishSnapShot(){
        size_t snap_size = 0;

        auto snap_start = MDPMarketUpdate{snap_size++, MEMarketUpdate{MarketUpdateType::SNAPSHOT_START, last_seq_num_}};
        snapshot_socket_.send(&snap_start, sizeof(snap_start));

        for(TickerId ticker = 0; ticker < ticker_order_to_update_.size(); ticker++){
            auto& orders = ticker_order_to_update_[ticker];
            
            MEMarketUpdate me_update{};
            me_update.ticker_id_ = ticker;
            me_update.type_ = MarketUpdateType::CLEAR;
            auto ticker_clear = MDPMarketUpdate{snap_size++, me_update};
            snapshot_socket_.send(&ticker_clear, sizeof(ticker_clear));

            for(const auto order : orders){
                if(order){
                    MDPMarketUpdate update = {snap_size++, *order};
                    snapshot_socket_.send(&update, sizeof(update));
                    snapshot_socket_.sendAndRecv();
                }
            }
            
        }
 
        auto snap_end = MDPMarketUpdate{snap_size++, MEMarketUpdate{MarketUpdateType::SNAPSHOT_END, last_seq_num_}};
        snapshot_socket_.send(&snap_end, sizeof(snap_end));
        snapshot_socket_.sendAndRecv();

        logger_.log("%:% %() % Published snapshot of % orders.\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_), snap_size - 1);
    }


    void SnapshotSynthetizer::run(){
        logger_.log("%:% %() %\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_));

        while(run_){
            for(auto update = market_updates_.getNextRead(); update != nullptr && market_updates_.size() > 0; update = market_updates_.getNextRead()){
                logger_.log("%:% %() % Processing %\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_), update->toString().c_str());

                addSnapShot(*update);

                market_updates_.updateNextRead();
            }

            if(getCurrentNanos() - last_snap_update_ > 60 * NANOS_TO_SECS){
                last_snap_update_ = getCurrentNanos();
                publishSnapShot();
            }
        }
    }
}