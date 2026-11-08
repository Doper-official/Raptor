#pragma once

// My libs
#include "rap_camera.hpp"
#include "rap_game_object.hpp"

// Lib
#include <vulkan/vulkan.h>
#include <cstddef>

namespace rap {

#define MAX_LIGHTS 10

    struct PointLight {
        glm::vec4 position{};
        glm::vec4 color{};
    };

    struct GlobalUbo {
        // Keep these offsets aligned with every GLSL std140 GlobalUbo block.
        alignas(16) glm::mat4 projection{ 1.f };
        alignas(16) glm::mat4 view{ 1.f };
        alignas(16) glm::mat4 inverseView{ 1.f };
        alignas(16) glm::mat4 directionalLightViewProjection{ 1.f };

        alignas(16) glm::vec4 ambientLightColor{
            1.f, 1.f, 1.f, 0.03f
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

    static_assert(sizeof(PointLight) == 32,
        "PointLight must match the GLSL std140 array stride");
    static_assert(offsetof(GlobalUbo, directionalLightViewProjection) == 192);
    static_assert(offsetof(GlobalUbo, ambientLightColor) == 256);
    static_assert(offsetof(GlobalUbo, pointLights) == 304);
    static_assert(offsetof(GlobalUbo, numLights) == 624);
    static_assert(sizeof(GlobalUbo) == 640,
        "GlobalUbo must match the GLSL std140 block size");

    struct FrameInfo {
        int frameIndex;
        float frameTime;
        VkCommandBuffer commandBuffer;
        RaptorCamera& camera;
        VkDescriptorSet globalDescriptorSet;
        RaptorGameObject::Map& gameObjects;
    };

} // namespace rap