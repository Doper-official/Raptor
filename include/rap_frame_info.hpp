#pragma once

// My libs
#include "rap_camera.hpp"
#include "rap_game_object.hpp"

// Lib
#include <vulkan/vulkan.h>

namespace rap {

#define MAX_LIGHTS 10

    struct PointLight {
        glm::vec4 position{};
        glm::vec4 color{};
    };

    struct GlobalUbo {
        alignas(16) glm::mat4 projection{ 1.f };
        alignas(16) glm::mat4 view{ 1.f };
        alignas(16) glm::mat4 inverseView{ 1.f };

        alignas(16) glm::vec4 ambientLightColor{
            1.f, 1.f, 1.f, 0.01f
        };

        // Direction the light rays travel.
        // The shader negates this to get the direction toward the light.
        alignas(16) glm::vec4 directionalLightDirection{
            -0.5f, -1.f, -0.3f, 0.f
        };

        // RGB = light color, W = intensity.
        alignas(16) glm::vec4 directionalLightColor{
            1.f, 1.f, 1.f, 1.f
        };

        PointLight pointLights[MAX_LIGHTS]{};

        int numLights = 0;
    };

    struct FrameInfo {
        int frameIndex;
        float frameTime;
        VkCommandBuffer commandBuffer;
        RaptorCamera& camera;
        VkDescriptorSet globalDescriptorSet;
        RaptorGameObject::Map& gameObjects;
    };

} // namespace rap