#pragma once

#include "utils/types.hpp"
#include "utils/logger.hpp"

#include "client/order_manager.hpp"
#include "client/feature_engine.hpp"

namespace trading::client {
    class MarketMaker {
    public:
    
        MarketMaker(Logger& logger, TradeEngine& trade_engine, const FeatureEngine& feature_engine, OrderManager& order_manager, const TradeEngineCfgHashMap& ticker_cfg);

        inline void onOrderBookUpdate(TickerId ticker, Price price, Side side, const MarketOrderBook& book) noexcept{
            logger_.log("%:% %() % ticker:% price:% side:%\n", __FILE__, __LINE__, __FUNCTION__,
                   getCurrentTimeStr(&time_str_), ticker, priceToString(price).c_str(), sideToString(side).c_str());

            const auto bbo = book.getBBO();
            const auto fair_price = feature_engine_.getMktPrice();

            if(bbo.bid_price_ != Price_INVALID && bbo.ask_price_ != Price_INVALID && fair_price != Feature_INVALID){
                logger_.log("%:% %() % % fair-price:%\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_), bbo.toString().c_str(), fair_price);

                const auto clip = ticker_cfg_[ticker].clip_;
                const auto threshold = ticker_cfg_[ticker].threshold_;

                const auto bid_price = bbo.bid_price_ - (fair_price - bbo.bid_price_ >= threshold ? 0 : 1);
                const auto ask_price = bbo.ask_price_ + (bbo.ask_price_ - fair_price >= threshold ? 0 : 1);

                START_MEASURE(Trading_Ordermanager_moveOrders);
                order_manager_.moveOrders(ticker, bid_price, ask_price, clip);
                END_MEASURE(Trading_Ordermanager_moveOrders, logger_);
            }
        }
        
        inline void onTradeUpdate(const trading::exchange::MEMarketUpdate& market_update, const MarketOrderBook& ) noexcept {
            logger_.log("%:% %() % %\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_), market_update.toString().c_str());
        }

        inline void onOrderUpdate(const trading::exchange::MEClientResponse& client_response) noexcept {
            logger_.log("%:% %() % %\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_), client_response.toString().c_str());

            START_MEASURE(Trading_Ordermanager_onOrderUpdate);
            order_manager_.onOrderUpdate(client_response);
            END_MEASURE(Trading_Ordermanager_onOrderUpdate, logger_);
        }

        MarketMaker() = delete;
        MarketMaker(MarketMaker& ) = delete;
        MarketMaker(MarketMaker&& ) = delete;
        MarketMaker& operator=(MarketMaker& ) = delete;
        MarketMaker& operator=(MarketMaker&& ) = delete;

    private:
        const FeatureEngine& feature_engine_;

        OrderManager& order_manager_;

        std::string time_str_;
        Logger& logger_;

        const TradeEngineCfgHashMap ticker_cfg_;
    };
}