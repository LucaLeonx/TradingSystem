#pragma once

#include <chrono>
#include <string>

#include "utils/perf_utils.hpp"

namespace trading {

    using Nanos = int64_t;

    constexpr Nanos NANOS_TO_MICROS = 1e+3;
    constexpr Nanos MICROS_TO_MILLIS = 1e+3;
    constexpr Nanos MILLIS_TO_SECS = 1e+3;
    constexpr Nanos NANOS_TO_SECS = 1e+9;

    inline std::string getCurrentTimeStr() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        return std::string(std::ctime(&time));
    }

    inline auto getCurrentTimeStr(std::string* time_str) {
        auto clock = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(clock);
        
        char nanos_str[24];
        sprintf(nanos_str, "%.8s.%09ld", ctime(&time) + 11, std::chrono::duration_cast<std::chrono::nanoseconds>(clock.time_since_epoch()).count() % NANOS_TO_SECS);

        time_str->assign(nanos_str);
        return *time_str;
    }   

    inline auto getCurrentNanos() noexcept {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}
}