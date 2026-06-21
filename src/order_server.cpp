#include "exchange/order_server.hpp"

namespace trading::exchange{
    OrderServer::OrderServer(ClientRequestLFQueue& client_request, ClientResponseLFQueue& client_response, const std::string& iface, const int port) 
    : iface_(iface), port_(port), outgoing_messages_(client_response), logger_("ex_order_server.log"), tcp_server_(logger_), fifo_sequencer_(client_request, logger_){
        cid_next_exp_seq_num_.fill(1);
        cid_next_outgoing_seq_num_.fill(1);
        cid_to_socket_.fill(nullptr);

        tcp_server_.recv_callback_ = [this](auto socket, auto time) { recvCallbacks(socket, time); };
        tcp_server_.recv_finished_callback_ = [this]() { recvFinishedCallbacks(); };
    } 

    OrderServer::~OrderServer(){
        stop();

        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(1s);
    }

    void OrderServer::start(){
        run_ = true;
        tcp_server_.listen(iface_, port_);

        thread_ = createAndStartThread(-1, "Exchange_OrderServer",[this](){ run(); });
    }

    void OrderServer::stop(){
        run_ = false;

        if(thread_.joinable()) 
            thread_.join();
    }

    void OrderServer::run() noexcept{
        logger_.log("%:% %() %\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_));
        
        while (run_) {
            tcp_server_.poll();

            tcp_server_.sendAndRecv();

            for (auto client_response = outgoing_messages_.getNextRead(); outgoing_messages_.size() && client_response; client_response = outgoing_messages_.getNextRead()) {
                auto &next_outgoing_seq_num = cid_next_outgoing_seq_num_[client_response->client_id_];

                logger_.log("%:% %() % Processing cid:% seq:% %\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_),
                            client_response->client_id_, next_outgoing_seq_num, client_response->toString());

                ASSERT(cid_to_socket_[client_response->client_id_] != nullptr, "Dont have a TCPSocket for ClientId:" + std::to_string(client_response->client_id_));
                
                cid_to_socket_[client_response->client_id_]->send(&next_outgoing_seq_num, sizeof(next_outgoing_seq_num));
                cid_to_socket_[client_response->client_id_]->send(client_response, sizeof(MEClientResponse));

                outgoing_messages_.updateNextRead();

                ++next_outgoing_seq_num;
            }
        }
    }
}
