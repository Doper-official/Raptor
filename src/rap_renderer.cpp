#include "rap_renderer.hpp"

// std
#include <array>
#include <cassert>
#include <stdexcept>

namespace rap {

    RaptorRenderer::RaptorRenderer(RaptorWindow& window, RaptorDevice& device)
        : rapWindow{ window }, rapDevice{ device } {
        recreateSwapChain();
        createCommandBuffers();
    }

    RaptorRenderer::~RaptorRenderer() {
        freeCommandBuffers();
    }

    void RaptorRenderer::recreateSwapChain() {
        auto extent = rapWindow.getExtent();

        while (extent.width == 0 ||
            extent.height == 0) {

            extent = rapWindow.getExtent();
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(rapDevice.device());

        if (rapSwapChain == nullptr) {

            rapSwapChain =
                std::make_unique<RaptorSwapChain>(
                    rapDevice,
                    extent);

        }
        else {
            std::shared_ptr<RaptorSwapChain> oldSwapChain = std::move(rapSwapChain);
            rapSwapChain =
                std::make_unique<RaptorSwapChain>(
                    rapDevice,
                    extent,
                    oldSwapChain);

            if (!oldSwapChain->compareSwapFormats(*rapSwapChain.get())) {
                throw std::runtime_error("Swap Chain image format (or depth) format has changed!");
            }

        }
    }

    void RaptorRenderer::createCommandBuffers() {
        commandBuffers.resize(RaptorSwapChain::MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfo{};

        allocInfo.sType =
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;

        allocInfo.level =
            VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        allocInfo.commandPool =
            rapDevice.getCommandPool();

        allocInfo.commandBufferCount =
            static_cast<uint32_t>(
                commandBuffers.size());

        if (vkAllocateCommandBuffers(
            rapDevice.device(),
            &allocInfo,
            commandBuffers.data()) != VK_SUCCESS) {

            throw std::runtime_error(
                "failed to allocate command buffers!");
        }
    }

    void RaptorRenderer::freeCommandBuffers() {
        vkFreeCommandBuffers(
            rapDevice.device(),
            rapDevice.getCommandPool(),
            static_cast<uint32_t>(
                commandBuffers.size()),
            commandBuffers.data());

        commandBuffers.clear();
    }

    VkCommandBuffer RaptorRenderer::beginFrame() {
        assert(
            !isFrameStarted &&
            "Cant call begin frame while already in progress");

        auto result =
            rapSwapChain->acquireNextImage(
                &currentImageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            return nullptr;
        }

        if (result != VK_SUCCESS &&
            result != VK_SUBOPTIMAL_KHR) {

            throw std::runtime_error(
                "failed to acquire swap chain image!");
        }

        isFrameStarted = true;

        auto commandBuffer =
            getCurrentCommandBuffer();

        VkCommandBufferBeginInfo beginInfo{};

        beginInfo.sType =
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(
            commandBuffer,
            &beginInfo) != VK_SUCCESS) {

            throw std::runtime_error(
                "failed to begin recording command buffer!");
        }

        return commandBuffer;
    }

    void RaptorRenderer::endFrame() {
        assert(
            isFrameStarted &&
            "Cant call endFrame while frame is in progress");

        auto commandBuffer =
            getCurrentCommandBuffer();

        if (vkEndCommandBuffer(
            commandBuffer) != VK_SUCCESS) {

            throw std::runtime_error(
                "failed to record command buffer!");
        }

        auto result =
            rapSwapChain->submitCommandBuffers(
                &commandBuffer,
                &currentImageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR ||
            result == VK_SUBOPTIMAL_KHR ||
            rapWindow.wasWindowResized()) {

            rapWindow.resetWindowResizedFlag();
            recreateSwapChain();

        }
        else if (result != VK_SUCCESS) {

            throw std::runtime_error(
                "failed to present swap chain image!");
        }

        isFrameStarted = false;
        currentFrameIndex = (currentFrameIndex + 1) % RaptorSwapChain::MAX_FRAMES_IN_FLIGHT;
    }

    void RaptorRenderer::beginSwapChainRenderPass(
        VkCommandBuffer commandBuffer) {

        assert(
            isFrameStarted &&
            "Cant call beginSwapChainRenderPass if frame is not in progress");

        assert(
            commandBuffer == getCurrentCommandBuffer() &&
            "Cant begin render pass on command buffer from a different frame");

        VkRenderPassBeginInfo renderPassInfo{};

        renderPassInfo.sType =
            VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

        renderPassInfo.renderPass =
            rapSwapChain->getRenderPass();

        renderPassInfo.framebuffer =
            rapSwapChain->getFrameBuffer(currentImageIndex);

        renderPassInfo.renderArea.offset =
        { 0, 0 };

        renderPassInfo.renderArea.extent =
            rapSwapChain->getSwapChainExtent();

        std::array<VkClearValue, 2> clearValues{};

        clearValues[0].color =
        { 0.f, 0.f, 0.f, 1.f };

        clearValues[1].depthStencil =
        { 1.0f, 0 };

        renderPassInfo.clearValueCount =
            static_cast<uint32_t>(
                clearValues.size());

        renderPassInfo.pClearValues =
            clearValues.data();

        vkCmdBeginRenderPass(
            commandBuffer,
            &renderPassInfo,
            VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport{};

        viewport.x = 0.0f;
        viewport.y = 0.0f;

        viewport.width =
            static_cast<float>(
                rapSwapChain->getSwapChainExtent().width);

        viewport.height =
            static_cast<float>(
                rapSwapChain->getSwapChainExtent().height);

        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{
            {0, 0},
            rapSwapChain->getSwapChainExtent()
        };

        vkCmdSetViewport(
            commandBuffer,
            0,
            1,
            &viewport);

        vkCmdSetScissor(
            commandBuffer,
            0,
            1,
            &scissor);
    }

    void RaptorRenderer::endSwapChainRenderPass(
        VkCommandBuffer commandBuffer) {

        assert(
            isFrameStarted &&
            "Cant call endSwapChainRenderPass if frame is not in progress");

        assert(
            commandBuffer == getCurrentCommandBuffer() &&
            "Cant end render pass on command buffer from a different frame");

        vkCmdEndRenderPass(commandBuffer);
    }

} // namespace rap