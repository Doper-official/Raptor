#pragma once

#include "rap_window.hpp"
#include "rap_device.hpp"
#include "rap_swap_chain.hpp"

// std
#include <memory>
#include <vector>
#include <cassert>

namespace rap {

    class RaptorRenderer {
    public:

        RaptorRenderer(RaptorWindow& window, RaptorDevice& device);
        ~RaptorRenderer();

        RaptorRenderer(const RaptorRenderer&) = delete;
        RaptorRenderer& operator=(const RaptorRenderer&) = delete;


        VkRenderPass getSwapChainRenderPass() const { return rapSwapChain->getRenderPass(); }
        float getAspectRatio() const { return rapSwapChain->extentAspectRatio(); }

        bool isFrameInProgress() const { return isFrameStarted; }

        VkCommandBuffer getCurrentCommandBuffer() const {
            assert(isFrameStarted && "Cannot get command buffer when frame not in progress");
            return commandBuffers[currentFrameIndex];
        }

        int getFrameIndex() const {
            assert(isFrameStarted && "Cannot get frame index when frame not in progress");
            return currentFrameIndex;
        }

        VkCommandBuffer beginFrame();
        void endFrame();

        void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
        void endSwapChainRenderPass(VkCommandBuffer commandBuffer);

    private:
        void createCommandBuffers();
        void freeCommandBuffers();
        void recreateSwapChain();

        RaptorWindow& rapWindow;
        RaptorDevice& rapDevice;

        std::unique_ptr<RaptorSwapChain> rapSwapChain;
        std::vector<VkCommandBuffer> commandBuffers;

        uint32_t currentImageIndex;
        int currentFrameIndex{};
        bool isFrameStarted{ false };
    };

} // namespace rap