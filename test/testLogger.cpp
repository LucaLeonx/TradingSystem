#include "logger.hpp"

using namespace std::literals::chrono_literals;

struct obj{
    int one;
    int two;
};

int main(){

    Logger logger{"file.log"};
    std::this_thread::sleep_for(4s);

    logger.pushVal(5);
    logger.pushVal(5.0);
    logger.pushVal("Ciao");
    logger.pushVal('g');
    logger.pushVal(5.0f);
    //logger.pushVal(obj{1,2}); //Raise an exception

}