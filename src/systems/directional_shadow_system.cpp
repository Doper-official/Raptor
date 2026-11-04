#include "systems/directional_shadow_system.hpp"

#include "rap_model.hpp"
#include "rap_swap_chain.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>

#include <array>
#include <cassert>
#include <cmath>
#include <stdexcept>

namespace rap {
    namespace {
        struct ShadowPushConstants {
            glm::mat4 lightViewProjection{ 1.f };
            glm::mat4 modelMatrix{ 1.f };
        };
        static_assert(sizeof(ShadowPushConstants) == 128);
    }

    DirectionalShadowSystem::DirectionalShadowSystem(RaptorDevice& device)
        : device{ device } {
        createShadowImages();
        createRenderPass();
        createFramebuffers();
        createPipelineLayout();
        createPipeline();
    }

    DirectionalShadowSystem::~DirectionalShadowSystem() {
        vkDeviceWaitIdle(device.device());
        cleanup();
    }

    glm::mat4 DirectionalShadowSystem::makeLightSpaceMatrix(
        const glm::vec3& rayDirection,
        const glm::vec3& sceneCenter) {

        const glm::vec3 rayDir = glm::normalize(rayDirection);

        // Rays travel from the sun toward the scene. Put the light camera
        // on the opposite side, looking along the ray direction.
        const glm::vec3 lightPosition = sceneCenter - rayDir * 12.f;
        const glm::vec3 lightForward = glm::normalize(sceneCenter - lightPosition);
        glm::vec3 up{ 0.f, 1.f, 0.f };
        if (std::abs(glm::dot(lightForward, up)) > 0.98f) {
            up = { 0.f, 0.f, 1.f };
        }

        // This engine's camera convention is +Z-forward, so use the LH
        // look-at/orthographic pair here too. No mysterious Y world flip.
        const glm::mat4 lightView = glm::lookAtLH(
            lightPosition, sceneCenter, up);
        const glm::mat4 lightProjection = glm::orthoLH_ZO(
            -5.5f, 5.5f, -5.5f, 5.5f, 0.1f, 28.f);

        return lightProjection * lightView;
    }

    void DirectionalShadowSystem::createShadowImages() {
        depthFormat = device.findSupportedFormat(
            {
                VK_FORMAT_D32_SFLOAT,
                VK_FORMAT_D32_SFLOAT_S8_UINT,
                VK_FORMAT_D24_UNORM_S8_UINT
            },
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT |
                VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);

        shadowImages.reserve(RaptorSwapChain::MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < RaptorSwapChain::MAX_FRAMES_IN_FLIGHT; ++i) {
            shadowImages.push_back(std::make_unique<RaptorImage>(
                device,
                VkExtent2D{ SHADOW_MAP_SIZE, SHADOW_MAP_SIZE },
                depthFormat,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                    VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_IMAGE_ASPECT_DEPTH_BIT));
        }
    }

    void DirectionalShadowSystem::createRenderPass() {
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = depthFormat;
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout =
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

        VkAttachmentReference depthReference{};
        depthReference.attachment = 0;
        depthReference.layout =
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 0;
        subpass.pColorAttachments = nullptr;
        subpass.pDepthStencilAttachment = &depthReference;

        std::array<VkSubpassDependency, 2> dependencies{};
        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask =
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
            VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependencies[0].dstStageMask =
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
            VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependencies[0].dstAccessMask =
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependencies[0].dependencyFlags = 0;

        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask =
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
            VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependencies[1].dstStageMask =
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[1].srcAccessMask =
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependencies[1].dependencyFlags = 0;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &depthAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount =
            static_cast<uint32_t>(dependencies.size());
        renderPassInfo.pDependencies = dependencies.data();

        if (vkCreateRenderPass(
            device.device(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create directional shadow render pass!");
        }
    }

    void DirectionalShadowSystem::createFramebuffers() {
        framebuffers.resize(shadowImages.size(), VK_NULL_HANDLE);

        for (size_t i = 0; i < shadowImages.size(); ++i) {
            const VkImageView attachment = shadowImages[i]->getImageView();

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = &attachment;
            framebufferInfo.width = SHADOW_MAP_SIZE;
            framebufferInfo.height = SHADOW_MAP_SIZE;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(
                device.device(), &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create directional shadow framebuffer!");
            }
        }
    }

    void DirectionalShadowSystem::createPipelineLayout() {
        VkPushConstantRange pushRange{};
        pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushRange.offset = 0;
        pushRange.size = sizeof(ShadowPushConstants);

        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.pushConstantRangeCount = 1;
        layoutInfo.pPushConstantRanges = &pushRange;

        if (vkCreatePipelineLayout(
            device.device(), &layoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create directional shadow pipeline layout!");
        }
    }

    void DirectionalShadowSystem::createPipeline() {
        RaptorPipelineConfigInfo config{};
        RaptorPipeline::defaultPipelineConfigInfo(config);

        config.bindingDescriptions = RaptorModel::Vertex::getBindingDescriptions();
        config.attributeDescriptions = RaptorModel::Vertex::getAttributeDescriptions();
        config.attributeDescriptions.resize(1); // depth pass only needs position
        config.renderPass = renderPass;
        config.pipelineLayout = pipelineLayout;
        config.depthOnly = true;
        config.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
        config.rasterizationInfo.depthBiasEnable = VK_TRUE;
        config.rasterizationInfo.depthBiasConstantFactor = 1.25f;
        config.rasterizationInfo.depthBiasSlopeFactor = 1.75f;
        config.depthStencilInfo.depthTestEnable = VK_TRUE;
        config.depthStencilInfo.depthWriteEnable = VK_TRUE;
        config.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;

        pipeline = std::make_unique<RaptorPipeline>(
            device,
            "shaders/shadow_map.vert.spv",
            "shaders/shadow_map.frag.spv",
            config);
    }

    VkDescriptorImageInfo DirectionalShadowSystem::getDescriptorInfo(
        int frameIndex) const {
        assert(frameIndex >= 0 && frameIndex < static_cast<int>(shadowImages.size()));
        const auto& image = shadowImages[static_cast<size_t>(frameIndex)];
        return {
            image->getSampler(),
            image->getImageView(),
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
        };
    }

    void DirectionalShadowSystem::render(
        FrameInfo& frameInfo,
        const glm::mat4& lightViewProjection) {

        assert(frameInfo.frameIndex >= 0 &&
            frameInfo.frameIndex < static_cast<int>(framebuffers.size()));

        VkCommandBuffer commandBuffer = frameInfo.commandBuffer;
        VkClearValue clearValue{};
        clearValue.depthStencil = { 1.f, 0 };

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = framebuffers[frameInfo.frameIndex];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = { SHADOW_MAP_SIZE, SHADOW_MAP_SIZE };
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearValue;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport{};
        viewport.width = static_cast<float>(SHADOW_MAP_SIZE);
        viewport.height = static_cast<float>(SHADOW_MAP_SIZE);
        viewport.minDepth = 0.f;
        viewport.maxDepth = 1.f;

        VkRect2D scissor{};
        scissor.extent = { SHADOW_MAP_SIZE, SHADOW_MAP_SIZE };
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        pipeline->bind(commandBuffer);

        for (auto& [id, object] : frameInfo.gameObjects) {
            (void)id;
            if (!object.model) {
                continue;
            }

            ShadowPushConstants push{};
            push.lightViewProjection = lightViewProjection;
            push.modelMatrix = object.transform.mat4();

            vkCmdPushConstants(
                commandBuffer,
                pipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT,
                0,
                sizeof(ShadowPushConstants),
                &push);

            object.model->bind(commandBuffer);

            const auto& primitives = object.model->getPrimitives();
            if (primitives.empty()) {
                object.model->draw(commandBuffer);
            }
            else {
                for (const auto& primitive : primitives) {
                    object.model->drawPrimitive(
                        commandBuffer,
                        primitive.firstIndex,
                        primitive.indexCount);
                }
            }
        }

        vkCmdEndRenderPass(commandBuffer);
    }

    void DirectionalShadowSystem::cleanup() {
        pipeline.reset();

        if (pipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
            pipelineLayout = VK_NULL_HANDLE;
        }

        for (VkFramebuffer framebuffer : framebuffers) {
            if (framebuffer != VK_NULL_HANDLE) {
                vkDestroyFramebuffer(device.device(), framebuffer, nullptr);
            }
        }
        framebuffers.clear();

        if (renderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(device.device(), renderPass, nullptr);
            renderPass = VK_NULL_HANDLE;
        }

        // Framebuffers/render pass go away before the depth image views.
        shadowImages.clear();
    }

} // namespace rap
