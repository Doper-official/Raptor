#include "first_app.hpp"

// my libs
#include "keyboard_movement_controller.hpp"
#include "rap_camera.hpp"
#include "systems/simple_render_system.hpp"
#include "systems/point_light_system.hpp"
#include "systems/deferred_lighting_system.hpp"
#include "rap_window.hpp"
#include "rap_buffer.hpp"

// GLM
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
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

        loadGameObjects();
    }

    RaptorFirstApp::~RaptorFirstApp() {}

    void RaptorFirstApp::run() {

        std::cout << "running raptor v0.0.2" << std::endl;

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
            globalSetLayout->getDescriptorSetLayout()
        };

        // Billboard pass: draws light markers into the swapchain.
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

                pointLightSystem.render(frameInfo);

                rapRenderer.endSwapChainRenderPass(commandBuffer);

                rapRenderer.endFrame();
            }
        }

        vkDeviceWaitIdle(device.device());
    }

    void RaptorFirstApp::loadGameObjects() {

        std::shared_ptr<RaptorModel> smooth_vaseModel =
            RaptorModel::createModelFromFile(
                device,
                "models/smooth_vase.glb"
            );

        // Vase 1.
        auto smooth_vase =
            RaptorGameObject::createGameObject();

        smooth_vase.model = smooth_vaseModel;

        smooth_vase.transform.translation = {
            0.25f, 0.25f, 1.f
        };

        smooth_vase.transform.scale = glm::vec3{ 1.f };
        smooth_vase.color = { 1.f, 0.f, 0.f };

        gameObjects.emplace(
            smooth_vase.getId(),
            std::move(smooth_vase)
        );

        // Vase 2.
        auto smooth_vase1 =
            RaptorGameObject::createGameObject();

        smooth_vase1.model = smooth_vaseModel;

        smooth_vase1.transform.translation = {
            -0.25f, 0.25f, -0.5f
        };

        smooth_vase1.transform.scale = glm::vec3{ 1.f };
        smooth_vase1.color = { 1.f, 0.f, 0.f };

        gameObjects.emplace(
            smooth_vase1.getId(),
            std::move(smooth_vase1)
        );

        // Ground floor.
        std::shared_ptr<RaptorModel> floorModel =
            RaptorModel::createModelFromFile(
                device,
                "models/quad.glb"
            );

        auto floor = RaptorGameObject::createGameObject();

        floor.model = floorModel;

        floor.transform.translation = {
            0.f, 0.25f, -0.5f
        };

        floor.transform.scale =
            glm::vec3{ 3.f, 1.f, 3.f };

        floor.color = { 1.f, 0.f, 0.f };

        gameObjects.emplace(
            floor.getId(),
            std::move(floor)
        );

        // Point lights.
        std::vector<glm::vec3> lightColors{
            { 1.f, .1f, .1f },
            { .1f, .1f, 1.f },
            { .1f, 1.f, .1f },
            { 1.f, 1.f, .1f },
            { .1f, 1.f, 1.f },
            { 1.f, 1.f, 1.f }
        };

        for (int i = 0;
            i < static_cast<int>(lightColors.size());
            ++i) {

            auto pointLight =
                RaptorGameObject::makePointLight(.2f);

            pointLight.color = lightColors[i];

            auto rotateLight = glm::rotate(
                glm::mat4(1.f),
                (
                    static_cast<float>(i) *
                    glm::two_pi<float>()
                    ) / static_cast<float>(lightColors.size()),
                glm::vec3{ 0.f, -1.f, 0.f }
            );

            pointLight.transform.translation = glm::vec3(
                rotateLight *
                glm::vec4(-1.f, -1.f, -1.f, -1.f)
            );

            gameObjects.emplace(
                pointLight.getId(),
                std::move(pointLight)
            );
        }
    }

} // namespace rap