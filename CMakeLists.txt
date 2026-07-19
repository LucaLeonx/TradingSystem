cmake_minimum_required(VERSION 3.25)

project(TradingSystem)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_FLAGS "-std=c++2a -Wall -Wextra -Werror -fsanitize=address")
#set(CMAKE_VERBOSE_MAKEFILE on)

include_directories(include)

set(SOURCES
    src/network/tcp_socket.cpp
    src/network/tcp_server.cpp
    src/network/mcast_socket.cpp

    src/exchange/matching_engine.cpp
    src/exchange/order_book.cpp
    src/exchange/order_server.cpp
    src/exchange/market_data_publisher.cpp
    src/exchange/snapshot_synthetizer.cpp

    src/client/market_data_consumer.cpp
    src/client/market_order_book.cpp
    src/client/order_gateway.cpp
    src/client/order_manager.cpp
) 

# Create a folder for logs inside the build directory
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/logs)   

add_executable(exchange_main src/exchange/exchange_main.cpp ${SOURCES})

enable_testing()

add_executable(testSnippet test/snippet.cpp)
add_test(NAME testSnippet COMMAND testSnippet WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_executable(testMemPool test/testPools.cpp)
add_test(NAME testPool COMMAND testMemPool WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_executable(testSocket test/testSocket.cpp ${SOURCES})
add_test(NAME testSocket COMMAND testSocket WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_executable(testSPSCQueue test/testSPSCQueue.cpp)
add_test(NAME testLockFreeQueue COMMAND testSPSCQueue WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_executable(testLogger test/testLogger.cpp)
add_test(NAME testAsyncLogger COMMAND testLogger WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
