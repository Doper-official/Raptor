#pragma once

#include "rap_device.hpp"
#include "rap_pipeline.hpp"
#include "rap_frame_info.hpp"
#include "rap_image.hpp"

#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace rap {

    class DirectionalShadowSystem {
    public:
        static constexpr uint32_t SHADOW_MAP_SIZE = 2048;

        explicit DirectionalShadowSystem(RaptorDevice& device);
        ~DirectionalShadowSystem();

        DirectionalShadowSystem(const DirectionalShadowSystem&) = delete;
        DirectionalShadowSystem& operator=(const DirectionalShadowSystem&) = delete;

        // direction is the direction the light rays travel (for a sun above
        // the scene, this has a negative Y component in Raptor's Y-up world).
        static glm::mat4 makeLightSpaceMatrix(
            const glm::vec3& rayDirection,
            const glm::vec3& sceneCenter = glm::vec3{ 0.f, -0.2f, -0.5f });

        void render(FrameInfo& frameInfo, const glm::mat4& lightViewProjection);
        VkDescriptorImageInfo getDescriptorInfo(int frameIndex) const;

    private:
        void createShadowImages();
        void createRenderPass();
        void createFramebuffers();
        void createPipelineLayout();
        void createPipeline();
        void cleanup();

        RaptorDevice& device;
        VkFormat depthFormat{ VK_FORMAT_UNDEFINED };
        VkRenderPass renderPass{ VK_NULL_HANDLE };
        VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };
        std::unique_ptr<RaptorPipeline> pipeline;
        std::vector<std::unique_ptr<RaptorImage>> shadowImages;
        std::vector<VkFramebuffer> framebuffers;
    };

} // namespace rap
