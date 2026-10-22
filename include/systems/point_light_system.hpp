#pragma once

#include "rap_camera.hpp"
#include "rap_pipeline.hpp"
#include "rap_device.hpp"
#include "rap_game_object.hpp"
#include "rap_frame_info.hpp"

// std
#include <memory>
#include <vector>

namespace rap {

    class PointLightSystem {
    public:

        PointLightSystem(
            RaptorDevice& device,
            VkRenderPass renderPass,
            VkDescriptorSetLayout globalSetLayout);

        ~PointLightSystem();

        PointLightSystem(const PointLightSystem&) = delete;
        PointLightSystem& operator=(const PointLightSystem&) = delete;

        void update(FrameInfo& frameInfo, GlobalUbo& ubo);

        // Bind the current frame's G-buffer depth image.
        void updateDepthDescriptorSet(
            uint32_t frameIndex,
            VkImageView depthImageView);

        void render(FrameInfo& frameInfo);

    private:

        void createDepthSampler();
        void createDepthDescriptorResources();

        void createPipelineLayout(
            VkDescriptorSetLayout globalSetLayout);

        void createPipeline(VkRenderPass renderPass);

        RaptorDevice& device;

        std::unique_ptr<RaptorPipeline> pipeline;

        VkPipelineLayout pipelineLayout =
            VK_NULL_HANDLE;

        VkDescriptorSetLayout depthSetLayout =
            VK_NULL_HANDLE;

        VkDescriptorPool depthDescriptorPool =
            VK_NULL_HANDLE;

        VkSampler depthSampler =
            VK_NULL_HANDLE;

        std::vector<VkDescriptorSet> depthDescriptorSets;
    };

} // namespace rap