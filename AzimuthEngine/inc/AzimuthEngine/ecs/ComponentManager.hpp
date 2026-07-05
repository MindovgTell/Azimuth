#pragma once

#include <unordered_map>
#include <typeindex>
#include <memory>
#include <vector>
#include <cassert>
#include <type_traits>
#include <utility>

#include "Entity.hpp"
#include "Utils/TypeId.hpp"
// #include "memory/PoolAllocator.hpp"

namespace azm::ecs
{
    class IComponentStorage
    {
    public:
        virtual ~IComponentStorage() = default;

        virtual void remove(Entity entity) = 0;
        virtual bool contains(Entity entity) const = 0;
        virtual void clear() = 0;

        virtual std::size_t size() const = 0;
        virtual const std::vector<Entity> entities() const = 0;
    };

    template<class T>
        class ComponentStorage final : public IComponentStorage
        {
        private:
            std::unordered_map<std::uint64_t, T> _components;
            std::unordered_map<std::uint64_t, Entity> _entities;
        public:
            template<class... Args>
            T& emplace(Entity entity, Args&&... args)
            {
                assert(entity.is_valid());
                assert(!contains(entity));

                const std::uint64_t key = entity.value();

                T component{ std::forward<Args>(args)... };

                auto [it, inserted] = _components.emplace(key, std::move(component));
                assert(inserted);

                _entities.emplace(key, entity);

                return it->second;
            }

            // template<class... Args>
            // T& EmplaceOrReplace(Entity entity, Args&&... args)
            // {
            //     if (Contains(entity))
            //         Remove(entity);

            //     return Emplace(entity, std::forward<Args>(args)...);
            // }

            T& get(Entity entity)
            {
                assert(contains(entity));
                return _components.at(entity.value());
            }

            const T& get(Entity entity) const
            {
                assert(contains(entity));
                return _components.at(entity.value());
            }

            bool contains(Entity entity) const override
            {
                if (!entity.is_valid())
                    return false;

                return _components.find(entity.value()) != _components.end();
            }

            void remove(Entity entity) override
            {
                const std::uint64_t key = entity.value();

                _components.erase(key);
                _entities.erase(key);
            }

            void clear() override
            {
                _components.clear();
                _entities.clear();
            }

            std::size_t size() const override
            {
                return _components.size();
            }

            const std::vector<Entity> entities() const override
            {
                std::vector<Entity> result;
                result.reserve(_entities.size());

                for (const auto& [key, entity] : _entities)
                {
                    result.push_back(entity);
                }

                return result;
            }

            template<class Func>
            void each(Func&& func)
            {
                for (auto& [key, component] : _components)
                {
                    Entity entity = _entities.at(key);

                    if constexpr (std::is_invocable_v<Func, Entity, T&>)
                    {
                        func(entity, component);
                    }
                    else
                    {
                        func(component);
                    }
                }
            }
        };

    using ComponentTypeId = std::uint32_t;

    class ComponentManager
    {
    private:
        std::unordered_map<ComponentTypeId, std::unique_ptr<IComponentStorage>> _storage;

    public:
        ComponentManager() = default;

        ComponentManager(const ComponentManager&) = delete;
        ComponentManager& operator=(const ComponentManager&) = delete;

        ComponentManager(ComponentManager&&) = default;
        ComponentManager& operator=(ComponentManager&&) = default;


        void destroy(Entity entity)
        {
            for (auto& [_, store] : _storage)
            {
                store->remove(entity);
            }
        }

        template<typename T, typename... Args>
        T& emplace(Entity entity, Args&&... args)
        {
            return storage<T>().emplace(entity, std::forward<Args>(args)...);
        }

        template<typename T>
        T& get(Entity entity)
        {
            return storage<T>().get(entity);
        }

        template<typename T>
        const T& get(Entity entity) const
        {
            const auto* store = find_storage<T>();
            assert(store != nullptr);
            return store->get(entity);
        }

        template<typename T>
        bool has(Entity entity) const
        {
            const auto* store = find_storage<T>();
            return store != nullptr && store->contains(entity);
        }

        template<typename T>
        void remove(Entity entity)
        {
            auto* store = find_storage<T>();
            if (store != nullptr)
            {
                storage<T>().remove(entity);
            }
        }

        template<typename T, typename Func>
        void each(Func&& func)
        {
            storage<T>().each(std::forward<Func>(func));
        }

    private:
        template<typename T>
        ComponentStorage<T>& storage()
        {
            const ComponentTypeId type = util::TypeId::type<T>();
            auto it = _storage.find(type);
            if (it == _storage.end())
            {
                auto created = std::make_unique<ComponentStorage<T>>();
                auto* raw = created.get();
                _storage.emplace(type, std::move(created));
                return *raw;
            }
            return static_cast<ComponentStorage<T>&>(*it->second);
        }


        template<typename T>
        const ComponentStorage<T>* find_storage() const
        {
            const ComponentTypeId type = util::TypeId::type<T>();

            auto it = _storage.find(type);
            if (it == _storage.end())
            {
                return nullptr;
            }

            return static_cast<const ComponentStorage<T>*>(it->second.get());
        }
    };
}
