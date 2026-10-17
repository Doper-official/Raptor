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

    class SimpleRenderSystem {
    public:

        SimpleRenderSystem(
            RaptorDevice& device,
            VkRenderPass renderPass,
            VkDescriptorSetLayout globalSetLayout);

        ~SimpleRenderSystem();

        SimpleRenderSystem(
            const SimpleRenderSystem&) = delete;

        SimpleRenderSystem& operator=(
            const SimpleRenderSystem&) = delete;

        void renderGameObjects(
           FrameInfo &frameInfo);

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