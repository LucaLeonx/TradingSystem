#pragma once

#include <sstream>
#include <array>

#include "utils/types.hpp"

namespace trading::client{
    struct MarketOrder{
        OrderId order_id_ = OrderId_INVALID;
        Side side_ = Side::INVALID;
        Price price_ = Price_INVALID;
        Qty qty_ = Qty_INVALID;
        Priority priority_ = Priority_INVALID;

        MarketOrder *prev_order_ = nullptr;
        MarketOrder *next_order_ = nullptr;

        MarketOrder() = default;

        MarketOrder(OrderId order, Side side, Price price, Qty qty, Priority priority, MarketOrder *prev, MarketOrder *next)
            : order_id_(order), side_(side), price_(price), qty_(qty), priority_(priority), prev_order_(prev), next_order_(next){}

        inline std::string toString() const{
            std::stringstream ss;
            ss << "MEOrder"
               << " ["
               << " orderId:" << orderIdToString(order_id_)
               << " side:" << sideToString(side_)
               << " price:" << priceToString(price_)
               << " qty:" << qtyToString(qty_)
               << " priority:" << priorityToString(priority_);
            ss << "]";

            return ss.str();
        }

    };

    struct MarketOrdersAtPrice {
        Side side_ = Side::INVALID;
        Price price_ = Price_INVALID;

        MarketOrder *first_order_ = nullptr;

        MarketOrdersAtPrice *prev_ = nullptr;
        MarketOrdersAtPrice *next_ = nullptr;
    
        MarketOrdersAtPrice() = default;

        MarketOrdersAtPrice(Side side, Price price, MarketOrder *market_order_head, MarketOrdersAtPrice *prev, MarketOrdersAtPrice *next)
            : side_(side), price_(price), first_order_(market_order_head), prev_(prev), next_(next) {}

        
        inline std::string toString() const{
            std::stringstream ss;
            ss << "MEOrder"
               << " ["
               << " side: " << sideToString(side_)
               << " price: " << priceToString(price_)
               << " First MarketOrder: " << first_order_ ? first_order_->toString() : "None";
            ss << "]";

            return ss.str();
        }
    };

    using OrderHashMap = std::array<MarketOrder*, ME_MAX_ORDER_IDS>;
    using OrderAtPriceHashmap = std::array<MarketOrdersAtPrice*, ME_MAX_PRICE_LEVELS>;

    /// @brief Best Bid Offer
    struct BBO{
        Price bid_price_ = Price_INVALID, ask_price_ = Price_INVALID;
        Qty bid_qty_ = Qty_INVALID, ask_qty_ = Qty_INVALID;
        
        inline std::string toString() const{
            std::stringstream ss;
            ss << "Best Bid Offer: "
               << qtyToString(bid_qty_) << "@" << priceToString(bid_price_)
               << " AND "
               << qtyToString(ask_qty_) << "@" << priceToString(ask_price_);

            return ss.str();
        }
    };

}