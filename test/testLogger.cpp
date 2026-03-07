#include "logger.hpp"

using namespace std::literals::chrono_literals;

struct obj{
    int one;
    int two;
};

int main(){

    Logger logger{"file.log"};
    std::this_thread::sleep_for(4s);
    std::string var{"Robot"};

    logger.log("Hi %, this is test number:%\n", var,0);
    logger.log("Hi %, this is test numb:%\n", 2.3,"1");
    logger.log("Hi %%, this is test numb:%\n");
    //logger.pushVal(obj{1,2}); //Raise an exception

}