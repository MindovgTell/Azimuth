#pragma once

#include <cstddef>
#include <cassert>

namespace azm::memory
{
    class Allocator
    {
    protected:
        void* _start = nullptr;
        std::size_t _size;
        std::size_t _used_memory;
        std::size_t _num_of_allocations;
    public:
        Allocator(std::size_t size, void* start)
        {
            _size = size;
            _start = start;
            _used_memory = 0;
            _num_of_allocations = 0;
        }

        virtual ~Allocator()
        {
            assert(_used_memory == 0 && _num_of_allocations == 0);
            _start = nullptr;
            _size = 0;
        }

        virtual void* allocate(std::size_t size, uint8_t allignment = 4) = 0;
        virtual void deallocate(void* p) = 0;
        void* getStart() const {return _start;}
        std::size_t getSize() const {return _size;}
        std::size_t getUsedMemory() const {return _used_memory;}
        std::size_t getNumAllocations() const {return _num_of_allocations;}
    };

    namespace allocator
    {

    } // namespace allocator
} // namespace azm::memory
