#pragma once

#include "rap_window.hpp"
#include "rap_device.hpp"
#include "rap_swap_chain.hpp"
#include "buffers/rap_geometry_buffer.hpp"

// Standard library
#include <cassert>
#include <memory>
#include <vector>

namespace rap {

    class RaptorRenderer {
    public:
        RaptorRenderer(RaptorWindow& window, RaptorDevice& device);
        ~RaptorRenderer();

        RaptorRenderer(const RaptorRenderer&) = delete;
        RaptorRenderer& operator=(const RaptorRenderer&) = delete;

        // Swapchain
        VkRenderPass getSwapChainRenderPass() const {
            return rapSwapChain->getRenderPass();
        }

        // Geometry render pass
        VkRenderPass getGeometryRenderPass() const {
            assert(
                geometryRenderPass != VK_NULL_HANDLE &&
                "Geometry render pass has not been created!");

            return geometryRenderPass;
        }

        // Current frame's G-buffer
        RaptorGeometryBuffer& getCurrentGeometryBuffer() {
            assert(
                isFrameStarted &&
                "Cannot get geometry buffer when frame is not in progress!");

            assert(
                currentFrameIndex >= 0 &&
                currentFrameIndex <
                static_cast<int>(geometryBuffers.size()) &&
                "Geometry buffer frame index is out of bounds!");

            assert(
                geometryBuffers[currentFrameIndex] != nullptr &&
                "Current geometry buffer is null!");

            return *geometryBuffers[currentFrameIndex];
        }

        float getAspectRatio() const {
            return rapSwapChain->extentAspectRatio();
        }

        bool isFrameInProgress() const {
            return isFrameStarted;
        }

        VkCommandBuffer getCurrentCommandBuffer() const {
            assert(
                isFrameStarted &&
                "Cannot get command buffer when frame not in progress");

            return commandBuffers[currentFrameIndex];
        }

        int getFrameIndex() const {
            assert(
                isFrameStarted &&
                "Cannot get frame index when frame not in progress");

            return currentFrameIndex;
        }

        // Frame management
        VkCommandBuffer beginFrame();
        void endFrame();

        // Geometry pass
        void beginGeometryPass(VkCommandBuffer commandBuffer);
        void endGeometryPass(VkCommandBuffer commandBuffer);

        // Swapchain pass
        void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
        void endSwapChainRenderPass(VkCommandBuffer commandBuffer);

    private:
        // Command buffers
        void createCommandBuffers();
        void freeCommandBuffers();

        // Swapchain
        void recreateSwapChain();

        // Geometry resources
        void createGeometryResources();
        void createGeometryRenderPass();
        void createGeometryFramebuffers();
        void cleanupGeometryResources();

        RaptorWindow& rapWindow;
        RaptorDevice& rapDevice;

        std::unique_ptr<RaptorSwapChain> rapSwapChain;
        std::vector<VkCommandBuffer> commandBuffers;

        // Geometry render pass and per-frame G-buffers
        VkRenderPass geometryRenderPass{ VK_NULL_HANDLE };

        std::vector<std::unique_ptr<RaptorGeometryBuffer>>
            geometryBuffers;

        std::vector<VkFramebuffer> geometryFramebuffers;

        uint32_t currentImageIndex{};
        int currentFrameIndex{};
        bool isFrameStarted{ false };
    };

} // namespace rap