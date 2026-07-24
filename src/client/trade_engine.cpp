#include "client/trade_engine.hpp"

namespace trading::client {
    TradeEngine::TradeEngine(ClientId clientId, AlgoType algo_type, const TradeEngineCfgHashMap& ticker_cfg, 
                    trading::exchange::ClientRequestLFQueue& client_requests, trading::exchange::ClientResponseLFQueue& client_responses, 
                    trading::exchange::MEMarketUpdateLFQueue& market_updates)
                    : client_id_(clientId), outgoing_requests_(client_requests), incoming_responses_(client_responses), incoming_market_updates_(market_updates), 
                      logger_("TradeEngine_" + clientIdToString(clientId) + ".log"), feature_engine_(logger_), position_keeper_(logger_), 
                      risk_manager_(logger_, position_keeper_, ticker_cfg), order_manager_(*this, risk_manager_, logger_)
    {
        for(TickerId i = 0; i < ME_MAX_TICKERS; ++i){
            ticker_order_book_[i] = std::make_unique<MarketOrderBook>(i, logger_);
            ticker_order_book_[i]->setTradeEngine(this);
        }

        algoOnOrderBookUpdate_ = [this](auto ticker,auto price,auto side, auto& marketOrderBook){ defaultAlgoOnOrderBookUpdate(ticker, price, side, marketOrderBook); };
        algoOnTradeUpdate_ = [this](auto& market_update, auto& marketOrderBook){ defaultAlgoOnTradeUpdate(market_update, marketOrderBook); };
        algoOnOrderUpdate_ = [this](auto& client_responses){ defaultAlgoOnOrderUpdate(client_responses); };

        if(algo_type == AlgoType::MAKER){
            mm_algo_ = std::make_unique<MarketMaker>(logger_, *this, feature_engine_, order_manager_, ticker_cfg);
        } else if (algo_type == AlgoType::TAKER) {
            taker_algo_ = std::make_unique<LiquidityTaker>(logger_, *this, feature_engine_, order_manager_, ticker_cfg);
        }

    }

    TradeEngine::~TradeEngine(){
        run_ = false;
        
        if(thread_.joinable())
            thread_.join();
        
        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(1s);

    }

    void TradeEngine::sendClientRequest(const trading::exchange::MEClientRequest& client_request) noexcept{
        logger_.log("%:% %() % Sending %\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_), client_request.toString().c_str());
    
        outgoing_requests_.getNextWrite() = client_request;
        outgoing_requests_.updateNextWrite();
        TTT_MEASURE(T10_TradeEngine_LFQueue_write, logger_);
    }

    void TradeEngine::run() noexcept{
        logger_.log("%:% %() %\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_));

        while(run_){
            for(auto response = incoming_responses_.getNextRead(); response; response = incoming_responses_.getNextRead()){
                TTT_MEASURE(T9t_TradeEngine_LFQueue_read, logger_);
                logger_.log("%:% %() % Processing %\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_), response->toString().c_str());
            
                onOrderUpdate(*response);
                incoming_responses_.updateNextRead();
                last_event_time_ = getCurrentNanos();
            }

            for(auto market_update = incoming_market_updates_.getNextRead(); market_update; market_update = incoming_market_updates_.getNextRead()){
                TTT_MEASURE(T9_TradeEngine_LFQueue_read, logger_);
                logger_.log("%:% %() % Processing %\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_), market_update->toString().c_str());
            
                ASSERT(market_update->ticker_id_ <= ME_MAX_TICKERS, "Unknown ticker Id " + tickerIdToString(market_update->ticker_id_));
                ticker_order_book_[market_update->ticker_id_]->onMarketUpdate(*market_update); 
                
                incoming_market_updates_.updateNextRead();
                last_event_time_ = getCurrentNanos();
            }
        }
    }


    void TradeEngine::onOrderBookUpdate(TickerId ticker, Price price, Side side, MarketOrderBook* order_book) noexcept{
        logger_.log("%:% %() % ticker:% price:% side:%\n", __FILE__, __LINE__, __FUNCTION__,
                getCurrentTimeStr(&time_str_), ticker, priceToString(price).c_str(), sideToString(side).c_str());
        
        auto bbo = order_book->getBBO();

        START_MEASURE(Trading_PositionKeeper_updateBBO);
        position_keeper_.updateBBO(ticker, bbo);
        END_MEASURE(Trading_PositionKeeper_updateBBO, logger_);

        START_MEASURE(Trading_FeatureEngine_onOrderBookUpdate);
        feature_engine_.onOrderBookUpdate(ticker, price, side, *order_book);
        END_MEASURE(Trading_FeatureEngine_onOrderBookUpdate, logger_);

        START_MEASURE(Trading_TradeEngine_algoOnOrderBookUpdate_);
        algoOnOrderBookUpdate_(ticker, price, side, *order_book);
        END_MEASURE(Trading_TradeEngine_algoOnOrderBookUpdate_, logger_);
    } 

    void TradeEngine::onTradeUpdate(const trading::exchange::MEMarketUpdate& market_update, MarketOrderBook* order_book) noexcept{
        logger_.log("%:% %() % %\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_), market_update.toString().c_str());

        START_MEASURE(Trading_FeatureEngine_onTradeUpdate);
        feature_engine_.onTradeUpdate(market_update, *order_book);
        END_MEASURE(Trading_FeatureEngine_onTradeUpdate, logger_);

        START_MEASURE(Trading_TradeEngine_algoOnTradeUpdate);
        algoOnTradeUpdate_(market_update, *order_book);
        END_MEASURE(Trading_TradeEngine_algoOnTradeUpdate, logger_);
    }

    void TradeEngine::onOrderUpdate(const trading::exchange::MEClientResponse& client_response) noexcept{
        logger_.log("%:% %() % %\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_), client_response.toString().c_str());

        if(client_response.type_ == trading::exchange::ClientResponseType::FILLED){
            START_MEASURE(Trading_PositionKeeper_addFill);
            position_keeper_.addFill(client_response);
            END_MEASURE(Trading_PositionKeeper_addFill, logger_);
        }

        START_MEASURE(Trading_TradeEngine_algoOnOrderUpdate);
        algoOnOrderUpdate_(client_response);
        END_MEASURE(Trading_TradeEngine_algoOnOrderUpdate, logger_);
    }



}