#include "client/liquidity_taker.hpp"
#include "client/trade_engine.hpp"

namespace trading::client {
    LiquidityTaker::LiquidityTaker(Logger& logger, TradeEngine& trade_engine, const FeatureEngine& feature_engine, OrderManager& order_manager, const TradeEngineCfgHashMap& ticker_cfg)
            : feature_engine_(feature_engine), order_manager_(order_manager), logger_(logger), ticker_cfg_(ticker_cfg) 
    {
        trade_engine.algoOnOrderBookUpdate_ = [this](auto ticker, auto price, auto side, auto& book){ onOrderBookUpdate(ticker, price, side, book); };
        trade_engine.algoOnTradeUpdate_ = [this](auto market_update, auto& book){ onTradeUpdate(market_update, book); };
        trade_engine.algoOnOrderUpdate_ = [this](auto client_response){ onOrderUpdate(client_response); };
    }

}