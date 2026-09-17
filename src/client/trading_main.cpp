#include <csignal>
#include <cstdlib>

#include "utils/logger.hpp"
#include "utils/types.hpp"

#include "client/order_gateway.hpp"
#include "client/market_data_consumer.hpp"
#include "client/trade_engine.hpp"

/// ./trading_main CLIENT_ID ALGO_TYPE [CLIP_1 THRESH_1 MAX_ORDER_SIZE_1 MAX_POS_1 MAX_LOSS_1] [CLIP_2 THRESH_2 MAX_ORDER_SIZE_2 MAX_POS_2 MAX_LOSS_2] ...
int main(int argc, char **argv){
    if(argc < 3){
        FATAL("USAGE trading_main CLIENT_ID ALGO_TYPE [CLIP_1 THRESH_1 MAX_ORDER_SIZE_1 MAX_POS_1 MAX_LOSS_1] [CLIP_2 THRESH_2 MAX_ORDER_SIZE_2 MAX_POS_2 MAX_LOSS_2] ...");
    }

    using namespace trading;

    const ClientId client_id = std::atoi(argv[1]);
    srand(client_id);

    const auto algo_type = trading::stringToAlgoType(argv[2]);

    Logger logger("TradingMain_" + trading::clientIdToString(client_id) + ".log");
    exchange::ClientRequestLFQueue client_request(ME_MAX_CLIENTS_UPDATES);
    exchange::ClientResponseLFQueue client_response(ME_MAX_CLIENTS_UPDATES);
    exchange::MEMarketUpdateLFQueue market_updates(ME_MAX_CLIENTS_UPDATES);

    std::string time_str;

    TradeEngineCfgHashMap ticker_cfg;

    // Parse and initialize the TradeEngineCfgHashMap above from the command line arguments.
    // [CLIP_1 THRESH_1 MAX_ORDER_SIZE_1 MAX_POS_1 MAX_LOSS_1] [CLIP_2 THRESH_2 MAX_ORDER_SIZE_2 MAX_POS_2 MAX_LOSS_2] ...
    size_t next_ticker_id = 0;
    for (int i = 3; i < argc; i += 5, ++next_ticker_id) {
    ticker_cfg.at(next_ticker_id) = {static_cast<Qty>(std::atoi(argv[i])), std::atof(argv[i + 1]),
                                        {static_cast<Qty>(std::atoi(argv[i + 2])),
                                        static_cast<Qty>(std::atoi(argv[i + 3])),
                                        std::atof(argv[i + 4])}};
    }

    logger.log("%:% %() % Starting Trade Engine...\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str));

    client::TradeEngine trade_engine(client_id, algo_type, ticker_cfg, client_request, client_response, market_updates);
    trade_engine.start();

    const std::string order_gw_ip = "127.0.0.1";
    const std::string order_gw_iface = "lo";
    const int order_gw_port = 12345;
    logger.log("%:% %() % Starting Order Gateway...\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str));
    
    client::OrderGateway order_gateway(client_id, client_request, client_response, order_gw_ip, order_gw_iface, order_gw_port);
    order_gateway.start();
    
    const std::string mkt_data_iface = "lo";
    const std::string snapshot_ip = "233.252.14.1";
    const int snapshot_port = 20000;
    const std::string incremental_ip = "233.252.14.3";
    const int incremental_port = 20001;
    logger.log("%:% %() % Starting Market Data Consumer...\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str));
    
    client::MarketDataConsumer market_data_consumer(client_id, market_updates, mkt_data_iface, snapshot_ip, snapshot_port, incremental_ip, incremental_port);
    market_data_consumer.start();

    usleep(10 * 1000 * 1000);

    trade_engine.initLastEventTime();




    while (trade_engine.silentSeconds() < 60) {
        logger.log("%:% %() % Waiting till no activity, been silent for % seconds...\n", __FILE__, __LINE__, __FUNCTION__,
                        getCurrentTimeStr(&time_str), trade_engine.silentSeconds());

        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(30s);
    }

    trade_engine.stop();
    market_data_consumer.stop();
    order_gateway.stop();

    using namespace std::literals::chrono_literals;
    std::this_thread::sleep_for(3s);

    std::exit(EXIT_SUCCESS);
}