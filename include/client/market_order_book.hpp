#pragma once

#include "utils/logger.hpp"
#include "utils/mem_pool.hpp"

#include "client/market_order.hpp"
#include "client/trade_engine.hpp"
#include "exchange/market_data.hpp"

namespace trading::client{
    class TradeEngine;

    class MarketOrderBook {
    public:
        MarketOrderBook(TickerId ticker, Logger logger) : ticker_id(ticker), orders_at_price_pool_(ME_MAX_PRICE_LEVELS), order_pool_(ME_MAX_ORDER_IDS), logger_(logger) {}

        ~MarketOrderBook(){
            logger_.log("%:% %() % OrderBook\n%\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_), toString(false, true));

            trade_engine_ = nullptr;
            bids_by_price_ = asks_by_price_ = nullptr;
            oid_to_order_.fill(nullptr);
        }

        inline void setTradeEngine(TradeEngine *tradeEngine){ trade_engine_ = tradeEngine; }

        std::string toString(bool detailed, bool validity_check) const;

        inline BBO& getBBO() noexcept { return bbo_; }
        inline const BBO& getBBO() const noexcept { return bbo_; }

        MarketOrderBook() = delete;
        MarketOrderBook(MarketOrderBook &&) = delete;
        MarketOrderBook(MarketOrderBook &) = delete;
        MarketOrderBook& operator=(MarketOrderBook&&) = delete;
        MarketOrderBook& operator=(MarketOrderBook&) = delete;

    private:
        const TickerId ticker_id;

        TradeEngine* trade_engine_ = nullptr;

        OrderHashMap oid_to_order_;

        MemPool<MarketOrdersAtPrice> orders_at_price_pool_;
        MarketOrdersAtPrice *bids_by_price_ = nullptr;
        MarketOrdersAtPrice *asks_by_price_ = nullptr;
        
        OrderAtPriceHashmap oid_to_price_level_;

        MemPool<MarketOrder> order_pool_;

        BBO bbo_;

        std::string time_str_;
        Logger& logger_;

    };
    
    using MarketOrderBookHashMap = std::array<MarketOrder*, ME_MAX_TICKERS>;
}