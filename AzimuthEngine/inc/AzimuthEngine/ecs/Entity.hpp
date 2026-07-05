#pragma once

#include <cstdint>
#include <limits>

namespace azm::ecs
{
    class Entity
    {
    public:
        using value_type = std::uint64_t;
        static constexpr std::uint32_t INDEX_BITS = 32;
        static constexpr std::uint32_t GENERATION_BITS = 32;

        static constexpr std::uint64_t INDEX_MASK =
            (std::uint64_t{1} << INDEX_BITS) - 1;

        static constexpr value_type INVALID_VALUE = std::numeric_limits<value_type>::max();
    private:
        value_type _value = INVALID_VALUE;
    public:
        constexpr Entity() = default;
        constexpr explicit Entity(value_type v) : _value(v) {}

        static constexpr Entity create(std::uint32_t id, std::uint32_t gen) {
            return Entity{
                (static_cast<value_type>(gen) << INDEX_BITS) |
                static_cast<value_type>(id)
            };
        }

        constexpr value_type value() const {
            return _value;
        }

        constexpr std::uint32_t index() const {
            return static_cast<std::uint32_t>(_value & INDEX_MASK);
        }

        constexpr std::uint32_t generation() const {
            return static_cast<std::uint32_t>(_value >> INDEX_BITS);
        }

        constexpr bool is_valid() const {
            return _value != INVALID_VALUE;
        }

        constexpr explicit operator bool() const {
            return is_valid();
        }

        friend constexpr bool operator==(Entity lhs, Entity rhs) {
            return lhs._value == rhs._value;
        }

        friend constexpr bool operator!=(Entity lhs, Entity rhs) {
            return !(lhs == rhs);
        }

    };
}
