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

        PointLightSystem(
            const PointLightSystem&) = delete;

        PointLightSystem& operator=(
            const PointLightSystem&) = delete;

        void update(FrameInfo& frameInfo, GlobalUbo& ubo);

        void render(
            FrameInfo& frameInfo);

    private:

        void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);

        void createPipeline(
            VkRenderPass renderPass);

        RaptorDevice& device;

        std::unique_ptr<RaptorPipeline> pipeline;

        VkPipelineLayout pipelineLayout =
            VK_NULL_HANDLE;
    };

} // namespace rap