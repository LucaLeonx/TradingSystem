#include <csignal>

#include "exchange/matching_engine.hpp"

trading::Logger* logger = nullptr;
trading::exchange::MatchingEngine* matching_engine = nullptr;

void signal_handler(int){
    using namespace std::literals::chrono_literals;
    std::this_thread::sleep_for(2s);

    delete logger; 
    logger = nullptr;
    delete matching_engine;
    matching_engine = nullptr;

    std::this_thread::sleep_for(5s);
    
    exit(EXIT_SUCCESS);
}


int main(){
    logger = new trading::Logger("exchange_ME.log");

    std::signal(SIGINT, signal_handler);

    trading::exchange::ClientRequestLFQueue client_requests(trading::ME_MAX_CLIENTS_UPDATES);
    trading::exchange::ClientResponseLFQueue client_responses(trading::ME_MAX_CLIENTS_UPDATES);
    trading::exchange::MEMarketUpdateLFQueue market_updates(trading::ME_MAX_MARKET_UPDATES);

    std::string time_str;

    logger->log("%:% %() %\tStarting Matching Engine...\n", __FILE__, __LINE__, __FUNCTION__, trading::getCurrentTimeStr(&time_str));
    matching_engine = new trading::exchange::MatchingEngine(client_requests, client_responses, market_updates);
    matching_engine->start();

    while(true){
        usleep(100 * 1000 * 1000);
    }

}