#include "systems/skybox_system.hpp"

#include <glm/gtc/matrix_inverse.hpp>

#include <cassert>
#include <stdexcept>

namespace rap {

    namespace {
        struct alignas(16) SkyboxPushConstants {
            glm::mat4 inverseProjection{ 1.0f };
            glm::vec4 time{};
        };

        static_assert(sizeof(SkyboxPushConstants) == 80,
            "Skybox push constants must match the GLSL layout (80 bytes)");
    }

    SkyboxSystem::SkyboxSystem(
        RaptorDevice& device,
        VkRenderPass renderPass,
        VkDescriptorSetLayout globalSetLayout,
        VkDescriptorSetLayout cubemapSetLayout)
        : device{ device } {

        createPipelineLayout(globalSetLayout, cubemapSetLayout);
        createPipeline(renderPass);
    }

    SkyboxSystem::~SkyboxSystem() {
        vkDeviceWaitIdle(device.device());

        pipeline.reset();

        if (pipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(
                device.device(),
                pipelineLayout,
                nullptr);
        }
    }

    void SkyboxSystem::createPipelineLayout(
        VkDescriptorSetLayout globalSetLayout,
        VkDescriptorSetLayout cubemapSetLayout) {

        VkDescriptorSetLayout setLayouts[] = {
            globalSetLayout,
            cubemapSetLayout
        };

        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(SkyboxPushConstants);

        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = 2;
        layoutInfo.pSetLayouts = setLayouts;
        layoutInfo.pushConstantRangeCount = 1;
        layoutInfo.pPushConstantRanges = &pushConstantRange;

        if (vkCreatePipelineLayout(
            device.device(),
            &layoutInfo,
            nullptr,
            &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error(
                "Failed to create skybox pipeline layout!");
        }
    }

    void SkyboxSystem::createPipeline(VkRenderPass renderPass) {
        assert(pipelineLayout != VK_NULL_HANDLE);

        RaptorPipelineConfigInfo config{};
        RaptorPipeline::defaultPipelineConfigInfo(config);

        // Fullscreen triangle generated from gl_VertexIndex.
        config.bindingDescriptions.clear();
        config.attributeDescriptions.clear();

        config.renderPass = renderPass;
        config.pipelineLayout = pipelineLayout;
        config.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;

        // The sky is a background layer. It does not read or write the
        // swapchain depth attachment; deferred lighting preserves it by
        // discarding fragments whose G-buffer depth is clear.
        config.depthStencilInfo.depthTestEnable = VK_FALSE;
        config.depthStencilInfo.depthWriteEnable = VK_FALSE;
        config.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_ALWAYS;

        pipeline = std::make_unique<RaptorPipeline>(
            device,
            "shaders/skybox.vert.spv",
            "shaders/skybox.frag.spv",
            config);
    }

    void SkyboxSystem::render(
        FrameInfo& frameInfo,
        VkDescriptorSet cubemapDescriptorSet,
        float elapsedTime) {

        assert(frameInfo.commandBuffer != VK_NULL_HANDLE);

        SkyboxPushConstants push{};
        push.inverseProjection = glm::inverse(frameInfo.camera.getProjection());
        push.time.x = elapsedTime;

        pipeline->bind(frameInfo.commandBuffer);

        //set 0: global UBO
        vkCmdBindDescriptorSets(
            frameInfo.commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout,
            0,
            1,
            &frameInfo.globalDescriptorSet,
            0,
            nullptr);
        
        //set 1: cubemap
        vkCmdBindDescriptorSets(
            frameInfo.commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout,
            1,
            1,
            &cubemapDescriptorSet,
            0,
            nullptr
        );

        vkCmdPushConstants(
            frameInfo.commandBuffer,
            pipelineLayout,
            VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(SkyboxPushConstants),
            &push);

        vkCmdDraw(frameInfo.commandBuffer, 3, 1, 0, 0);
    }

} // namespace rap
