#pragma once

#include "utils/mem_pool.hpp"
#include "utils/logger.hpp"

#include "exchange/market_data.hpp"
#include "exchange/me_order.hpp"
#include "exchange/client_response.hpp"


namespace trading::exchange{

    class MatchingEngine;

    class MEOrderBook{
        private:
        TickerId ticker_ = TickerId_INVALID;

        MatchingEngine& matching_engine_;

        //Hashmap that based on an ClientId and OrderId gives you the a reference to the Order object (MEOrder)
        ClientOrderHashMap cid_oid_to_order_{};  

        MemPool<MEOrdersAtPrice> orders_at_price_pool_;
        MEOrdersAtPrice* bids_by_price_ = nullptr;
        MEOrdersAtPrice* asks_by_price_ = nullptr;

        OrderAtPriceHashMap price_to_orders_at_price_{};

        MemPool<MEOrder> orders_pool_;

        OrderId next_order_id_{1};
        
        std::string time_str_;
        Logger& logger_;

        /// @brief  Evaluate the next orderId
        /// @return  The current available orderId
        inline OrderId generateNextOrderId() noexcept{
            return next_order_id_++;
        }

        inline Price priceToIndex(const Price p) const noexcept{
            return p % ME_MAX_PRICE_LEVELS; 
        }

        inline MEOrdersAtPrice* getOrderAtPrice(Price p) const {
            return price_to_orders_at_price_.at(priceToIndex(p));
        }

        inline Priority getPriority(Price p){
            const auto orderLevel = getOrderAtPrice(p);
            if(!orderLevel){
                return 1;
            }

            return orderLevel->orders_head_->prev_order_->priority_ + 1;
        }

        Qty checkForMatch(ClientId clientId, OrderId client_orderId, Side side, Price price, Qty qty, OrderId market_order_id);

        void match(ClientId clientId, OrderId client_orderId, OrderId market_orderId, Side side, MEOrder* order, Qty& left) noexcept;

        void AddOrderToPriceLevel(MEOrder* orderObj) noexcept;

        void AddOrderAtPrice(MEOrdersAtPrice* orderAtPriceObj) noexcept;

        void removeOrder(MEOrder* orderObj) noexcept;

        void removeOrderAtPrice(MEOrdersAtPrice* orderObj) noexcept;

    public:
        explicit MEOrderBook(TickerId ticker, Logger& logger, MatchingEngine& matching_engine);

        ~MEOrderBook();

        MEOrderBook(MEOrderBook&) = delete;
        MEOrderBook(MEOrderBook&&) = delete;
        MEOrderBook& operator=(MEOrderBook&) = delete;
        MEOrderBook& operator=(MEOrderBook&&) = delete;

        void add(ClientId clientId, OrderId client_orderId, Side side, Price price, Qty qty) noexcept;
        
        void cancel(ClientId clientId, OrderId orderId) noexcept;

        std::string toString(bool detailed, bool validity_check) const;
    };

    using OrderBookHashMap = std::array<std::unique_ptr<MEOrderBook>, ME_MAX_TICKERS>;
}