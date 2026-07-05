#pragma once

#include <atomic>
#include <cstddef>
#include <type_traits>

namespace azm::ecs::util
{
    class TypeId
    {
    private:

        static std::size_t identifier() noexcept
        {
            static std::atomic_size_t value{0};
            return value.fetch_add(1, std::memory_order_relaxed);
        }

        template<class T>
        static std::size_t type_impl() noexcept
        {
            static const std::size_t value = identifier();
            return value;
        }

    public:

        template<class T>
        static std::size_t type() noexcept
        {
            using CleanT = std::remove_cv_t<std::remove_reference_t<T>>;
            return type_impl<CleanT>();
        }
    };
}
