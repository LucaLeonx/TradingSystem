#pragma once

#include "utils/types.hpp"
#include "utils/logger.hpp"

#include "client/order_manager.hpp"
#include "client/feature_engine.hpp"

namespace trading::client {
    class LiquidityTaker {
    public:
    
        LiquidityTaker(Logger& logger, TradeEngine& trade_engine, const FeatureEngine& feature_engine, OrderManager& order_manager, const TradeEngineCfgHashMap& ticker_cfg);

        inline void onOrderBookUpdate(TickerId ticker, Price price, Side side, const MarketOrderBook& ) noexcept{
           logger_.log("%:% %() % ticker:% price:% side:%\n", __FILE__, __LINE__, __FUNCTION__,
                   getCurrentTimeStr(&time_str_), ticker, priceToString(price).c_str(), sideToString(side).c_str());
        }
        
        inline void onTradeUpdate(const trading::exchange::MEMarketUpdate& market_update, const MarketOrderBook& book) noexcept {
            logger_.log("%:% %() % %\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_), market_update.toString().c_str());

            const auto bbo = book.getBBO();
            const auto agg_qty_ratio = feature_engine_.getAggTradeQtyRatio();

            if(bbo.ask_price_ != Price_INVALID && bbo.bid_price_ != Price_INVALID && agg_qty_ratio != Feature_INVALID){
                logger_.log("%:% %() % % agg-qty_ratio:%\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_), bbo.toString().c_str(), agg_qty_ratio);

                const auto clip = ticker_cfg_[market_update.ticker_id_].clip_;
                const auto threshold = ticker_cfg_[market_update.ticker_id_].threshold_;

                if(agg_qty_ratio >= threshold){
                    if(market_update.side_ == Side::BUY){
                        order_manager_.moveOrders(market_update.ticker_id_, bbo.ask_price_, Price_INVALID, clip);
                    } else {
                        order_manager_.moveOrders(market_update.ticker_id_, Price_INVALID, bbo.bid_price_, clip);
                    }
                }
            }
        }

        inline void onOrderUpdate(const trading::exchange::MEClientResponse& client_response) noexcept {
            logger_.log("%:% %() % %\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_), client_response.toString().c_str());

            order_manager_.onOrderUpdate(client_response);
        }

        LiquidityTaker() = delete;
        LiquidityTaker(LiquidityTaker& ) = delete;
        LiquidityTaker(LiquidityTaker&& ) = delete;
        LiquidityTaker& operator=(LiquidityTaker& ) = delete;
        LiquidityTaker& operator=(LiquidityTaker&& ) = delete;

    private:
        const FeatureEngine& feature_engine_;

        OrderManager& order_manager_;

        std::string time_str_;
        Logger& logger_;

        const TradeEngineCfgHashMap ticker_cfg_;
    };
}