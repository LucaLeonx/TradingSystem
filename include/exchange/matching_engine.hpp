#pragma once

#include <array>

#include "utils/logger.hpp"
#include "utils/spsc_queue.hpp"
#include "utils/types.hpp"

#include "exchange/client_request.hpp"
#include "exchange/client_response.hpp"
#include "exchange/me_order.hpp"
#include "exchange/order_book.hpp"

namespace trading::exchange{
    class MatchingEngine final{
    private:
        OrderBookHashMap ticker_order_book_;

        ClientRequestLFQueue* incoming_request_queue_ = nullptr;
        ClientResponseLFQueue* outcoming_response_queue_ = nullptr;
        MEMarketUpdateLFQueue* market_updates_queue_ = nullptr;

        std::thread thread_;

        volatile bool run_{false};

        std::string time_str_;
        Logger logger_;

    public:
        MatchingEngine(ClientRequestLFQueue* requests_queue, ClientResponseLFQueue* response_queue, MEMarketUpdateLFQueue* market_updates_queue);

        ~MatchingEngine();

        void start();

        auto run() noexcept;
        
        void stop();

        auto processClientRequest(const MEClientRequest& client_request) noexcept;

        auto sendClientResponse(MEClientResponse&& client_response) noexcept;

        auto sendMarketUpdate(MEMarketUpdate&& market_update) noexcept;

        MatchingEngine() = delete;
        MatchingEngine(MatchingEngine& other) = delete;
        MatchingEngine(MatchingEngine&& other) = delete;
        MatchingEngine& operator=(MatchingEngine& other) = delete;
        MatchingEngine& operator=(MatchingEngine&& other) = delete;
    };

}
