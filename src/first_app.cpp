#include "first_app.hpp"

// My libs
#include "keyboard_movement_controller.hpp"
#include "rap_camera.hpp"
#include "systems/simple_render_system.hpp"
#include "systems/point_light_system.hpp"
#include "systems/deferred_lighting_system.hpp"
#include "rap_window.hpp"
#include "buffers/rap_buffer.hpp"

// GLM
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// Std
#include <array>
#include <cassert>
#include <stdexcept>
#include <chrono>
#include <numeric>
#include <memory>
#include <vector>
#include <iostream>

namespace rap {

    RaptorFirstApp::RaptorFirstApp() {
        textureSetLayout = RaptorDescriptorSetLayout::Builder{ device }
            .addBinding(
                0,
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                VK_SHADER_STAGE_FRAGMENT_BIT
            )
            .build();

        globalSetLayout = RaptorDescriptorSetLayout::Builder{ device }
            .addBinding(
                0,
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                VK_SHADER_STAGE_ALL_GRAPHICS
            )
            .build();

        globalPool = RaptorDescriptorPool::Builder{ device }
            .addPoolSize(
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                RaptorSwapChain::MAX_FRAMES_IN_FLIGHT
            )
            .setMaxSets(RaptorSwapChain::MAX_FRAMES_IN_FLIGHT)
            .build();

        texturePool = RaptorDescriptorPool::Builder{ device }
            .addPoolSize(
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                1000
            )
            .setMaxSets(1000)
            .build();

        const std::vector<unsigned char> whitePixel{
            255, 255, 255, 255
        };

        defaultTexture = std::make_unique<RaptorImage>(
            device,
            VkExtent2D{ 1, 1 },
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_ASPECT_COLOR_BIT,
            whitePixel
        );

        VkDescriptorImageInfo imageInfo{};
        imageInfo.sampler = defaultTexture->getSampler();
        imageInfo.imageView = defaultTexture->getImageView();
        imageInfo.imageLayout =
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        RaptorDescriptorWriter textureWriter{
            *textureSetLayout,
            *texturePool
        };

        textureWriter.writeImage(0, &imageInfo);

        if (!textureWriter.build(defaultTextureDescriptorSet)) {
            throw std::runtime_error(
                "failed to create default texture descriptor set!"
            );
        }

        loadGameObjects();
    }

    RaptorFirstApp::~RaptorFirstApp() {}

    void RaptorFirstApp::run() {

        std::cout << "running raptor v0.0.4" << std::endl;

        // One uniform buffer per frame in flight.
        std::vector<std::unique_ptr<RapBuffer>> uboBuffers(
            RaptorSwapChain::MAX_FRAMES_IN_FLIGHT
        );

        for (size_t i = 0; i < uboBuffers.size(); ++i) {
            uboBuffers[i] = std::make_unique<RapBuffer>(
                device,
                sizeof(GlobalUbo),
                1,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            );

            uboBuffers[i]->map();
        }

        // Allocate the global descriptor sets.
        std::vector<VkDescriptorSet> globalDescriptorSets(
            RaptorSwapChain::MAX_FRAMES_IN_FLIGHT
        );

        for (size_t i = 0; i < globalDescriptorSets.size(); ++i) {
            auto bufferInfo = uboBuffers[i]->descriptorInfo();

            RaptorDescriptorWriter writer{
                *globalSetLayout,
                *globalPool
            };

            writer.writeBuffer(0, &bufferInfo);
            writer.build(globalDescriptorSets[i]);
        }

        // Geometry pass: writes albedo, normal, and depth.
        SimpleRenderSystem simpleRenderSystem{
            device,
            rapRenderer.getGeometryRenderPass(),
            globalSetLayout->getDescriptorSetLayout(),
            textureSetLayout->getDescriptorSetLayout()
        };

        // Point-light system remains available for future lights.
        PointLightSystem pointLightSystem{
            device,
            rapRenderer.getSwapChainRenderPass(),
            globalSetLayout->getDescriptorSetLayout()
        };

        // Deferred pass: shades the G-buffer into the swapchain.
        DeferredLightingSystem deferredLightingSystem{
            device,
            rapRenderer.getSwapChainRenderPass(),
            globalSetLayout->getDescriptorSetLayout()
        };

        RaptorCamera camera{};

        auto viewerObject = RaptorGameObject::createGameObject();

        viewerObject.transform.translation = {
            0.f, 0.f, -2.5f
        };

        viewerObject.transform.rotation = {
            0.f, 0.f, 0.f
        };

        KeyboardMovementController cameraController{};

        auto currentTime =
            std::chrono::high_resolution_clock::now();

        while (!window.shouldClose()) {
            glfwPollEvents();

            if (glfwGetKey(
                window.getGLFWwindow(),
                GLFW_KEY_ESCAPE
            ) == GLFW_PRESS) {
                glfwSetWindowShouldClose(
                    window.getGLFWwindow(),
                    GLFW_TRUE
                );
            }

            auto newTime =
                std::chrono::high_resolution_clock::now();

            float frameTime =
                std::chrono::duration<float>(
                    newTime - currentTime
                ).count();

            currentTime = newTime;

            frameTime = glm::min(frameTime, MAX_FRAME_TIME);

            // Camera movement.
            cameraController.moveInPlaneXZ(
                window.getGLFWwindow(),
                frameTime,
                viewerObject
            );

            camera.setViewYXZ(
                viewerObject.transform.translation,
                viewerObject.transform.rotation
            );

            // Camera projection.
            const float aspect = rapRenderer.getAspectRatio();

            camera.setPerspectiveProjection(
                glm::radians(50.f),
                aspect,
                0.1f,
                100.f
            );

            if (auto commandBuffer = rapRenderer.beginFrame()) {
                const int frameIndex =
                    rapRenderer.getFrameIndex();

                assert(frameIndex >= 0);
                assert(
                    static_cast<size_t>(frameIndex) <
                    uboBuffers.size()
                );

                FrameInfo frameInfo{
                    frameIndex,
                    frameTime,
                    commandBuffer,
                    camera,
                    globalDescriptorSets[frameIndex],
                    gameObjects
                };

                // Update the global UBO.
                GlobalUbo ubo{};

                ubo.projection = camera.getProjection();
                ubo.view = camera.getView();
                ubo.inverseView = camera.getInverseView();

                // Directional light.
                // This direction represents the direction the
                // light rays travel, not the direction toward
                // the light source.
                ubo.directionalLightDirection = glm::vec4(
                    glm::normalize(
                        glm::vec3(-0.5f, -1.f, -0.3f)
                    ),
                    0.f
                );

                // RGB = light color, W = intensity.
                ubo.directionalLightColor = {
                    1.f, 1.f, 1.f, 1.f
                };

                // Ambient illumination.
                ubo.ambientLightColor = {
                    1.f, 1.f, 1.f, 0.01f
                };

                // No point-light objects are created anymore.
                // The system still updates the UBO, so adding
                // point lights later requires no redesign.
                pointLightSystem.update(frameInfo, ubo);

                uboBuffers[frameIndex]->writeToBuffer(&ubo);

                // Update deferred-lighting descriptors for this frame.
                deferredLightingSystem.updateDescriptorSet(
                    frameIndex,
                    rapRenderer.getCurrentGeometryBuffer()
                );

                // Bind this frame's G-buffer depth image to the
                // point-light fragment shader.
                pointLightSystem.updateDepthDescriptorSet(
                    static_cast<uint32_t>(frameIndex),
                    rapRenderer
                    .getCurrentGeometryBuffer()
                    .getDepthImageView()
                );

                // PASS 1:
                // Render scene geometry into the G-buffer.
                rapRenderer.beginGeometryPass(commandBuffer);

                simpleRenderSystem.renderGameObjects(frameInfo);

                rapRenderer.endGeometryPass(commandBuffer);

                // PASS 2:
                // Shade the G-buffer and draw depth-tested billboards.
                rapRenderer.beginSwapChainRenderPass(commandBuffer);

                deferredLightingSystem.render(frameInfo);

                // This renders nothing while there are no point lights.
                pointLightSystem.render(frameInfo);

                rapRenderer.endSwapChainRenderPass(commandBuffer);

                rapRenderer.endFrame();
            }
        }

        vkDeviceWaitIdle(device.device());
    }

    // Makes one descriptor set (set 1: sampler2D) for every texture
    // loaded from the model. Index i == the model's texture i.
    std::vector<VkDescriptorSet> RaptorFirstApp::createTextureDescriptorSets(
        const RaptorModel& model) {

        std::vector<VkDescriptorSet> sets(model.getTextureCount());

        for (size_t i = 0; i < sets.size(); ++i) {
            VkDescriptorImageInfo imageInfo =
                model.getTextureDescriptorInfo(i);

            RaptorDescriptorWriter writer{
                *textureSetLayout,
                *texturePool
            };

            writer.writeImage(0, &imageInfo);

            if (!writer.build(sets[i])) {
                throw std::runtime_error(
                    "failed to create descriptor set for model texture "
                    + std::to_string(i)
                );
            }
        }

        return sets;
    }

    void RaptorFirstApp::loadGameObjects() {
        // Ground floor.
        std::shared_ptr<RaptorModel> floorModel =
            RaptorModel::createModelFromFile(
                device,
                "models/plane.glb"
            );

        auto floor = RaptorGameObject::createGameObject();

        floor.model = floorModel;

        floor.transform.translation = {
            0.f, 0.25f, -0.5f
        };

        floor.transform.scale =
            glm::vec3{ 3.f, 1.f, 3.f };

        floor.color = { 1.f, 1.f, 1.f };

        floor.defaultTextureDescriptorSet = defaultTextureDescriptorSet;
        floor.textureDescriptorSets = createTextureDescriptorSets(*floorModel);

        gameObjects.emplace(
            floor.getId(),
            std::move(floor)
        );

        // No point-light objects are created here.
        // The point-light system and shader remain available
        // for future use.
    }

} // namespace rap