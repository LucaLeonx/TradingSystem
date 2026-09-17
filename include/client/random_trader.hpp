#pragma once

#include <array>
#include <cstdlib>
#include <functional>
#include <utility>
#include <vector>

#include "exchange/client_request.hpp"
#include "utils/time_utils.hpp"
#include "utils/types.hpp"

namespace trading::client {
    class RandomTrader {
    public:
        using SendRequest = std::function<void(const exchange::MEClientRequest&)>;

        RandomTrader(ClientId client_id, SendRequest send_request)
            : client_id_(client_id), send_request_(std::move(send_request)),
              next_order_id_(client_id * 1000),
              next_action_time_(getCurrentNanos() + 10 * NANOS_TO_SECS) {
            for(auto& base_price : ticker_base_price_)
                base_price = (rand() % 100) + 100;
        }

        void onTimer() noexcept {
            if(order_count_ >= 10000 || getCurrentNanos() < next_action_time_)
                return;

            if(!cancel_next_){
                const auto ticker_id = static_cast<TickerId>(rand() % ME_MAX_TICKERS);
                const auto price = ticker_base_price_[ticker_id] + (rand() % 10) + 1;
                const auto qty = static_cast<Qty>(1 + (rand() % 100) + 1);
                const auto side = (rand() % 2 ? Side::BUY : Side::SELL);

                exchange::MEClientRequest request{exchange::ClientRequestType::NEW, client_id_, ticker_id,
                                                  next_order_id_++, side, price, qty};
                send_request_(request);
                requests_.push_back(request);
                ++order_count_;
                cancel_next_ = true;
            } else {
                const auto cancel_index = rand() % requests_.size();
                auto request = requests_[cancel_index];
                request.type_ = exchange::ClientRequestType::CANCEL;
                send_request_(request);
                cancel_next_ = false;
            }

            next_action_time_ = getCurrentNanos() + 20 * 1000 * 1000;
        }

        RandomTrader() = delete;
        RandomTrader(RandomTrader&) = delete;
        RandomTrader(RandomTrader&&) = delete;
        RandomTrader& operator=(RandomTrader&) = delete;
        RandomTrader& operator=(RandomTrader&&) = delete;

    private:
        const ClientId client_id_;
        SendRequest send_request_;
        OrderId next_order_id_;
        size_t order_count_ = 0;
        Nanos next_action_time_;
        bool cancel_next_ = false;
        std::vector<exchange::MEClientRequest> requests_;
        std::array<Price, ME_MAX_TICKERS> ticker_base_price_{};
    };
}
