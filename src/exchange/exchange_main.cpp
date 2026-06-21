#include <csignal>

#include "exchange/matching_engine.hpp"
#include "exchange/order_server.hpp"
#include "exchange/market_data_publisher.hpp"

trading::Logger* logger = nullptr;
trading::exchange::MatchingEngine* matching_engine = nullptr;
trading::exchange::OrderServer* order_server = nullptr;
trading::exchange::MarketDataPublisher* market_data_publisher = nullptr;


void signal_handler(int){
    using namespace std::literals::chrono_literals;
    std::this_thread::sleep_for(2s);

    delete logger; 
    logger = nullptr;
    delete matching_engine;
    matching_engine = nullptr;
    delete order_server;
    order_server = nullptr;
    delete market_data_publisher;
    market_data_publisher = nullptr;

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

    const std::string mkt_pub_iface = "lo";
    const std::string snap_pub_ip = "233.252.14.1", inc_pub_ip = "233.252.14.3";
    const int snap_pub_port = 20000, inc_pub_port = 20001;

    logger->log("%:% %() % Starting Market Data Publisher...\n", __FILE__, __LINE__, __FUNCTION__, trading::getCurrentTimeStr(&time_str));
    market_data_publisher = new trading::exchange::MarketDataPublisher(market_updates, mkt_pub_iface, snap_pub_ip, snap_pub_port, inc_pub_ip, inc_pub_port);
    market_data_publisher->start();

    const std::string order_gw_iface = "lo";
    const int order_gw_port = 12345;

    logger->log("%:% %() % Starting Order Server...\n", __FILE__, __LINE__, __FUNCTION__, trading::getCurrentTimeStr(&time_str));
    order_server = new trading::exchange::OrderServer(client_requests, client_responses, order_gw_iface, order_gw_port);
    order_server->start();

    
    while(true){
        usleep(100 * 1000 * 1000);
    }

}