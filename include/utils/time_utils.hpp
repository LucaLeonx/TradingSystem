#pragma once

#include <chrono>
#include <string>

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
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        time_str->assign(std::ctime(&time));
        return *time_str;
    }   

    inline auto getCurrentNanos() noexcept {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}
}