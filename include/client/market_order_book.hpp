#pragma once

#include "utils/logger.hpp"
#include "utils/mem_pool.hpp"

#include "client/market_order.hpp"
#include "exchange/market_data.hpp"

namespace trading::client{
    class TradeEngine;

    class MarketOrderBook {
    public:
        MarketOrderBook(TickerId ticker, Logger& logger) : ticker_id_(ticker), orders_at_price_pool_(ME_MAX_PRICE_LEVELS), order_pool_(ME_MAX_ORDER_IDS), logger_(logger) {}

        ~MarketOrderBook();

        inline void setTradeEngine(TradeEngine *tradeEngine){ trade_engine_ = tradeEngine; }

        void onMarketUpdate(const trading::exchange::MEMarketUpdate& market_update) noexcept;

        void updateBBO(bool update_bid, bool update_ask) noexcept;

        std::string toString(bool detailed, bool validity_check) const;

        inline BBO& getBBO() noexcept { return bbo_; }
        inline const BBO& getBBO() const noexcept { return bbo_; }

        MarketOrderBook() = delete;
        MarketOrderBook(MarketOrderBook &&) = delete;
        MarketOrderBook(MarketOrderBook &) = delete;
        MarketOrderBook& operator=(MarketOrderBook&&) = delete;
        MarketOrderBook& operator=(MarketOrderBook&) = delete;

    private:
        const TickerId ticker_id_;

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

        inline auto priceToIndex(Price price) const noexcept {
            return (price % ME_MAX_PRICE_LEVELS);
        }

        inline MarketOrdersAtPrice * getOrdersAtPrice(Price price) const noexcept {
            return oid_to_price_level_.at(priceToIndex(price));
        }

        void AddOrder(MarketOrder* orderObj) noexcept;

        void AddOrderAtPrice(MarketOrdersAtPrice* orderAtPriceObj) noexcept;

        void removeOrder(MarketOrder* orderObj) noexcept;

        void removeOrderAtPrice(MarketOrdersAtPrice* orderObj) noexcept;

    };
    
}