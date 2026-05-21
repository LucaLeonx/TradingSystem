#include <iostream>
#include <cassert>
#include "mem_pool.hpp"

struct myStruct{
    int val1;
    double val2;
};

inline std::ostream& operator<<(std::ostream& os, const myStruct& s) {
    return os << "{ val1: " << s.val1 << ", val2: " << s.val2 << " }";
}

int main(){

    trading::memory::MemPool<int> pool1(100);
    trading::memory::MemPool<myStruct> pool2(100);

    assert(pool1.allocate(10));
    assert(pool2.allocate(myStruct{10,10.0}));

    int *addr = pool1.allocate(7);
    std::cout<<"Allocated on first pool obj: " << *addr << " at addr: " <<addr <<std::endl;

    pool1.deallocate(addr);
    std::cout<<"Deallocated on first pool obj: " << *addr << " at addr: " <<addr <<std::endl;

    myStruct *addr2 = pool2.allocate(myStruct({7,7.7}));
    std::cout<<"\nAllocated on first pool obj: " << *addr2 << " at addr: " <<addr2 <<std::endl;
    
    pool2.deallocate(addr2);
    std::cout<<"Deallocated on first pool obj: " << *addr2 << " at addr: " <<addr2 <<std::endl;

}
