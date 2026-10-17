#pragma once

#include "rap_device.hpp"

// std
#include <string>
#include <vector>



namespace rap {

    struct RaptorPipelineConfigInfo {

        RaptorPipelineConfigInfo() = default;

        RaptorPipelineConfigInfo(
            const RaptorPipelineConfigInfo&) = delete;

        RaptorPipelineConfigInfo& operator=(
            const RaptorPipelineConfigInfo&) = delete;


        std::vector<VkVertexInputBindingDescription> bindingDescriptions{};
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
        VkPipelineViewportStateCreateInfo
            viewportInfo{};

        VkPipelineInputAssemblyStateCreateInfo
            inputAssemblyInfo{};

        VkPipelineRasterizationStateCreateInfo
            rasterizationInfo{};

        VkPipelineMultisampleStateCreateInfo
            multisampleInfo{};

        VkPipelineColorBlendAttachmentState
            colorBlendAttachment{};

        VkPipelineColorBlendStateCreateInfo
            colorBlendInfo{};

        VkPipelineDepthStencilStateCreateInfo
            depthStencilInfo{};

        std::vector<VkDynamicState>
            dynamicStateEnables{};

        VkPipelineDynamicStateCreateInfo
            dynamicStateInfo{};

        VkPipelineLayout
            pipelineLayout = VK_NULL_HANDLE;

        VkRenderPass
            renderPass = VK_NULL_HANDLE;

        uint32_t subpass = 0;
    };

    class RaptorPipeline {

    public:

        RaptorPipeline(
            RaptorDevice& device,
            const std::string& vertFilepath,
            const std::string& fragFilepath,
            const RaptorPipelineConfigInfo& configInfo);

        ~RaptorPipeline();

        RaptorPipeline(
            const RaptorPipeline&) = delete;

        RaptorPipeline& operator=(
            const RaptorPipeline&) = delete;

        void bind(
            VkCommandBuffer commandBuffer);

        static void defaultPipelineConfigInfo(
            RaptorPipelineConfigInfo& configInfo);

    private:

        static std::vector<char> readFile(
            const std::string& filepath);

        void createGraphicsPipeline(
            const std::string& vertFilepath,
            const std::string& fragFilepath,
            const RaptorPipelineConfigInfo& configInfo);

        void createShaderModule(
            const std::vector<char>& code,
            VkShaderModule* shaderModule);

        RaptorDevice& device;

        VkPipeline graphicsPipeline =
            VK_NULL_HANDLE;

        VkShaderModule vertShaderModule =
            VK_NULL_HANDLE;

        VkShaderModule fragShaderModule =
            VK_NULL_HANDLE;
    };


} // namespace rap
