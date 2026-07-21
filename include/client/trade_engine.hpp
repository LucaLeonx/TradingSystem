#pragma once 

#include <functional>
#include <memory>

#include "exchange/market_data.hpp"
#include "exchange/client_response.hpp"
#include "exchange/client_request.hpp"

#include "client/market_order_book.hpp"
#include "client/order_manager.hpp"
#include "client/feature_engine.hpp"
#include "client/position_keeper.hpp"
#include "client/market_maker.hpp"
#include "client/liquidity_taker.hpp"

namespace trading::client {
    using MarketOrderBookHashMap = std::array<std::unique_ptr<MarketOrderBook>, ME_MAX_TICKERS>;


    class TradeEngine{
    public:
        TradeEngine(ClientId clientId, AlgoType algo_type, const TradeEngineCfgHashMap& ticker_cfg, 
                    trading::exchange::ClientRequestLFQueue& client_requests, trading::exchange::ClientResponseLFQueue& client_responses, trading::exchange::MEMarketUpdateLFQueue& market_updates);

        ~TradeEngine();

        inline void start(){
            run_ = true;
            thread_ = createAndStartThread(-1, "Trading/TradeEngine", [this](){ run(); });
        }

        inline void stop(){
            while(!incoming_responses_.empty() || !incoming_market_updates_.empty()){
                logger_.log("%:% %() % Sleeping till all updates are consumed ogw-size:% md-size:%\n", __FILE__, __LINE__, __FUNCTION__,
                            getCurrentTimeStr(&time_str_), incoming_market_updates_.size(), incoming_responses_.size());

                using namespace std::literals::chrono_literals;
                std::this_thread::sleep_for(10ms);
            }

            if(thread_.joinable())
                thread_.join();

            logger_.log("%:% %() % POSITIONS\n%\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_), position_keeper_.toString());
        
            run_ = false;
        }

        void run() noexcept;

        ///Process the changes on the order book, updates PositionKeeper, FeatureEnginge and informs the trading algorithm about the update 
        void onOrderBookUpdate(TickerId ticker, Price price, Side side, MarketOrderBook* order_book) noexcept; 
        
        ///Process trade events, updates FeatureEngine
        void onTradeUpdate(const trading::exchange::MEMarketUpdate& market_update, MarketOrderBook* order_book) noexcept;

        ///Write a client request to the lock-free queue in common with the order server to send to the exchange
        void SendClientRequest(const trading::exchange::MEClientRequest& client_request) noexcept;

        ///Process client responses, updates the PositionKeeper and informs trading algorithm
        void onOrderUpdate(const trading::exchange::MEClientResponse& client_response) noexcept;

        inline ClientId clientId() const noexcept{ 
            return client_id_;
        }

        inline void initLastEventTime() {
            last_event_time_ = getCurrentNanos();
        }

        inline auto silentSeconds() const{
            return (getCurrentNanos() - last_event_time_) / NANOS_TO_SECS;
        }

        std::function<void(TickerId, Price, Side,MarketOrderBook& )> algoOnOrderBookUpdate_;
        std::function<void(const trading::exchange::MEMarketUpdate&, const MarketOrderBook&)> algoOnTradeUpdate_;
        std::function<void(const trading::exchange::MEClientResponse&)> algoOnOrderUpdate_;
    
        TradeEngine() = delete;
        TradeEngine(TradeEngine& ) = delete;
        TradeEngine(TradeEngine&& ) = delete;
        TradeEngine operator=(TradeEngine& ) = delete;
        TradeEngine operator=(TradeEngine&& ) = delete;

    private:
        const ClientId client_id_;
        
        MarketOrderBookHashMap ticker_order_book_;

        trading::exchange::ClientRequestLFQueue& outgoing_requests_; 
        trading::exchange::ClientResponseLFQueue& incoming_responses_;
        trading::exchange::MEMarketUpdateLFQueue& incoming_market_updates_;
        
        volatile bool run_ = false;
        std::thread thread_;

        Nanos last_event_time_;

        std::string time_str_;
        Logger logger_;

        FeatureEngine feature_engine_;
        PositionKeeper position_keeper_;
        RiskManager risk_manager_;
        OrderManager order_manager_;

        //One of the two will be created
        std::unique_ptr<MarketMaker> mm_algo_;
        std::unique_ptr<LiquidityTaker> taker_algo_;

        /// Default methods to initialize the function wrappers.
        auto defaultAlgoOnOrderBookUpdate(TickerId ticker_id, Price price, Side side, MarketOrderBook& ) noexcept -> void {
            logger_.log("%:% %() % ticker:% price:% side:%\n", __FILE__, __LINE__, __FUNCTION__,
                        getCurrentTimeStr(&time_str_), ticker_id, priceToString(price).c_str(), sideToString(side).c_str());
        }

        auto defaultAlgoOnTradeUpdate(const trading::exchange::MEMarketUpdate& market_update,const  MarketOrderBook& ) noexcept -> void {
            logger_.log("%:% %() % %\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_),
                        market_update.toString().c_str());
        }

        auto defaultAlgoOnOrderUpdate(const trading::exchange::MEClientResponse& client_response) noexcept -> void {
            logger_.log("%:% %() % %\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_),
                        client_response.toString().c_str());
        }

    };
}