cmake_minimum_required(VERSION 3.25)

project(TradingSystem)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_FLAGS "-std=c++2a -Wall -Wextra -Werror -fsanitize=address")
#set(CMAKE_VERBOSE_MAKEFILE on)

include_directories(include)

enable_testing()

add_executable(testPool test/testPools.cpp)

