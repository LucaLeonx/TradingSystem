#pragma once

#include <cstring>
#include <iostream>

#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

inline auto ASSERT(bool cond, const std::string &msg) noexcept {
  if (UNLIKELY(!cond)) {
    std::cerr << "ASSERT : " << msg << std::endl;

    std::exit(EXIT_FAILURE);
  }
}

inline auto FATAL(const std::string& msg){
  std::cerr << "FATAL : " << msg << std::endl;

  std::exit(EXIT_FAILURE);
  
}