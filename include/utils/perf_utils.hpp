#pragma once

#include <cstdint>

namespace trading {
    inline auto rdtsc() noexcept {
        unsigned int lo, hi;
        __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
        return ((uint64_t) hi << 32) | lo;
    }
}

#define START_MEASURE(TAG) const auto TAG = trading::rdtsc()

/// End latency measurement using rdtsc(). Expects a variable called TAG to already exist in the local scope.
#define END_MEASURE(TAG, LOGGER)                                                              \
      do {                                                                                    \
        const auto end = trading::rdtsc();                                                     \
        LOGGER.log("% RDTSC "#TAG" %\n", trading::getCurrentTimeStr(&time_str_), (end - TAG)); \
      } while(false)

/// Log a current timestamp at the time this macro is invoked.
#define TTT_MEASURE(TAG, LOGGER)                                                              \
      do {                                                                                    \
        const auto TAG = trading::getCurrentNanos();                                           \
        LOGGER.log("% TTT "#TAG" %\n", trading::getCurrentTimeStr(&time_str_), TAG);           \
      } while(false)
