#pragma once

#include <chrono>
#include <string>

namespace trading {

using Nanos = int64_t;

constexpr Nanos NANOS_TO_MICROS = 1000;
constexpr Nanos MICROS_TO_MILLIS = 1000;
constexpr Nanos MILLIS_TO_SEC = 1000;

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

}