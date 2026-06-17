#pragma once

#include <limits>
#include <cstdint>
#include <string>

#include "utils/macros.hpp"

namespace trading{

    constexpr size_t LOG_QUEUE_SIZE = 8 * 1024 * 1024;

    constexpr size_t ME_MAX_TICKERS = 8;
    constexpr size_t ME_MAX_CLIENTS_UPDATES = 256 * 1024;

    constexpr size_t ME_MAX_NUM_CLIENTS = 256;
    constexpr size_t ME_MAX_ORDER_IDS = 1024 * 1024;
    constexpr size_t ME_MAX_PRICE_LEVELS = 256;

    using OrderId = uint64_t;
    constexpr auto OrderId_INVALID = std::numeric_limits<OrderId>::max();
    inline auto orderIdToString(OrderId orderId) -> std::string {
        if (UNLIKELY(orderId == OrderId_INVALID)) {
            return "INVALID";
        }

        return std::to_string(orderId);
    }

    using TickerId = uint32_t;
    constexpr auto TickerId_INVALID = std::numeric_limits<TickerId>::max();
    inline auto tickerIdToString(TickerId tickerId) -> std::string {
        if (UNLIKELY(tickerId == TickerId_INVALID)) {
            return "INVALID";
        }

        return std::to_string(tickerId);
    }

    using ClientId = uint32_t;
    constexpr auto ClientId_INVALID = std::numeric_limits<ClientId>::max();
    inline auto clientIdToString(ClientId clientId) -> std::string {
        if (UNLIKELY(clientId == ClientId_INVALID)) {
            return "INVALID";
        }

        return std::to_string(clientId);
    }

    using Price = uint64_t;
    constexpr auto Price_INVALID = std::numeric_limits<Price>::max();
    inline auto priceToString(Price price) -> std::string {
        if (UNLIKELY(price == Price_INVALID)) {
            return "INVALID";
        }

        return std::to_string(price);
    }

    using Qty = uint32_t;
    constexpr auto Qty_INVALID = std::numeric_limits<Qty>::max();
    inline auto qtyToString(Qty qty) -> std::string {
        if (UNLIKELY(qty == Qty_INVALID)) {
            return "INVALID";
        }

        return std::to_string(qty);
    }

    using Priority = uint64_t;
    constexpr auto Priority_INVALID = std::numeric_limits<Priority>::max();
    inline auto priorityToString(Priority priority) -> std::string {
        if (UNLIKELY(priority == Priority_INVALID)) {
            return "INVALID";
        }

        return std::to_string(priority);
    }

    enum class Side : int8_t{
        INVALID = 0,
        BUY = 1,
        SELL = -1
    };

    inline auto sideToString(Side side) -> std::string {
        switch (side)
        {
            case Side::BUY:
                return "BUY";
            case Side::SELL:
                return "SELL";
            case Side::INVALID:
                return "INVALID";
        }
        return "UNKNOWN";
    }

}