#pragma once

#include "EntityManager.hpp"
#include "ComponentManager.hpp"

namespace azm::ecs
{
    class Registry {
    private:
        EntityManager _entities;
        ComponentManager _components;

    public:
        Entity create()
        {
            return _entities.create();
        }

        void destroy(Entity entity)
        {
            assert(_entities.is_alive(entity));

            _components.destroy(entity);
            _entities.destroy(entity);
        }

        bool is_alive(Entity entity) const
        {
            return _entities.is_alive(entity);
        }

        template<typename T, typename... Args>
        T& emplace(Entity entity, Args&&... args)
        {
            assert(_entities.is_alive(entity));
            return _components.emplace<T>(entity, std::forward<Args>(args)...);
        }

        template<typename T>
        T& get(Entity entity)
        {
            assert(_entities.is_alive(entity));
            return _components.get<T>(entity);
        }

        template<typename T>
        const T& get(Entity entity) const
        {
            assert(_entities.is_alive(entity));
            return _components.get<T>(entity);
        }

        template<typename T>
        bool has(Entity entity) const
        {
            return _entities.is_alive(entity) && _components.has<T>(entity);
        }

        template<typename T>
        void remove(Entity entity)
        {
            assert(_entities.is_alive(entity));
            _components.remove<T>(entity);
        }

        template<typename T, typename Func>
        void each(Func&& func)
        {
            _components.each<T>(std::forward<Func>(func));
        }
    };
} // namespace azm::ecs
