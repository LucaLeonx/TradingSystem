#pragma once

#include "utils/types.hpp"

#include "client/position_keeper.hpp"

namespace trading::client{
    enum class RiskCheckResult : int8_t{
        INVALID = 0,
        ORDER_TOO_LARGE = 1,
        POSITION_TOO_LARGE = 2,
        LOSS_TOO_LARGE = 3,
        ALLOWED = 4
    };

    inline std::string riskCheckResultToString(RiskCheckResult risk){
        switch(risk){
            case RiskCheckResult::ALLOWED:              return "ALLOWED";
            case RiskCheckResult::INVALID:              return "INVALID";
            case RiskCheckResult::ORDER_TOO_LARGE:      return "ORDER_TOO_LARGE";
            case RiskCheckResult::POSITION_TOO_LARGE:   return "POSITION_TOO_LARGE";
            case RiskCheckResult::LOSS_TOO_LARGE:       return "LOSS_TOO_LARGE";
        }

        return "INVALID";
    }

    struct RiskInfo{
        const PositionInfo* position_info_;
        RiskCfg risk_cfg_;

        auto checkPreTradeRisk(Side side, Qty qty) const noexcept{
            if(qty > risk_cfg_.max_order_size_)
                return RiskCheckResult::ORDER_TOO_LARGE;
            if(std::abs(position_info_->position_ +  sideToValue(side) * static_cast<int32_t>(qty)) > static_cast<int32_t>(risk_cfg_.max_position_))
                return RiskCheckResult::POSITION_TOO_LARGE;
            if(position_info_->total_pnl_ < risk_cfg_.max_loss_)
                return RiskCheckResult::LOSS_TOO_LARGE;

            return RiskCheckResult::ALLOWED;
        }

        auto toString() const {
            std::stringstream ss;
            ss << "RiskInfo" << "["
                << "pos:" << position_info_->toString() << " "
                << risk_cfg_.toString()
                << "]";

            return ss.str();
        }

    };

    using TickerRiskInfoHashMap = std::array<RiskInfo, ME_MAX_TICKERS>;

    class RiskManager{
    public:
        RiskManager(Logger& logger, const PositionKeeper& position_keeper, const TickerRiskInfoHashMap& ticker_cfg) : logger_(logger){
            for(TickerId i = 0; i < ME_MAX_TICKERS; ++i){
                ticker_risk_[i].position_info_ = position_keeper.getPositionInfo(i);
                ticker_risk_[i].risk_cfg_ = ticker_cfg[i].risk_cfg_;
            }
        }

        inline RiskCheckResult checkPreTradeRisk(TickerId ticker, Side side, Qty qty) const noexcept{
            return ticker_risk_.at(ticker).checkPreTradeRisk(side, qty);
        }


    private:
        std::string time_str_;
        Logger& logger_;

        TickerRiskInfoHashMap ticker_risk_;
    };
}
