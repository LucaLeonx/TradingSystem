#include <iostream>
#include <string>
#include <fstream>
#include <thread>

#include "spsc_queue.hpp"
#include "thread_utils.hpp"
#include "time_utils.hpp"

class Logger
{
private: 
    std::ofstream file;
    std::string fileName;

    spscQueue<std::string> loggerQueue;
    std::thread loggerThread;

public: 
    Logger(const std::string& filename, size_t queueLength) : fileName(filename), loggerQueue(queueLength){
        file.open(filename);
        if(file.is_open()){
            loggerThread = createAndStartThread(-1, "logger" + filename, [this]{ flushQueue();});
        }else std::cerr<<"Could not open the file" << filename << std::endl;
    } 

    void flushQueue(){
        using namespace std::literals::chrono_literals;

        while(!loggerQueue.empty()){
            std::this_thread::sleep_for(5s);
            auto toPrint = loggerQueue.getNextRead(); 
            if(toPrint != nullptr){
                file << toPrint;
                loggerQueue.updateNextRead();
            }
        }
    }

    template<typename T>
    /// @brief Pushes a value to the logger queue after converting it to a string
    /// @tparam T The type of the value to push (must be convertible via std::to_string)
    /// @note This function returns early if std::to_string produces an empty string
    /// @note std::to_string does not raise exceptions; it may produce unexpected results
    ///       for floating-point special values (NaN, Inf) but will not throw
    void pushVal(T val){
        std::string newVal = std::to_string(val);
        if(newVal.empty()){
            std::cerr<<"Value cannot be written to a file, no conversion to std::string"<<std::endl;
            return;
        }
        loggerQueue.getNextWrite() = newVal;
        loggerQueue.updateNextWrite();
    }

    //delete move and assign methods

    ~Logger(){
        loggerThread.join();
        file.close();
    }

};
