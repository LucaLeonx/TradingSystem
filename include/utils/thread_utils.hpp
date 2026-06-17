#pragma once

#include <iostream>
#include <thread>
#include <atomic>
#include <sys/syscall.h>

namespace trading {

/// Set the current thread to a fixed CPU core defined by core_id
inline bool setThreadAffinity(int core_id) noexcept{
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    CPU_SET(core_id, &cpu_set);

    return pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpu_set) == 0;
}   

///Create Thread, set its affinity and passes the function through perfer forwarding
template<typename T, typename... A>
inline auto createAndStartThread(int core_id, const std::string& name, T&& func, A&& ...args) noexcept{

    auto threadBody = [&](){
        if(core_id >= 0 && !setThreadAffinity(core_id)){
            std::cerr<<"Failed to set thread affinity for thread: "<< name << " to core: "<< core_id <<std::endl;
            return;
        }
        std::cout<<"Thread affinity set for thread: "<< name << " to core: "<< core_id <<std::endl;

        std::forward<T>(func) (std::forward<A>(args)...);
    };

    std::thread t(threadBody);

    using namespace std::literals::chrono_literals;
    std::this_thread::sleep_for(1s);

    return t;
}

}
