#pragma once

#include "client/market_order_book.hpp"

namespace trading::client{
    constexpr auto Feature_INVALID = std::numeric_limits<double>::quiet_NaN();

    class FeatureEngine{
    public:
        FeatureEngine(Logger& logger) : logger_(logger) {}

        inline double getMktPrice() const noexcept{
            return mkt_price_;
        }
        
        inline double getAggTradeQtyRatio() const noexcept{
            return agg_trade_qty_ratio_;
        }

        inline void onOrderBookUpdate(TickerId ticker_id, Price price, Side side, const MarketOrderBook& order_book) noexcept{
            const auto& bbo = order_book.getBBO();
            if(bbo.bid_price_ != Price_INVALID && bbo.ask_price_ != Price_INVALID){
                mkt_price_ = ((bbo.bid_price_ * bbo.bid_qty_) + (bbo.ask_price_ * bbo.ask_qty_)) / static_cast<double>(bbo.ask_qty_ + bbo.bid_qty_); 
            }

            logger_.log("%:% %() % ticker:% price:% side:% mkt-price:% agg-trade-ratio:%\n", __FILE__, __LINE__, __FUNCTION__,
                   getCurrentTimeStr(&time_str_), ticker_id, priceToString(price).c_str(), sideToString(side).c_str(), mkt_price_, agg_trade_qty_ratio_);
        }

        inline void onTradeUpdate(const trading::exchange::MEMarketUpdate& market_update, const MarketOrderBook& order_book) noexcept{
            const auto& bbo = order_book.getBBO();
            if(bbo.bid_price_ != Price_INVALID && bbo.ask_price_ != Price_INVALID){
                agg_trade_qty_ratio_ = static_cast<double>(market_update.qty_) / (market_update.side_ == Side::BUY ? bbo.ask_qty_ : bbo.bid_qty_); 
            }

            logger_.log("%:% %() % % mkt-price:% agg-trade-ratio:%\n", __FILE__, __LINE__, __FUNCTION__,
                   getCurrentTimeStr(&time_str_), market_update.toString().c_str(), mkt_price_, agg_trade_qty_ratio_);
        }

        FeatureEngine() = delete;
        FeatureEngine(FeatureEngine& ) = delete;
        FeatureEngine(FeatureEngine&& ) = delete;
        FeatureEngine& operator=(FeatureEngine& ) = delete;
        FeatureEngine& operator=(FeatureEngine&& ) = delete;

    private:
        std::string time_str_;
        Logger& logger_;

        double mkt_price_ = Feature_INVALID;
        double agg_trade_qty_ratio_ = Feature_INVALID;
    };
}
