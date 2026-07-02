#pragma once

#include <stdint.h>
namespace azm::ecs
{
    class Entity
    {
      uint32_t _index;

        bool operator==(const Entity& other) const {
            return _index == other._index;
        }
    };
}
