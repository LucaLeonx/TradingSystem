#include "client/order_manager.hpp"
#include "client/trade_engine.hpp"

namespace trading::client{
    void OrderManager::newOrder(OMOrder& order, TickerId tickerId, Price price, Side side, Qty qty) noexcept{
        const trading::exchange::MEClientRequest newRequest{trading::exchange::ClientRequestType::NEW, trade_engine_.clientId(), tickerId, next_order_id_, side, price, qty};

        trade_engine_.SendClientRequest(newRequest);

        order = {tickerId, next_order_id_, side, price, qty, OMOrderState::PENDING_NEW};
        ++next_order_id_;
        
        logger_.log("%:% %() % Sent new order % for %\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_),
                 newRequest.toString().c_str(), order.toString().c_str());
    }

    void OrderManager::cancelOrder(OMOrder& order) noexcept{
        const trading::exchange::MEClientRequest cancelRequest{trading::exchange::ClientRequestType::CANCEL, trade_engine_.clientId(), 
                    order.ticker_id_, order.order_id_, order.side_, order.price_, order.qty_};

        trade_engine_.SendClientRequest(cancelRequest);

        order.order_state_ = OMOrderState::PENDING_CANCEL;

        logger_.log("%:% %() % Sent new order % for %\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_),
                 cancelRequest.toString().c_str(), order.toString().c_str());
    }

    void OrderManager::moveOrder(OMOrder& order, TickerId tickerId, Price price, Side side, Qty qty) noexcept{
        switch(order.order_state_){
            case OMOrderState::LIVE:{
                if(order.price_ != price) cancelOrder(order);
            }
            break;
            case OMOrderState::DEAD: {
                const auto risk_result = risk_manager_.checkPreTradeRisk(tickerId, side, qty);
                if(risk_result == RiskCheckResult::ALLOWED)
                    newOrder(order, tickerId, price, side, qty);
                else{
                    logger_.log("%:% %() % Ticker:% Side:% Qty:% RiskCheckResult:%\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_), 
                            tickerIdToString(tickerId), sideToString(side), qtyToString(qty), riskCheckResultToString(risk_result));
                }
            }
            break;
            case OMOrderState::INVALID: break;
            case OMOrderState::PENDING_CANCEL: break;
            case OMOrderState::PENDING_NEW: break;
        }
    }

    void OrderManager::moveOrders(TickerId tickerId, Price bid_price, Price ask_price, Qty qty) noexcept{
        auto& bid_order = ticker_side_order_.at(tickerId).at(sideToIndex(Side::BUY));
        moveOrder(bid_order, tickerId, bid_price, Side::BUY, qty);

        auto& ask_order = ticker_side_order_.at(tickerId).at(sideToIndex(Side::SELL));
        moveOrder(ask_order, tickerId, ask_price, Side::SELL, qty);
    }


    void OrderManager::onOrderUpdate(const trading::exchange::MEClientResponse& client_response) noexcept{
        logger_.log("%:% %() % %\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_), client_response.toString().c_str());
      
        auto& order = ticker_side_order_.at(client_response.ticker_id_).at(sideToIndex(client_response.side_));
        
        logger_.log("%:% %() % %\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_), order.toString().c_str());

        using namespace trading::exchange;
        switch(client_response.type_){
            case ClientResponseType::ACCEPTED: order.order_state_ = OMOrderState::LIVE;
            break;
            
            case ClientResponseType::CANCELLED: order.order_state_ = OMOrderState::DEAD;
            break;
            
            case ClientResponseType::FILLED: {
                order.qty_ = client_response.left_qty_;
                if(!order.qty_){
                    order.order_state_ = OMOrderState::DEAD;
                }
            }
            break;

            case ClientResponseType::CANCEL_REJECTED: break;
            case ClientResponseType::INVALID: break;
        }
    }

}