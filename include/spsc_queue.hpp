#pragma once

#include <vector>
#include <atomic>
#include <cassert>

template<typename T>
class spscQueue final{
private:
    std::vector<T> store_;
    std::atomic<size_t> next_read_idx = {0};
    std::atomic<size_t> next_write_idx = {0};

    std::atomic<size_t> num_elements = {0};
public:
    spscQueue(size_t numEl) : store_(numEl, T{}) {
    }

    spscQueue() = delete;
    spscQueue(spscQueue const &) = delete; //Copy constructor
    spscQueue(spscQueue&& other) = delete; //Move constructor
    spscQueue& operator=(spscQueue const &) = delete; //copy assignment
    spscQueue& operator=(spscQueue&& other) = delete; //Move assignment

    auto getNextWrite() noexcept{
        return &store_[next_write_idx];
    }

    auto updateNextWrite() noexcept{
        next_write_idx = (next_write_idx + 1) % store_.size();
        num_elements++;
    }

    const T* getNextRead() const noexcept{
        if(next_read_idx == next_write_idx) return nullptr;
        return &store_[next_read_idx];
    }

    auto updateNextRead(){
        next_read_idx = (next_read_idx + 1) % store_.size();
        assert(num_elements != 0);
        num_elements--;
    }

    auto size() noexcept{
        return num_elements.load();
    }

    bool empty() noexcept{
        return this->size() == 0;
    }
};