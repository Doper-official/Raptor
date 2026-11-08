#pragma once

#include "rap_device.hpp"
#include "rap_pipeline.hpp"
#include "rap_frame_info.hpp"

#include <glm/glm.hpp>
#include <memory>

namespace rap {

    class SkyboxSystem {
    public:
        SkyboxSystem(
            RaptorDevice& device,
            VkRenderPass renderPass,
            VkDescriptorSetLayout globalSetLayout,
            VkDescriptorSetLayout cubemapSetLayout);

        ~SkyboxSystem();

        SkyboxSystem(const SkyboxSystem&) = delete;
        SkyboxSystem& operator=(const SkyboxSystem&) = delete;

        // Call inside the swapchain render pass, before deferred lighting.
        void render(
            FrameInfo& frameInfo,
            VkDescriptorSet cubemapDescriptorSet,
            float elapsedTime);

    private:
        void createPipelineLayout(
            VkDescriptorSetLayout globalSetLayout,
            VkDescriptorSetLayout cubemapSetLayout);
        void createPipeline(VkRenderPass renderPass);

        RaptorDevice& device;
        VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };
        std::unique_ptr<RaptorPipeline> pipeline;
    };

} // namespace rap
