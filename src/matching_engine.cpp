#include "exchange/matching_engine.hpp"

namespace trading::exchange{

    MatchingEngine::MatchingEngine(ClientRequestLFQueue& requests_queue, ClientResponseLFQueue& response_queue, MEMarketUpdateLFQueue& market_updates_queue) 
        : logger_("Matching_Engine.log"), incoming_request_queue_(requests_queue), outcoming_response_queue_(response_queue),  market_updates_queue_(market_updates_queue) 
    {
        for(size_t i{}; i < ticker_order_book_.size(); ++i){
            ticker_order_book_[i] = std::make_unique<MEOrderBook>(i, logger_, *this);
        }
    }

    MatchingEngine::~MatchingEngine(){
        run_ = false;

        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(1s);
        logger_.log("%:% %() %\tDestroying the Matching Engine, SELF DESTRUCTION IN  3   2   1...\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_));

        if(thread_.joinable()) thread_.join();
    }

    auto MatchingEngine::processClientRequest(const MEClientRequest& client_request) noexcept{
        auto& order_book = ticker_order_book_[client_request.ticker_id_];
        switch(client_request.type_){
            case ClientRequestType::NEW:
                order_book->add(client_request.client_id_, client_request.order_id_, client_request.side_, client_request.price_, client_request.qty_);
                break;
            case ClientRequestType::CANCEL:
                order_book->cancel(client_request.client_id_, client_request.order_id_);
                break;
            case ClientRequestType::INVALID:
                ASSERT(false, "Received INVALID ClientRequest");
                break;
        }
    }

    auto MatchingEngine::run() noexcept{
        logger_.log("%:% %() %\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_));
        
        while(run_){
            const auto client_request = incoming_request_queue_.getNextRead();
            if(client_request){
                logger_.log("%:% %() %- Processing:  %\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_), client_request->toString());
                processClientRequest(*client_request);
                incoming_request_queue_.updateNextRead();
            }
        }
    }

    //Publish the response to the client into the responses Lock-free queue, outcoming_response_queue_
    void MatchingEngine::sendClientResponse(MEClientResponse&& client_response) noexcept{
        logger_.log("%:% %() % - Send response: %\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_), client_response.toString());
        
        auto& next_write = outcoming_response_queue_.getNextWrite();
        next_write = std::move(client_response);
        outcoming_response_queue_.updateNextWrite();
    }

    void MatchingEngine::sendMarketUpdate(MEMarketUpdate&& market_update) noexcept{
        logger_.log("%:% %() % - Send market data update: %\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_), market_update.toString());
        
        auto& next_write = market_updates_queue_.getNextWrite();
        next_write = std::move(market_update);
        market_updates_queue_.updateNextWrite();
    }

    void MatchingEngine::start(){
        run_ = true;
        thread_ = trading::createAndStartThread(-1, "Matching Engine thread", [this](){ run();} );
    }

    void MatchingEngine::stop(){
        run_ = false;
        if(thread_.joinable()) thread_.join();
    }

}