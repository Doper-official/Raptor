#pragma once

#include "rap_window.hpp"
#include "rap_device.hpp"
#include "rap_game_object.hpp"
#include "rap_renderer.hpp"
#include "rap_descriptors.hpp"
#include "rap_cubemap.hpp"

// std
#include <memory>
#include <vector>
#include <string>
#include <cstdint>

namespace rap {

    enum class SkyboxType : uint8_t {
        CUBE_SEPERATE,
        CUBE_COMBINED,
        PANORAMA
    };

    class RaptorFirstApp {
    public:
        static constexpr int WIDTH = 800;
        static constexpr int HEIGHT = 600;

        const std::string NAME = "Raptor Engine Test Or Smth";
        const float MAX_FRAME_TIME = 0.1f;

        RaptorFirstApp();
        ~RaptorFirstApp();

        RaptorFirstApp(const RaptorFirstApp&) = delete;
        RaptorFirstApp& operator=(const RaptorFirstApp&) = delete;

        void run();

    private:
        void loadGameObjects();
        std::vector<VkDescriptorSet> createTextureDescriptorSets(
            const RaptorModel& model);
        void createPipeline();
        void renderGameObjects(VkCommandBuffer commandBuffer);

        RaptorWindow window{
            WIDTH,
            HEIGHT,
            NAME
        };

        RaptorDevice device{ window };
        RaptorRenderer rapRenderer{ window, device };

        VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };

        std::unique_ptr<RaptorDescriptorSetLayout> globalSetLayout{};
        std::unique_ptr<RaptorDescriptorSetLayout> textureSetLayout;
        std::unique_ptr<RaptorDescriptorPool> globalPool{};
        std::unique_ptr<RaptorDescriptorPool> texturePool{};

		std::unique_ptr<RaptorImage> defaultTexture{};
        VkDescriptorSet defaultTextureDescriptorSet = VK_NULL_HANDLE;

        std::unique_ptr<RaptorCubeMap> cubemap{};
        VkDescriptorSet cubemapDescriptorSet = VK_NULL_HANDLE;

		RaptorGameObject::Map gameObjects{};
    };

} // namespace rap