#include "client/market_data_consumer.hpp"

namespace trading::client {
    MarketDataConsumer::MarketDataConsumer(ClientId clientId, trading::exchange::MEMarketUpdateLFQueue& market_update_queue, const std::string& iface, 
                            const std::string snapshot_ip, const int snapshot_port, const std::string incremental_ip, const int incremental_port)
        :   incoming_market_update_queue_(market_update_queue) , logger_("data_consumer_" + clientIdToString(clientId) + ".log"), incremental_socket_(logger_), snapshot_socket_(logger_),
                                iface_(iface), snapshot_ip_(snapshot_ip), snapshot_port_(snapshot_port) 
    {
        auto recvCallback = [this](auto socket){
            recvCallbacks(socket);
        };
        incremental_socket_.recv_callback_ = recvCallback;


        ASSERT(incremental_socket_.init(incremental_ip, iface, incremental_port, /*is_listening*/ true) >= 0,
                "Unable to create incremental mcast socket. error:" + std::string(std::strerror(errno)));

        ASSERT(incremental_socket_.join(incremental_ip),
                "Join failed on:" + std::to_string(incremental_socket_.socket_fd_) + " error:" + std::string(std::strerror(errno)));

        snapshot_socket_.recv_callback_ = recvCallback;

    }

    MarketDataConsumer::~MarketDataConsumer(){
        stop();

        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(5s);
    }

    void MarketDataConsumer::start(){
        run_ = true;

        thread_ = createAndStartThread(-1, "Market Data Publisher", [this](){ run(); });
    }

    void MarketDataConsumer::stop(){
        run_ = false;

        if(thread_.joinable()){
            thread_.join();
        }

    }

    void MarketDataConsumer::run() noexcept{
        logger_.log("%:% %() %\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_));

        while(run_){
            incremental_socket_.sendAndRecv();
            snapshot_socket_.sendAndRecv();
        }
    }

    void MarketDataConsumer::recvCallbacks(McastSocket* socket){
        
    }

}