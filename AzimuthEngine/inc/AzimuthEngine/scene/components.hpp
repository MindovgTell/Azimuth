#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace azm
{
    struct Transform
    {
        glm::vec3 position{0.0f};
        glm::vec3 rotation{0.0f};
        glm::vec3 scale{1.0f};
    };

    struct Spin
    {
        glm::vec3 axis{0.0f, 1.0f, 0.0f};
        float speed{1.0f};
    };
} // namespace azm
