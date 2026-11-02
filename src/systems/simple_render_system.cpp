#include "systems/simple_render_system.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <cassert>
#include <stdexcept>
#include <vector>

namespace rap {

    struct SimplePushConstantData {
        glm::mat4 modelMatrix{ 1.f };
        glm::mat4 normalMatrix{ 1.f };
    };


    SimpleRenderSystem::SimpleRenderSystem(
        RaptorDevice& device,
        VkRenderPass renderPass,
        VkDescriptorSetLayout globalSetLayout,
        VkDescriptorSetLayout textureSetLayout)
        : device{ device } {

        createPipelineLayout(
            globalSetLayout,
            textureSetLayout
        );

        createPipeline(renderPass);
    }


    SimpleRenderSystem::~SimpleRenderSystem() {
        vkDeviceWaitIdle(device.device());

        vkDestroyPipelineLayout(
            device.device(),
            pipelineLayout,
            nullptr
        );
    }


    void SimpleRenderSystem::createPipelineLayout(
        VkDescriptorSetLayout globalSetLayout,
        VkDescriptorSetLayout textureSetLayout) {

        VkPushConstantRange pushConstantRange{};

        pushConstantRange.stageFlags =
            VK_SHADER_STAGE_VERTEX_BIT |
            VK_SHADER_STAGE_FRAGMENT_BIT;

        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(SimplePushConstantData);

        std::vector<VkDescriptorSetLayout> descriptorSetLayouts{
            globalSetLayout,
            textureSetLayout
        };

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};

        pipelineLayoutInfo.sType =
            VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

        pipelineLayoutInfo.setLayoutCount =
            static_cast<uint32_t>(descriptorSetLayouts.size());

        pipelineLayoutInfo.pSetLayouts =
            descriptorSetLayouts.data();

        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges =
            &pushConstantRange;

        if (vkCreatePipelineLayout(
            device.device(),
            &pipelineLayoutInfo,
            nullptr,
            &pipelineLayout) != VK_SUCCESS) {

            throw std::runtime_error(
                "failed to create pipeline layout!"
            );
        }
    }


    void SimpleRenderSystem::createPipeline(
        VkRenderPass renderPass) {

        assert(
            pipelineLayout != VK_NULL_HANDLE &&
            "Pipeline layout must exist before creating the pipeline!"
        );

        RaptorPipelineConfigInfo pipelineConfig{};

        RaptorPipeline::defaultPipelineConfigInfo(pipelineConfig);

        pipelineConfig.renderPass = renderPass;
        pipelineConfig.pipelineLayout = pipelineLayout;

        // G-buffer has TWO color attachments:
        // 0 = albedo, 1 = world-space normal.
        pipelineConfig.colorBlendAttachments = {
            pipelineConfig.colorBlendAttachment,
            pipelineConfig.colorBlendAttachment
        };

        pipeline = std::make_unique<RaptorPipeline>(
            device,
            "shaders/simple_shader.vert.spv",
            "shaders/simple_shader.frag.spv",
            pipelineConfig
        );
    }


    void SimpleRenderSystem::renderGameObjects(
        FrameInfo& frameInfo) {

        pipeline->bind(frameInfo.commandBuffer);

        vkCmdBindDescriptorSets(
            frameInfo.commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout,
            0,
            1,
            &frameInfo.globalDescriptorSet,
            0,
            nullptr
        );

        for (auto& kv : frameInfo.gameObjects) {
            auto& obj = kv.second;

            if (obj.model == nullptr) {
                continue;
            }

            SimplePushConstantData push{};

            push.modelMatrix = obj.transform.mat4();
            push.normalMatrix = glm::mat4{ obj.transform.normalMatrix() };

            vkCmdPushConstants(
                frameInfo.commandBuffer,
                pipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT |
                VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(SimplePushConstantData),
                &push
            );

            obj.model->bind(frameInfo.commandBuffer);

            const auto& primitives = obj.model->getPrimitives();
            const auto& materials = obj.model->getMaterials();

            auto bindTextureSet = [&](VkDescriptorSet textureSet) {
                // The pipeline layout always expects set 1, so never
                // draw without one.
                if (textureSet == VK_NULL_HANDLE) {
                    textureSet = obj.defaultTextureDescriptorSet;
                }

                if (textureSet == VK_NULL_HANDLE) {
                    return;
                }

                vkCmdBindDescriptorSets(
                    frameInfo.commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelineLayout,
                    1,
                    1,
                    &textureSet,
                    0,
                    nullptr
                );
            };

            // Models without primitive metadata (OBJ) use the full-model draw
            // with the white default texture.
            if (primitives.empty()) {
                bindTextureSet(VK_NULL_HANDLE);
                obj.model->draw(frameInfo.commandBuffer);
                continue;
            }

            // GLB models can have a different material on each primitive.
            for (const auto& primitive : primitives) {
                VkDescriptorSet textureSet = VK_NULL_HANDLE;

                if (primitive.materialIndex >= 0 &&
                    static_cast<size_t>(primitive.materialIndex) <
                    materials.size()) {

                    const int32_t textureIndex =
                        materials[primitive.materialIndex]
                        .baseColorTextureIndex;

                    if (textureIndex >= 0 &&
                        static_cast<size_t>(textureIndex) <
                        obj.textureDescriptorSets.size()) {

                        textureSet =
                            obj.textureDescriptorSets[textureIndex];
                    }
                }

                // Falls back to the white default texture if none.
                bindTextureSet(textureSet);

                obj.model->drawPrimitive(
                    frameInfo.commandBuffer,
                    primitive.firstIndex,
                    primitive.indexCount
                );
            }
        }
    }

} // namespace rap