#pragma once

#include "exchange/market_data.hpp"
#include "exchange/me_order.hpp"
#include "exchange/client_request.hpp"

namespace trading::exchange{

    class MEOrderBook{
    private:
    
    public:
        void add(){}
        void cancel(){}
    };

    using OrderBookHashMap = std::array<std::unique_ptr<MEOrderBook>, ME_MAX_TICKERS>;
}