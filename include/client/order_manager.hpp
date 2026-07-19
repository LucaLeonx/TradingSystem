#pragma once 

#include "utils/logger.hpp"

#include "exchange/client_request.hpp"
#include "exchange/client_response.hpp"

#include "client/om_order.hpp"
#include "client/risk_manager.hpp"

namespace trading::client{
    class TradeEngine;

    class OrderManager{
    public:
        OrderManager(TradeEngine& trade_engine, const RiskManager& risk_manager, Logger& logger) : trade_engine_(trade_engine), risk_manager_(risk_manager), logger_(logger) {}

        inline const OMOrderSideHashMap& getOMOrderSideHashMap(TickerId ticker) const {
            return ticker_side_order_.at(ticker);
        }

        void onOrderUpdate(const trading::exchange::MEClientResponse& client_response) noexcept;

        void newOrder(OMOrder& order, TickerId tickerId, Price price, Side side, Qty qty) noexcept;

        void cancelOrder(OMOrder& order) noexcept;

        void moveOrder(OMOrder& order, TickerId tickerId, Price price, Side side, Qty qty) noexcept;

        void moveOrders(TickerId tickerId, Price bid_price, Price ask_price, Qty qty) noexcept;

        OrderManager() = delete;
        OrderManager(OrderManager& ) = delete;
        OrderManager(OrderManager&& ) = delete;
        OrderManager& operator=(OrderManager& ) = delete;
        OrderManager& operator=(OrderManager&& ) = delete;
    
        private:
        TradeEngine& trade_engine_;
        const RiskManager& risk_manager_;

        std::string time_str_;
        Logger& logger_;

        OMOrderTickerSideHashMap ticker_side_order_;
        OrderId next_order_id_ = 1;
    };

}