#pragma once 

#include "exchange/market_data.hpp"
#include "client/market_order_book.hpp"

namespace trading::client {

    class TradeEngine{
    public:
        TradeEngine() {}

        void onOrderBookUpdate(TickerId ticker, Price price, Side side, MarketOrderBook* order_book){} 
        
        void onTradeUpdate(const trading::exchange::MEMarketUpdate& market_data, MarketOrderBook* order_book){}
    };
}