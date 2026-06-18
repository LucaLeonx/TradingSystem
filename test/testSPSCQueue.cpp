#include <iostream>
#include <thread>

#include "utils/spsc_queue.hpp"
#include "utils/thread_utils.hpp"

using namespace std::literals::chrono_literals;

int main(){
    trading::spscQueue<int> lfQueue(30);

    auto consumer = [&lfQueue] (){
        std::this_thread::sleep_for(5s); 
        while(lfQueue.size()){
            auto el = lfQueue.getNextRead();
            if(el != nullptr ) lfQueue.updateNextRead();
            else{
                std::cout<<"CANNOT ACCESS PRODUCER CELL"<<std::endl;
                break;
            }
            std::cout<<"Thread "<< std::this_thread::get_id() <<" consumed data: "<< *el <<"\t queue still has: " << lfQueue.size() << " elements."<<std::endl;
            std::this_thread::sleep_for(2s); 
        
        }
    };

    auto c = trading::createAndStartThread(7, "consumerThread", consumer);
    
    for(int i{}; i < 20; ++i){    
        lfQueue.getNextWrite() = 10;
        lfQueue.updateNextWrite();
        std::cout<<"Thread "<< std::this_thread::get_id() <<" produced data: "<< 10 <<"\t queue now has: " << lfQueue.size() << " elements."<<std::endl;
        std::this_thread::sleep_for(2s);
    }

    c.join();
     
    return 0;
}