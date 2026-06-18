#pragma once

#include <iostream>
#include <string>
#include <fstream>
#include <thread>
#include <sstream>

#include "spsc_queue.hpp"
#include "thread_utils.hpp"
#include "time_utils.hpp"
#include "macros.hpp"

namespace trading {

constexpr size_t defaultQueueSize = 8 * 1024 * 1024;

class Logger final
{
private: 
    std::ofstream file;
    const std::string fileName;

    std::atomic<bool> running{true};
    spscQueue<std::string> loggerQueue;
    std::thread loggerThread;

public: 
    Logger(const std::string& filename, size_t queueLength = defaultQueueSize) : fileName(filename), loggerQueue(queueLength){
        file.open(filename);
        ASSERT(file.is_open(),"File creation failed.");
        loggerThread = createAndStartThread(-1, "logger" + filename, [this]{ flushQueue();});
    } 

    void flushQueue(){
        using namespace std::literals::chrono_literals;

        while(running){
            auto toPrint = loggerQueue.getNextRead(); 
            if(toPrint != nullptr){
                file << *toPrint;
                loggerQueue.updateNextRead();
                file.flush();
            }
        }
    }

    template<typename T>
    /// @brief Pushes a value to the logger queue after converting it to a string
    /// @tparam T The type of the value to push (must have the operator<< defined )
    /// @note This function returns early if the operator<< produces an empty string
    /// @note It raise an exceptions; it may produce unexpected results
    ///       for floating-point special values (NaN, Inf) but will not throw
    void pushVal(T val) noexcept{
        std::ostringstream oss;
        oss << val;
        std::string newVal = oss.str();

        if(newVal.empty()){
            std::cerr<<"Value cannot be written to a file, no conversion to std::string"<<std::endl;
            return;
        }
        loggerQueue.getNextWrite() = newVal;
        loggerQueue.updateNextWrite();
    }

    ///Print-like function for log strings into file using % as a general escape character 
    template<typename T, typename ...Targs>
    void log(const char* text,const T& value, Targs... args) noexcept{
        while(*text){
            if(*text == '%'){
                if(*(text + 1) == '%') ++text;
                else{
                    pushVal(value);
                    log(text + 1, args...); //recursive call
                    return;
                }
            }
            pushVal(*text++);
        }
    }


    void log(const char* text) noexcept{
        for(; *text != '\0'; text++){
            pushVal(*text);
        }
    }
    


    Logger() = delete;
    Logger(Logger&& ) = delete; // Move contructor
    Logger(Logger const & ) = delete; // Copy constructor
    Logger& operator=(Logger const &) = delete;// Copy assignment
    Logger& operator=(Logger&&) = delete;// Move assignment


    ~Logger(){
        using namespace std::literals::chrono_literals;
        std::cout<<"Closing logger at "<< getCurrentTimeStr() <<" -> flushing the queue..."<<std::endl;
        
        while(!loggerQueue.empty()){
            std::this_thread::sleep_for(3s);
        }

        running = false;
        loggerThread.join();
        file.close();
    }
};

}
