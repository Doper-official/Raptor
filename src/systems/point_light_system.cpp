#include "systems/point_light_system.hpp"

#include "rap_swap_chain.hpp"

// GLM
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// std
#include <algorithm>
#include <array>
#include <cassert>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace rap {

    struct PointLightPushConstants {
        glm::vec4 position{};
        glm::vec4 color{};
        float radius{};
    };

    PointLightSystem::PointLightSystem(
        RaptorDevice& device,
        VkRenderPass renderPass,
        VkDescriptorSetLayout globalSetLayout)
        : device{ device } {

        createDepthSampler();
        createDepthDescriptorResources();
        createPipelineLayout(globalSetLayout);
        createPipeline(renderPass);
    }

    PointLightSystem::~PointLightSystem() {
        vkDeviceWaitIdle(device.device());

        pipeline.reset();

        if (pipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(
                device.device(),
                pipelineLayout,
                nullptr);
        }

        if (depthDescriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(
                device.device(),
                depthDescriptorPool,
                nullptr);
        }

        if (depthSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(
                device.device(),
                depthSetLayout,
                nullptr);
        }

        if (depthSampler != VK_NULL_HANDLE) {
            vkDestroySampler(
                device.device(),
                depthSampler,
                nullptr);
        }
    }

    void PointLightSystem::createDepthSampler() {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType =
            VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

        samplerInfo.magFilter = VK_FILTER_NEAREST;
        samplerInfo.minFilter = VK_FILTER_NEAREST;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;

        samplerInfo.addressModeU =
            VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV =
            VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW =
            VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;
        samplerInfo.borderColor =
            VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;

        if (vkCreateSampler(
            device.device(),
            &samplerInfo,
            nullptr,
            &depthSampler) != VK_SUCCESS) {

            throw std::runtime_error(
                "Failed to create point-light depth sampler!");
        }
    }

    void PointLightSystem::createDepthDescriptorResources() {
        VkDescriptorSetLayoutBinding depthBinding{};
        depthBinding.binding = 0;
        depthBinding.descriptorType =
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        depthBinding.descriptorCount = 1;
        depthBinding.stageFlags =
            VK_SHADER_STAGE_FRAGMENT_BIT;
        depthBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &depthBinding;

        if (vkCreateDescriptorSetLayout(
            device.device(),
            &layoutInfo,
            nullptr,
            &depthSetLayout) != VK_SUCCESS) {

            throw std::runtime_error(
                "Failed to create point-light depth descriptor layout!");
        }

        constexpr uint32_t frameCount =
            RaptorSwapChain::MAX_FRAMES_IN_FLIGHT;

        VkDescriptorPoolSize poolSize{};
        poolSize.type =
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSize.descriptorCount = frameCount;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = frameCount;

        if (vkCreateDescriptorPool(
            device.device(),
            &poolInfo,
            nullptr,
            &depthDescriptorPool) != VK_SUCCESS) {

            throw std::runtime_error(
                "Failed to create point-light depth descriptor pool!");
        }

        std::vector<VkDescriptorSetLayout> layouts(
            frameCount, depthSetLayout);

        depthDescriptorSets.resize(frameCount);

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = depthDescriptorPool;
        allocInfo.descriptorSetCount = frameCount;
        allocInfo.pSetLayouts = layouts.data();

        if (vkAllocateDescriptorSets(
            device.device(),
            &allocInfo,
            depthDescriptorSets.data()) != VK_SUCCESS) {

            throw std::runtime_error(
                "Failed to allocate point-light depth descriptor sets!");
        }
    }

    void PointLightSystem::createPipelineLayout(
        VkDescriptorSetLayout globalSetLayout) {

        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags =
            VK_SHADER_STAGE_VERTEX_BIT |
            VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size =
            sizeof(PointLightPushConstants);

        const std::array<VkDescriptorSetLayout, 2> layouts{
            globalSetLayout,
            depthSetLayout
        };

        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType =
            VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount =
            static_cast<uint32_t>(layouts.size());
        layoutInfo.pSetLayouts = layouts.data();
        layoutInfo.pushConstantRangeCount = 1;
        layoutInfo.pPushConstantRanges = &pushConstantRange;

        if (vkCreatePipelineLayout(
            device.device(),
            &layoutInfo,
            nullptr,
            &pipelineLayout) != VK_SUCCESS) {

            throw std::runtime_error(
                "Failed to create point-light pipeline layout!");
        }
    }

    void PointLightSystem::createPipeline(
        VkRenderPass renderPass) {

        assert(
            pipelineLayout != VK_NULL_HANDLE &&
            "Cannot create pipeline before pipeline layout");

        RaptorPipelineConfigInfo pipelineConfig{};

        RaptorPipeline::defaultPipelineConfigInfo(
            pipelineConfig);

        RaptorPipeline::enableAlphaBlending(
            pipelineConfig);

        pipelineConfig.attributeDescriptions.clear();
        pipelineConfig.bindingDescriptions.clear();

        pipelineConfig.renderPass = renderPass;
        pipelineConfig.pipelineLayout = pipelineLayout;

        pipeline = std::make_unique<RaptorPipeline>(
            device,
            "shaders/point_light.vert.spv",
            "shaders/point_light.frag.spv",
            pipelineConfig);
    }

    void PointLightSystem::update(
        FrameInfo& frameInfo,
        GlobalUbo& ubo) {

        const auto rotateLight = glm::rotate(
            glm::mat4(1.f),
            frameInfo.frameTime,
            glm::vec3{ 0.f, -1.f, 0.f });

        int lightIndex = 0;

        for (auto& kv : frameInfo.gameObjects) {
            auto& obj = kv.second;

            if (obj.pointLight == nullptr) {
                continue;
            }

            assert(
                lightIndex < MAX_LIGHTS &&
                "Too many point lights!");

            obj.transform.translation = glm::vec3(
                rotateLight *
                glm::vec4(obj.transform.translation, 1.f));

            ubo.pointLights[lightIndex].position =
                glm::vec4(obj.transform.translation, 1.f);

            ubo.pointLights[lightIndex].color =
                glm::vec4(
                    obj.color,
                    obj.pointLight->lightIntensity);

            ++lightIndex;
        }

        ubo.numLights = lightIndex;
    }

    void PointLightSystem::updateDepthDescriptorSet(
        uint32_t frameIndex,
        VkImageView depthImageView) {

        assert(frameIndex < depthDescriptorSets.size());
        assert(depthImageView != VK_NULL_HANDLE);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.sampler = depthSampler;
        imageInfo.imageView = depthImageView;
        imageInfo.imageLayout =
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = depthDescriptorSets[frameIndex];
        write.dstBinding = 0;
        write.dstArrayElement = 0;
        write.descriptorType =
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.descriptorCount = 1;
        write.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(
            device.device(),
            1,
            &write,
            0,
            nullptr);
    }

    void PointLightSystem::render(
        FrameInfo& frameInfo) {

        std::vector<std::pair<float, RaptorGameObject::id_t>>
            sorted;

        sorted.reserve(frameInfo.gameObjects.size());

        for (const auto& kv : frameInfo.gameObjects) {
            const auto& obj = kv.second;

            if (obj.pointLight == nullptr) {
                continue;
            }

            const auto offset =
                frameInfo.camera.getPosition() -
                obj.transform.translation;

            const float disSquared =
                glm::dot(offset, offset);

            sorted.emplace_back(disSquared, obj.getId());
        }

        std::sort(
            sorted.begin(),
            sorted.end(),
            [](const auto& a, const auto& b) {
                return a.first > b.first;
            });

        pipeline->bind(frameInfo.commandBuffer);

        assert(
            frameInfo.frameIndex >= 0 &&
            static_cast<size_t>(frameInfo.frameIndex) <
            depthDescriptorSets.size());

        const VkDescriptorSet descriptorSets[] = {
            frameInfo.globalDescriptorSet,
            depthDescriptorSets[frameInfo.frameIndex]
        };

        vkCmdBindDescriptorSets(
            frameInfo.commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout,
            0,
            2,
            descriptorSets,
            0,
            nullptr);

        for (const auto& light : sorted) {
            auto& obj =
                frameInfo.gameObjects.at(light.second);

            if (obj.pointLight == nullptr) {
                continue;
            }

            PointLightPushConstants push{};

            push.position =
                glm::vec4(obj.transform.translation, 1.f);

            push.color =
                glm::vec4(
                    obj.color,
                    obj.pointLight->lightIntensity);

            push.radius =
                obj.transform.scale.x * 0.75f;

            vkCmdPushConstants(
                frameInfo.commandBuffer,
                pipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT |
                VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(PointLightPushConstants),
                &push);

            vkCmdDraw(
                frameInfo.commandBuffer,
                6,
                1,
                0,
                0);
        }
    }

} // namespace rap