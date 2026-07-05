#pragma once

#include <cstddef>

namespace azm::memory
{
    template<typename T>
    class PoolAllocator
    {
    public:
        explicit PoolAllocator(std::size_t capacity = 1024);

        T* allocate();
        void deallocate(T* ptr);
    };
}
