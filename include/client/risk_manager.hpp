#pragma once

#include "utils/types.hpp"

namespace trading::client{
    enum class RiskCheckResult : int8_t{
        ALLOWED = 0
    };

    std::string riskCheckResultToString(RiskCheckResult risk){
        switch(risk){
            case RiskCheckResult::ALLOWED: return "ALLOWED";
        }

        return "INVALID";
    }

    
    class RiskManager{
    public:
        RiskCheckResult checkPreTradeRisk(TickerId ticker, Side side, Qty qty) const noexcept;


    private:
    };
}
