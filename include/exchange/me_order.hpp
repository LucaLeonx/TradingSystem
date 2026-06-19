#pragma once

#include <memory>
#include <sstream>
#include <list>

#include "utils/types.hpp"

namespace trading::exchange{
    struct MEOrder{
        TickerId ticker_id_ = TickerId_INVALID;
        ClientId client_id_ = ClientId_INVALID;
        OrderId client_order_id_ = OrderId_INVALID;
        OrderId market_order_id_ = OrderId_INVALID;
        Side side_ = Side::INVALID;
        Price price_ = Price_INVALID;
        Qty qty_ = Qty_INVALID;
        Priority priority_ = Priority_INVALID;

        MEOrder* next_order_ = nullptr;
        MEOrder* prev_order_ = nullptr;

        MEOrder(
            TickerId ticker_id, ClientId client_id,
            OrderId client_order_id, OrderId market_order_id,
            Side side, Price price, Qty qty, Priority priority,
            MEOrder* next_ptr,  MEOrder* prev_ptr) noexcept
            :  ticker_id_(ticker_id),
               client_id_(client_id),
               client_order_id_(client_order_id),
               market_order_id_(market_order_id),
               side_(side),
               price_(price),
               qty_(qty),
               priority_(priority),
               next_order_(next_ptr),
               prev_order_(prev_ptr)
        {}

        MEOrder() = default; //Defaule constructor for the memory pool

        inline auto toString() const {
            std::stringstream ss;
            ss << "MEOrder"
               << " ["
               << "ticker:" << tickerIdToString(ticker_id_)
               << " client:" << clientIdToString(client_id_)
               << " client_oid:" << orderIdToString(client_order_id_)
               << " market_oid:" << orderIdToString(market_order_id_)
               << " side:" << sideToString(side_)
               << " price:" << priceToString(price_)
               << " qty:" << qtyToString(qty_)
               << " priority:" << priorityToString(priority_);
            ss << "]";

            return ss.str();
        }

    };

    struct MEOrdersAtPrice{
        Side side_ = Side::INVALID;
        Price price_ = Price_INVALID;
        
        MEOrder* orders_head_ = nullptr;

        MEOrdersAtPrice* next_ = nullptr;
        MEOrdersAtPrice* prev_ = nullptr;


        MEOrdersAtPrice(Side s, Price p, MEOrder *first_order, MEOrdersAtPrice* next, MEOrdersAtPrice* prev) 
            : side_(s), price_(p), orders_head_(first_order), next_(next), prev_(prev) {} 

        MEOrdersAtPrice() = default; //Defaule constructor for the memory pool

        inline auto toString() const {
            std::stringstream ss;
            ss << "MEOrdersAtPrice"
               << " ["
               << "side:" << sideToString(side_)
               << " price:" << priceToString(price_)
               << " orders:" << orders_head_->toString()
               << "]";

            return ss.str();
        }
    };

    using OrderHashMap = std::array<MEOrder*, ME_MAX_ORDER_IDS>;
    using ClientOrderHashMap = std::array<OrderHashMap, ME_MAX_NUM_CLIENTS>;
    
    using OrderAtPriceHashMap = std::array<MEOrdersAtPrice*, ME_MAX_PRICE_LEVELS>;
}