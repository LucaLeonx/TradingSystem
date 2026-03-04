cmake_minimum_required(VERSION 3.25)

project(TradingSystem)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_FLAGS "-std=c++2a -Wall -Wextra -Werror -fsanitize=address")
#set(CMAKE_VERBOSE_MAKEFILE on)

include_directories(include)

enable_testing()

add_executable(testMemPool test/testPools.cpp)
add_test(NAME testPool COMMAND testMemPool WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_executable(testSPSCQueue test/testSPSCQueue.cpp)
add_test(NAME testLockFreeQueue COMMAND testSPSCQueue WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_executable(testLogger test/testLogger.cpp)
add_test(NAME testAsyncLogger COMMAND testLogger WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
