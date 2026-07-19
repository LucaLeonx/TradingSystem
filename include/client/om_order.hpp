#pragma once

#include <array>
#include <sstream>

#include <utils/types.hpp>

namespace trading::client {
    enum class OMOrderState : int8_t {
        INVALID = 0,
        PENDING_NEW = 1,
        LIVE = 2,
        PENDING_CANCEL = 3,
        DEAD = 4
    };

    inline std::string OMOrderStateToString(OMOrderState side) noexcept{
        switch (side) {
          case OMOrderState::PENDING_NEW:
            return "PENDING_NEW";
          case OMOrderState::LIVE:
            return "LIVE";
          case OMOrderState::PENDING_CANCEL:
            return "PENDING_CANCEL";
          case OMOrderState::DEAD:
            return "DEAD";
          case OMOrderState::INVALID:
            return "INVALID";
        }

        return "UNKNOWN";
    }

    struct OMOrder {
        TickerId ticker_id_ = TickerId_INVALID;
        OrderId order_id_ = OrderId_INVALID;
        Side side_ = Side::INVALID;
        Price price_ = Price_INVALID;
        Qty qty_ = Qty_INVALID;
        OMOrderState order_state_ = OMOrderState::INVALID;

        inline auto toString() const noexcept{
            std::stringstream ss;
            ss << "OMOrder" << "["
               << "tid:" << tickerIdToString(ticker_id_) << " "
               << "oid:" << orderIdToString(order_id_) << " "
               << "side:" << sideToString(side_) << " "
               << "price:" << priceToString(price_) << " "
               << "qty:" << qtyToString(qty_) << " "
               << "state:" << OMOrderStateToString(order_state_) << "]";

            return ss.str();
        }
    };

    using OMOrderSideHashMap = std::array<OMOrder, sideToIndex(Side::MAX) + 1>; //TODO: maybe change with std::pair or a new struct with the operator[] defined 
    using OMOrderTickerSideHashMap = std::array<OMOrderSideHashMap, ME_MAX_TICKERS>; 
}