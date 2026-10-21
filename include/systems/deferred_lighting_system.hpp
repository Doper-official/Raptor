#pragma once

#include "rap_device.hpp"
#include "rap_pipeline.hpp"
#include "rap_frame_info.hpp"
#include "buffers/rap_geometry_buffer.hpp"

#include <memory>
#include <vector>

namespace rap {

    class DeferredLightingSystem {
    public:
        DeferredLightingSystem(
            RaptorDevice& device,
            VkRenderPass swapChainRenderPass,
            VkDescriptorSetLayout globalSetLayout);

        ~DeferredLightingSystem();

        DeferredLightingSystem(const DeferredLightingSystem&) = delete;
        DeferredLightingSystem& operator=(const DeferredLightingSystem&) = delete;

        // Call once per frame after beginFrame() and before the geometry pass.
        // This updates only the descriptor set for the frame whose fence has been waited.
        void updateDescriptorSet(
            int frameIndex,
            const RaptorGeometryBuffer& geometryBuffer);

        // Call inside the swapchain render pass, after the geometry pass has ended.
        void render(FrameInfo& frameInfo);

    private:
        void createDescriptorResources();
        void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
        void createPipeline(VkRenderPass swapChainRenderPass);

        RaptorDevice& device;
        VkSampler imageSampler{ VK_NULL_HANDLE };
        VkDescriptorSetLayout gBufferSetLayout{ VK_NULL_HANDLE };
        VkDescriptorPool descriptorPool{ VK_NULL_HANDLE };
        std::vector<VkDescriptorSet> gBufferDescriptorSets;
        VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };
        std::unique_ptr<RaptorPipeline> pipeline;
    };

} // namespace rap
