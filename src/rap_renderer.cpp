#include "rap_renderer.hpp"

// My libs
#include "buffers/rap_geometry_buffer.hpp"

// Standard library
#include <array>
#include <cassert>
#include <stdexcept>
#include <utility>

namespace rap {

    RaptorRenderer::RaptorRenderer(
        RaptorWindow& window,
        RaptorDevice& device)
        : rapWindow{ window },
        rapDevice{ device } {

        recreateSwapChain();
        createCommandBuffers();
    }

    RaptorRenderer::~RaptorRenderer() {
        vkDeviceWaitIdle(rapDevice.device());

        cleanupGeometryResources();
        freeCommandBuffers();
    }

    void RaptorRenderer::recreateSwapChain() {
        auto extent = rapWindow.getExtent();

        while (extent.width == 0 || extent.height == 0) {
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
            std::shared_ptr<RaptorSwapChain> oldSwapChain =
                std::move(rapSwapChain);

            rapSwapChain =
                std::make_unique<RaptorSwapChain>(
                    rapDevice,
                    extent,
                    oldSwapChain);

            if (!oldSwapChain->compareSwapFormats(*rapSwapChain)) {
                throw std::runtime_error(
                    "Swap chain image format (or depth format) has changed!");
            }
        }

        // This must run on initial creation AND recreation.
        cleanupGeometryResources();
        createGeometryResources();
    }

    void RaptorRenderer::createCommandBuffers() {
        commandBuffers.resize(
            RaptorSwapChain::MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfo{};

        allocInfo.sType =
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;

        allocInfo.level =
            VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        allocInfo.commandPool =
            rapDevice.getCommandPool();

        allocInfo.commandBufferCount =
            static_cast<uint32_t>(commandBuffers.size());

        if (vkAllocateCommandBuffers(
            rapDevice.device(),
            &allocInfo,
            commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error(
                "Failed to allocate command buffers!");
        }
    }

    void RaptorRenderer::freeCommandBuffers() {
        if (commandBuffers.empty()) {
            return;
        }

        vkFreeCommandBuffers(
            rapDevice.device(),
            rapDevice.getCommandPool(),
            static_cast<uint32_t>(commandBuffers.size()),
            commandBuffers.data());

        commandBuffers.clear();
    }

    VkCommandBuffer RaptorRenderer::beginFrame() {
        assert(
            !isFrameStarted &&
            "Cannot call beginFrame while a frame is already in progress!");

        const auto result =
            rapSwapChain->acquireNextImage(&currentImageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            return VK_NULL_HANDLE;
        }

        if (result != VK_SUCCESS &&
            result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error(
                "Failed to acquire swap chain image!");
        }

        isFrameStarted = true;

        const auto commandBuffer = getCurrentCommandBuffer();

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType =
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(
            commandBuffer,
            &beginInfo) != VK_SUCCESS) {
            isFrameStarted = false;

            throw std::runtime_error(
                "Failed to begin recording command buffer!");
        }

        return commandBuffer;
    }

    void RaptorRenderer::endFrame() {
        assert(
            isFrameStarted &&
            "Cannot call endFrame when a frame is not in progress!");

        const auto commandBuffer = getCurrentCommandBuffer();

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error(
                "Failed to end recording command buffer!");
        }

        const auto result =
            rapSwapChain->submitCommandBuffers(
                &commandBuffer,
                &currentImageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR ||
            result == VK_SUBOPTIMAL_KHR ||
            rapWindow.wasWindowResized()) {

            rapWindow.resetWindowResizedFlag();
            isFrameStarted = false;

            recreateSwapChain();
        }
        else if (result != VK_SUCCESS) {
            isFrameStarted = false;

            throw std::runtime_error(
                "Failed to present swap chain image!");
        }

        isFrameStarted = false;

        currentFrameIndex =
            (currentFrameIndex + 1) %
            RaptorSwapChain::MAX_FRAMES_IN_FLIGHT;
    }

    // ------------------------------------------------------------
    // Geometry resources
    // ------------------------------------------------------------

    void RaptorRenderer::createGeometryResources() {
        geometryBuffers.clear();

        geometryBuffers.reserve(
            RaptorSwapChain::MAX_FRAMES_IN_FLIGHT);

        const VkExtent2D extent =
            rapSwapChain->getSwapChainExtent();

        for (int i = 0;
            i < RaptorSwapChain::MAX_FRAMES_IN_FLIGHT;
            ++i) {

            geometryBuffers.push_back(
                std::make_unique<RaptorGeometryBuffer>(
                    rapDevice,
                    extent));
        }

        createGeometryRenderPass();
        createGeometryFramebuffers();
    }

    void RaptorRenderer::createGeometryRenderPass() {
        assert(
            !geometryBuffers.empty() &&
            "Cannot create geometry render pass without G-buffers!");

        std::array<VkAttachmentDescription, 3> attachments{};

        // Albedo attachment.
        attachments[0].format = VK_FORMAT_R8G8B8A8_UNORM;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout =
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        // Normal attachment.
        attachments[1].format = VK_FORMAT_R16G16B16A16_SFLOAT;
        attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[1].finalLayout =
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        // Depth attachment.
        attachments[2].format =
            geometryBuffers[0]->getDepthFormat();
        attachments[2].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[2].finalLayout =
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

        std::array<VkAttachmentReference, 2> colorReferences{};

        colorReferences[0].attachment = 0;
        colorReferences[0].layout =
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        colorReferences[1].attachment = 1;
        colorReferences[1].layout =
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthReference{};
        depthReference.attachment = 2;
        depthReference.layout =
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint =
            VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount =
            static_cast<uint32_t>(colorReferences.size());
        subpass.pColorAttachments = colorReferences.data();
        subpass.pDepthStencilAttachment = &depthReference;

        std::array<VkSubpassDependency, 2> dependencies{};

        // External work -> geometry pass.
        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask =
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
            VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependencies[0].dstStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependencies[0].srcAccessMask =
            VK_ACCESS_SHADER_READ_BIT |
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependencies[0].dstAccessMask =
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependencies[0].dependencyFlags =
            VK_DEPENDENCY_BY_REGION_BIT;

        // Geometry pass -> subsequent shader reads.
        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
            VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependencies[1].dstStageMask =
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[1].srcAccessMask =
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependencies[1].dependencyFlags =
            VK_DEPENDENCY_BY_REGION_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType =
            VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount =
            static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount =
            static_cast<uint32_t>(dependencies.size());
        renderPassInfo.pDependencies = dependencies.data();

        if (vkCreateRenderPass(
            rapDevice.device(),
            &renderPassInfo,
            nullptr,
            &geometryRenderPass) != VK_SUCCESS) {
            throw std::runtime_error(
                "Failed to create Raptor geometry render pass!");
        }
    }

    void RaptorRenderer::createGeometryFramebuffers() {
        assert(
            geometryRenderPass != VK_NULL_HANDLE &&
            "Geometry render pass has not been created!");

        geometryFramebuffers.resize(geometryBuffers.size(),
            VK_NULL_HANDLE);

        for (size_t i = 0; i < geometryBuffers.size(); ++i) {
            std::array<VkImageView, 3> attachments{};

            attachments[0] =
                geometryBuffers[i]->getAlbedoImageView();
            attachments[1] =
                geometryBuffers[i]->getNormalImageView();
            attachments[2] =
                geometryBuffers[i]->getDepthImageView();

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType =
                VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = geometryRenderPass;
            framebufferInfo.attachmentCount =
                static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width =
                geometryBuffers[i]->getExtent().width;
            framebufferInfo.height =
                geometryBuffers[i]->getExtent().height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(
                rapDevice.device(),
                &framebufferInfo,
                nullptr,
                &geometryFramebuffers[i]) != VK_SUCCESS) {

                throw std::runtime_error(
                    "Failed to create Raptor geometry framebuffer!");
            }
        }
    }

    void RaptorRenderer::cleanupGeometryResources() {
        for (VkFramebuffer framebuffer : geometryFramebuffers) {
            if (framebuffer != VK_NULL_HANDLE) {
                vkDestroyFramebuffer(
                    rapDevice.device(),
                    framebuffer,
                    nullptr);
            }
        }

        geometryFramebuffers.clear();

        if (geometryRenderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(
                rapDevice.device(),
                geometryRenderPass,
                nullptr);

            geometryRenderPass = VK_NULL_HANDLE;
        }

        // Destroy images only after framebuffers and render pass.
        geometryBuffers.clear();
    }

    // ------------------------------------------------------------
    // Geometry pass commands
    // ------------------------------------------------------------

    void RaptorRenderer::beginGeometryPass(
        VkCommandBuffer commandBuffer) {

        assert(
            isFrameStarted &&
            "Cannot begin geometry pass outside a frame!");

        assert(
            commandBuffer == getCurrentCommandBuffer() &&
            "Geometry pass command buffer belongs to another frame!");

        assert(
            geometryRenderPass != VK_NULL_HANDLE &&
            "Geometry render pass has not been initialized!");

        const VkExtent2D extent =
            geometryBuffers[currentFrameIndex]->getExtent();

        std::array<VkClearValue, 3> clearValues{};

        clearValues[0].color =
        { { 0.0f, 0.0f, 0.0f, 1.0f } };

        clearValues[1].color =
        { { 0.0f, 0.0f, 0.0f, 1.0f } };

        clearValues[2].depthStencil =
        { 1.0f, 0 };

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType =
            VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = geometryRenderPass;
        renderPassInfo.framebuffer =
            geometryFramebuffers[currentFrameIndex];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = extent;
        renderPassInfo.clearValueCount =
            static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(
            commandBuffer,
            &renderPassInfo,
            VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(extent.width);
        viewport.height = static_cast<float>(extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = extent;

        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    }

    void RaptorRenderer::endGeometryPass(
        VkCommandBuffer commandBuffer) {

        assert(
            isFrameStarted &&
            "Cannot end geometry pass outside a frame!");

        assert(
            commandBuffer == getCurrentCommandBuffer() &&
            "Geometry pass command buffer belongs to another frame!");

        vkCmdEndRenderPass(commandBuffer);
    }

    // ------------------------------------------------------------
    // Existing swapchain render pass
    // ------------------------------------------------------------

    void RaptorRenderer::beginSwapChainRenderPass(
        VkCommandBuffer commandBuffer) {

        assert(
            isFrameStarted &&
            "Cannot begin swapchain render pass outside a frame!");

        assert(
            commandBuffer == getCurrentCommandBuffer() &&
            "Swapchain command buffer belongs to another frame!");

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType =
            VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

        renderPassInfo.renderPass =
            rapSwapChain->getRenderPass();

        renderPassInfo.framebuffer =
            rapSwapChain->getFrameBuffer(currentImageIndex);

        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent =
            rapSwapChain->getSwapChainExtent();

        std::array<VkClearValue, 2> clearValues{};

        clearValues[0].color =
        { { 0.0f, 0.0f, 0.0f, 1.0f } };

        clearValues[1].depthStencil =
        { 1.0f, 0 };

        renderPassInfo.clearValueCount =
            static_cast<uint32_t>(clearValues.size());

        renderPassInfo.pClearValues =
            clearValues.data();

        vkCmdBeginRenderPass(
            commandBuffer,
            &renderPassInfo,
            VK_SUBPASS_CONTENTS_INLINE);

        const VkExtent2D extent =
            rapSwapChain->getSwapChainExtent();

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(extent.width);
        viewport.height = static_cast<float>(extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = extent;

        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    }

    void RaptorRenderer::endSwapChainRenderPass(
        VkCommandBuffer commandBuffer) {

        assert(
            isFrameStarted &&
            "Cannot end swapchain render pass outside a frame!");

        assert(
            commandBuffer == getCurrentCommandBuffer() &&
            "Swapchain command buffer belongs to another frame!");

        vkCmdEndRenderPass(commandBuffer);
    }

} // namespace rap