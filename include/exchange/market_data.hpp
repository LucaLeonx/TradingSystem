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
        TRADE = 4,
        CLEAR = 5,
        SNAPSHOT_START = 6,
        SNAPSHOT_END = 7
    };

    inline std::ostream& operator<<(std::ostream& os, MarketUpdateType c){
        switch (c)
        {
            case MarketUpdateType::INVALID: 
                os << "INVALID";
                break;
            case MarketUpdateType::ADD:     
                os << "ADD";
                break;
            case MarketUpdateType::MODIFY:  
                os << "MODIFY";
                break;
            case MarketUpdateType::CANCEL:  
                os << "CANCEL";
                break;
            case MarketUpdateType::TRADE:   
                os << "TRADE";
                break;
            case MarketUpdateType::CLEAR:   
                os << "CLEAR";
                break;
            case MarketUpdateType::SNAPSHOT_START:   
                os << "SNAPSHOT_START";
                break;
            case MarketUpdateType::SNAPSHOT_END:   
                os << "SNAPSHOT_END";
                break;
            default: os << "UNKNOWN";
        }
        return os;
    }

    ///used by the matching engine to provide market data updates to the market data publishing component.
    struct __attribute__((packed)) MEMarketUpdate {
        MarketUpdateType type_ = MarketUpdateType::INVALID;

        OrderId order_id_ = OrderId_INVALID;
        TickerId ticker_id_ = TickerId_INVALID;
        Side side_ = Side::INVALID;
        Price price_ = Price_INVALID;
        Qty qty = Qty_INVALID;
        Priority priority_ = Priority_INVALID; 
    
        inline auto toString() const {
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

    ///Public format, used by the market data publishing component to provide market data updates to market clients.
    struct __attribute__((packed)) MDPMarketUpdate {
        size_t seq_num_ = 0;
        MEMarketUpdate me_market_update_;

        inline auto toString() const {
            std::stringstream ss;
            ss << "MDParketUpdate"
                << " ["
                << " seq num: " << seq_num_
                << " message: " << me_market_update_.toString()
                << "]";
            return ss.str();
        }
    };

    using MEMarketUpdateLFQueue = spscQueue<MEMarketUpdate>;
    using MDPMarketUpdateLFQueue = spscQueue<MDPMarketUpdate>;
}