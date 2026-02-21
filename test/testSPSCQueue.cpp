#include <iostream>
#include <thread>

#include "spsc_queue.hpp"

using namespace std::literals::chrono_literals;

int main(){
    spscQueue<int> lfQueue(100);

    std::thread consumer([&lfQueue] (){
        std::this_thread::sleep_for(1s); 
        while(lfQueue.size()){
            auto el = lfQueue.getNextRead();
            if(el != nullptr ) lfQueue.updateNextRead();
            else{
                std::cout<<"CANNOT ACCESS PRODUCER CELL"<<std::endl;
                break;
            }
            std::cout<<"Thread "<< std::this_thread::get_id() <<" consumed data: "<< *el <<"\t queue still has: " << lfQueue.size() << " elements."<<std::endl;
        }
    });

    std::thread producer([&lfQueue] (){ 
        std::this_thread::sleep_for(1s);
        for(int i{}; i < 90; ++i){    
            *lfQueue.getNextWrite() = 10;
            lfQueue.updateNextWrite();
            std::cout<<"Thread "<< std::this_thread::get_id() <<" produced data: "<< 10 <<"\t queue now has: " << lfQueue.size() << " elements."<<std::endl;
        }
    });

    consumer.join();
    producer.join();
}