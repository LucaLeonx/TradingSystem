#pragma once 

#include <functional>

#include "exchange/market_data.hpp"
#include "exchange/client_response.hpp"

#include "client/market_order_book.hpp"
#include "client/order_manager.hpp"

namespace trading::client {

    class TradeEngine{
    public:
        TradeEngine() {}

        void onOrderBookUpdate(TickerId ticker, Price price, Side side, MarketOrderBook* order_book); 
        
        void onTradeUpdate(const trading::exchange::MEMarketUpdate& market_data, MarketOrderBook* order_book);

        void SendClientRequest(const trading::exchange::MEClientRequest& client_request);

        ClientId clientId() noexcept;

        std::function<void(TickerId, Price, Side,MarketOrderBook& )> algoOnOrderBookUpdate_;

        std::function<void(trading::exchange::MEMarketUpdate, const MarketOrderBook&)> algoOnTradeUpdate_;

        std::function<void(trading::exchange::MEClientResponse&)> algoOnOrderUpdate_;
    };
}