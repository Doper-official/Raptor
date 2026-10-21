#include "systems/deferred_lighting_system.hpp"

#include "rap_swap_chain.hpp"

#include <array>
#include <cassert>
#include <stdexcept>
#include <vector>

namespace rap {

    DeferredLightingSystem::DeferredLightingSystem(
        RaptorDevice& device,
        VkRenderPass swapChainRenderPass,
        VkDescriptorSetLayout globalSetLayout)
        : device{ device } {

        createDescriptorResources();
        createPipelineLayout(globalSetLayout);
        createPipeline(swapChainRenderPass);
    }

    DeferredLightingSystem::~DeferredLightingSystem() {
        vkDeviceWaitIdle(device.device());

        pipeline.reset();

        if (pipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
        }
        if (descriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device.device(), descriptorPool, nullptr);
        }
        if (gBufferSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device.device(), gBufferSetLayout, nullptr);
        }
        if (imageSampler != VK_NULL_HANDLE) {
            vkDestroySampler(device.device(), imageSampler, nullptr);
        }
    }

    void DeferredLightingSystem::createDescriptorResources() {
        // Nearest sampling is valid for both color and depth formats and avoids
        // requiring linear-filter support from the chosen depth format.
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_NEAREST;
        samplerInfo.minFilter = VK_FILTER_NEAREST;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;
        samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;

        if (vkCreateSampler(device.device(), &samplerInfo, nullptr, &imageSampler) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create deferred-lighting image sampler!");
        }

        std::array<VkDescriptorSetLayoutBinding, 3> bindings{};
        for (uint32_t i = 0; i < bindings.size(); ++i) {
            bindings[i].binding = i;
            bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            bindings[i].descriptorCount = 1;
            bindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            bindings[i].pImmutableSamplers = nullptr;
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(
            device.device(), &layoutInfo, nullptr, &gBufferSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create G-buffer descriptor-set layout!");
        }

        constexpr uint32_t frameCount = RaptorSwapChain::MAX_FRAMES_IN_FLIGHT;
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSize.descriptorCount = frameCount * 3;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.maxSets = frameCount;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;

        if (vkCreateDescriptorPool(device.device(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create deferred-lighting descriptor pool!");
        }

        std::vector<VkDescriptorSetLayout> layouts(frameCount, gBufferSetLayout);
        gBufferDescriptorSets.resize(frameCount, VK_NULL_HANDLE);

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = frameCount;
        allocInfo.pSetLayouts = layouts.data();

        if (vkAllocateDescriptorSets(device.device(), &allocInfo, gBufferDescriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate deferred-lighting descriptor sets!");
        }
    }

    void DeferredLightingSystem::createPipelineLayout(
        VkDescriptorSetLayout globalSetLayout) {

        const std::array<VkDescriptorSetLayout, 2> setLayouts{
            globalSetLayout,
            gBufferSetLayout
        };

        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
        layoutInfo.pSetLayouts = setLayouts.data();

        if (vkCreatePipelineLayout(device.device(), &layoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create deferred-lighting pipeline layout!");
        }
    }

    void DeferredLightingSystem::createPipeline(VkRenderPass swapChainRenderPass) {
        RaptorPipelineConfigInfo config{};
        RaptorPipeline::defaultPipelineConfigInfo(config);

        // Fullscreen triangle uses gl_VertexIndex and has no vertex buffers.
        config.bindingDescriptions.clear();
        config.attributeDescriptions.clear();
        config.renderPass = swapChainRenderPass;
        config.pipelineLayout = pipelineLayout;
        config.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
        config.depthStencilInfo.depthTestEnable = VK_FALSE;
        config.depthStencilInfo.depthWriteEnable = VK_FALSE;
        config.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_ALWAYS;

        pipeline = std::make_unique<RaptorPipeline>(
            device,
            "shaders/deferred_lighting.vert.spv",
            "shaders/deferred_lighting.frag.spv",
            config);
    }

    void DeferredLightingSystem::updateDescriptorSet(
        int frameIndex,
        const RaptorGeometryBuffer& geometryBuffer) {

        assert(frameIndex >= 0 && frameIndex < static_cast<int>(gBufferDescriptorSets.size()) &&
            "Deferred-lighting frame index is out of bounds!");

        const std::array<VkDescriptorImageInfo, 3> imageInfos{{
            { imageSampler, geometryBuffer.getAlbedoImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
            { imageSampler, geometryBuffer.getNormalImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
            { imageSampler, geometryBuffer.getDepthImageView(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL }
        }};

        std::array<VkWriteDescriptorSet, 3> writes{};
        for (uint32_t i = 0; i < writes.size(); ++i) {
            writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[i].dstSet = gBufferDescriptorSets[frameIndex];
            writes[i].dstBinding = i;
            writes[i].dstArrayElement = 0;
            writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[i].descriptorCount = 1;
            writes[i].pImageInfo = &imageInfos[i];
        }

        vkUpdateDescriptorSets(
            device.device(),
            static_cast<uint32_t>(writes.size()), writes.data(),
            0, nullptr);
    }

    void DeferredLightingSystem::render(FrameInfo& frameInfo) {
        assert(frameInfo.frameIndex >= 0 && frameInfo.frameIndex < static_cast<int>(gBufferDescriptorSets.size()) &&
            "Deferred-lighting frame index is out of bounds!");

        pipeline->bind(frameInfo.commandBuffer);

        const std::array<VkDescriptorSet, 2> descriptorSets{
            frameInfo.globalDescriptorSet,
            gBufferDescriptorSets[frameInfo.frameIndex]
        };

        vkCmdBindDescriptorSets(
            frameInfo.commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout,
            0,
            static_cast<uint32_t>(descriptorSets.size()),
            descriptorSets.data(),
            0,
            nullptr);

        // Fullscreen triangle: vertices are generated in the vertex shader.
        vkCmdDraw(frameInfo.commandBuffer, 3, 1, 0, 0);
    }

} // namespace rap
