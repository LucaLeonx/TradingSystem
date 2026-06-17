#include "utils/logger.hpp"

using namespace std::literals::chrono_literals;

#include <filesystem>
#include <fstream>
#include <thread>
struct obj{
    int one;
    int two;
};

int main(){

    // Ensure the logs directory and file exist for later use
    std::filesystem::path logPath = std::filesystem::path("../logs") / "test1.log";
    std::filesystem::create_directories(logPath.parent_path());
    std::ofstream{logPath.string(), std::ios::app};

    trading::Logger logger{"../logs/test1.log"};
    std::this_thread::sleep_for(4s);
    std::string var{"Robot"};

    logger.log("Hi %, this is test number:%\n", var,0);
    logger.log("Hi %, this is test numb:%\n", 2.3,"1");
    logger.log("Hi %%, this is test numb:%\n");
    //logger.pushVal(obj{1,2}); //Raise an exception

}