///used by the matching engine to provide market data updates to the market data publishing component.
#pragma once

#include <sstream>

#include "utils/types.hpp"
#include "utils/spsc_queue.hpp"

namespace trading::exchange{

    enum class MarketUpdateType : uint8_t {
        INVALID = 0,
        ADD = 1,
        MODIFY = 2,
        CANCEL = 3, 
        TRADE = 4
    };

    inline std::ostream& operator<<(std::ostream& os, MarketUpdateType c){
        switch (c)
        {
            case MarketUpdateType::INVALID: os << "INVALID";
            case MarketUpdateType::ADD:     os << "ADD";
            case MarketUpdateType::MODIFY:  os << "MODIFY";
            case MarketUpdateType::CANCEL:  os << "CANCEL";
            case MarketUpdateType::TRADE:   os << "TRADE";
            default: os << "UNKNOWN";
        }
        return os;
    }

    struct __attribute__((packed)) MEMarketUpdate {
        MarketUpdateType type_ = MarketUpdateType::INVALID;

        OrderId order_id_ = OrderId_INVALID;
        TickerId ticker_id_ = TickerId_INVALID;
        Side side_ = Side::INVALID;
        Price price_ = Price_INVALID;
        Priority priority_ = Priority_INVALID; 
    
        auto toString() const {
            std::stringstream ss;
            ss << "MEMarketUpdate"
                << " ["
                << "type:" << type_
                << " orderID:" << clientIdToString(order_id_)
                << " ticker:" << tickerIdToString(ticker_id_)
                << " side:" << sideToString(side_)
                << " price:" << priceToString(price_)
                << " priority:" << priceToString(priority_)
                << "]";
            return ss.str();
        }
    };

    using MEMarketUpdateLFQueue = spscQueue<MEMarketUpdate>;
}