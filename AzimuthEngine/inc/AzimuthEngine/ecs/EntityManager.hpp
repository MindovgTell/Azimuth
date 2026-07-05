#pragma once

#include <vector>
#include <cassert>

#include "Entity.hpp"

namespace azm::ecs
{

    class EntityManager
    {
        struct EntityInfo {
            std::uint32_t generation = 1;
            bool is_alive = false;
            bool pending_destroy = false;
        };

        std::vector<EntityInfo> _infos;
        std::vector<std::uint32_t> _free_ids;
        std::vector<Entity> _pending_destroyed;
    public:

        void reserve(std::size_t count) {
            _infos.reserve(count);
            _free_ids.reserve(count);
            _pending_destroyed.reserve(count);
        }

        Entity create() {
            std::uint32_t index;
            if (!_free_ids.empty()) {
                index = _free_ids.back();
                _free_ids.pop_back();
            }
            else {
                index = static_cast<std::uint32_t>(_infos.size());
                _infos.emplace_back();
            }

            EntityInfo& slot = _infos[index];
            slot.is_alive = true;
            slot.pending_destroy = false;

            return Entity::create(index, slot.generation);
        }

        bool destroy(Entity entity) {
            if (!is_alive(entity))
                return false;

            EntityInfo& info = _infos[entity.index()];

            info.is_alive = false;
            info.pending_destroy = true;

            ++info.generation;

            if (info.generation == 0)
                ++info.generation;

            _pending_destroyed.push_back(entity);

            return true;
        }

        bool is_alive(Entity entity) const {
            if(!entity.is_valid())
                return false;
            const std::uint32_t index = entity.index();
            if (index >= _infos.size())
                return false;
            const EntityInfo& info = _infos[index];
            return info.is_alive &&
                   !info.pending_destroy &&
                   info.generation == entity.generation();
        }

        std::vector<Entity> take_pending_destroyed()
        {
            std::vector<Entity> result;
            result.swap(_pending_destroyed);
            return result;
        }


        void recycle_destroyed_index(std::uint32_t index)
        {
            assert(index < _infos.size());

            EntityInfo& info = _infos[index];

            assert(!info.is_alive);
            assert(info.pending_destroy);

            info.pending_destroy = false;

            _free_ids.push_back(index);
        }

        std::size_t capacity() const
        {
            return _infos.size();
        }

        std::size_t free_count() const
        {
            return _free_ids.size();
        }
    };
}   // namespace azm::ecs
