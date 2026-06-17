#pragma once

#include <string>
#include <vector>
#include <cassert>

namespace trading {

template <typename T>
class MemPool
{
private:
    struct ObjectBlock
    {
        T object;
        bool is_free = true;
    };
    std::vector<ObjectBlock> cells;
    size_t next_free_index = 0;
    void updateNextFree()
    {
        for (size_t i{}; i < cells.size(); ++i)
        { // could be further optimized with less variables
            if (cells[i].is_free)
            {
                next_free_index = i;
                return;
            }
        }
        // No more free cells, program abort
        next_free_index = cells.size();
        assert(false);
    }

public:
    MemPool(size_t numCells) : cells(numCells, {T{}, true})
    {
        assert(reinterpret_cast<ObjectBlock *> (&(cells[0].object)) == &(cells[0])); //I want that the first element in the row is the object, so they need to have same address
        // Should also assert the numCells smaller than the free heap size?
    };

    MemPool() = delete;
    MemPool &operator=(MemPool &&other) = delete;      // move assignment
    MemPool &operator=(MemPool const &other) = delete; // copy assignment
    MemPool(MemPool &&other) = delete;                 // move constructor
    MemPool(MemPool const &other) = delete;            // copy constructor

    template <typename... Args>
    T *allocate(Args... args)
    {
        auto obj_block = &(cells[next_free_index]);
        T *ret = &(obj_block->object);

        ret = new (ret) T(args...); // Placement new
        obj_block->is_free = false;

        updateNextFree();

        return ret;
    }

    void deallocate(const T *oldObject) 
    {   
        auto index = reinterpret_cast<const ObjectBlock*>(oldObject) - &cells[0];

        assert(index >= 0 && static_cast<size_t>(index) < cells.size()); 
        assert(!cells[index].is_free);

        cells[index].is_free = true;
    }
};

}
