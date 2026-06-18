#pragma once

#include <ostream>
#include <string>
#include <sstream>

#include "utils/types.hpp"
#include "utils/spsc_queue.hpp"

namespace trading::exchange{
    
    enum class ClientResponseType : uint8_t{
        INVALID = 0,
        ACCEPTED = 1,
        CANCELLED = 2,
        FILLED = 3,
        CANCEL_REJECTED = 5
    };

    inline std::ostream& operator<<(std::ostream& os, ClientResponseType c){
        switch (c)
        {
            case ClientResponseType::INVALID:           
                os << "INVALID";
                break;
            case ClientResponseType::ACCEPTED:          
                os << "ACCEPTED";
                break;
            case ClientResponseType::CANCELLED:         
                os << "CANCELLED";
                break;
            case ClientResponseType::FILLED:            
                os << "FILLED";
                break;
            case ClientResponseType::CANCEL_REJECTED:   
                os << "CANCEL_REJECTED";
                break;
            default: os << "UNKNOWN";
        }
        return os;
    }

    struct __attribute__((packed)) MEClientResponse {
        ClientResponseType type_ = ClientResponseType::INVALID;
        
        ClientId client_id_ = ClientId_INVALID;
        TickerId ticker_id_ = TickerId_INVALID;
        OrderId client_order_id_ = OrderId_INVALID;
        OrderId market_order_id_ = OrderId_INVALID;
        Side side_ = Side::INVALID;
        Price price_ = Price_INVALID;
        Qty exec_qty_ = Qty_INVALID; 
        Qty left_qty_ = Qty_INVALID; 

        inline auto toString() const {
            std::stringstream ss;
            ss << "MEClientResponse"
                << " ["
                << "type:" << type_
                << " client:" << clientIdToString(client_id_)
                << " ticker:" << tickerIdToString(ticker_id_)
                << " client oid:" << orderIdToString(client_order_id_)
                << " market oid:" << orderIdToString(market_order_id_)
                << " side:" << sideToString(side_)
                << " executed qty:" << qtyToString(exec_qty_)
                << " left qty:" << qtyToString(left_qty_)
                << " price:" << priceToString(price_)
                << "]";
            return ss.str();
        }
    };

    using ClientResponseLFQueue = spscQueue<MEClientResponse>;

}