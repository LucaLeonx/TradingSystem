#pragma once

#include <ostream>
#include <string>
#include <sstream>

#include "utils/types.hpp"
#include "utils/spsc_queue.hpp"

namespace trading::exchange{
    
    enum class ClientRequestType : uint8_t{
        INVALID = 0,
        NEW = 1,
        CANCEL = 2
    };

    inline std::ostream& operator<<(std::ostream& os, ClientRequestType c){
        switch (c)
        {
            case ClientRequestType::INVALID:
                os << "INVALID";
                break;
            case ClientRequestType::NEW:
                os << "NEW";
                break;
            case ClientRequestType::CANCEL:
                os << "CANCEL";
                break;
            default: os << "UNKNOWN";
        }
        return os;
    }

    struct __attribute__((packed)) MEClientRequest {
        ClientRequestType type_ = ClientRequestType::INVALID;
        
        ClientId client_id_ = ClientId_INVALID;
        TickerId ticker_id_ = TickerId_INVALID;
        OrderId order_id_ = OrderId_INVALID;
        Side side_ = Side::INVALID;
        Price price_ = Price_INVALID;
        Qty qty_ = Qty_INVALID; 

        auto toString() const {
            std::stringstream ss;
            ss << "MEClientRequest"
                << " ["
                << "type:" << type_
                << " client:" << clientIdToString(client_id_)
                << " ticker:" << tickerIdToString(ticker_id_)
                << " oid:" << orderIdToString(order_id_)
                << " side:" << sideToString(side_)
                << " qty:" << qtyToString(qty_)
                << " price:" << priceToString(price_)
                << "]";
            return ss.str();
        }
    };

    using ClientRequestLFQueue = spscQueue<MEClientRequest>;

}