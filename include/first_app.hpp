#pragma once

#include "rap_window.hpp"
#include "rap_device.hpp"
#include "rap_game_object.hpp"
#include "rap_renderer.hpp"
#include "rap_descriptors.hpp"

// std
#include <memory>
#include <vector>
#include <string>

namespace rap {

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
        std::unique_ptr<RaptorDescriptorPool> globalPool{};
		RaptorGameObject::Map gameObjects{};
    };

} // namespace rap