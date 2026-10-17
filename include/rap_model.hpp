#pragma once

#include "rap_device.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include "rap_buffer.hpp"

// std
#include <memory>
#include <vector>
#include <string>

namespace rap {

    class RaptorModel {
    public:

        struct Vertex {
            glm::vec3 position{};
            glm::vec3 color{};
            glm::vec3 normal{};
            glm::vec2 uv{};

            static std::vector<VkVertexInputBindingDescription>
                getBindingDescriptions();

            static std::vector<VkVertexInputAttributeDescription>
                getAttributeDescriptions();

            bool operator==(const Vertex& other) const {
                return position == other.position &&
                    color == other.color &&
                    normal == other.normal &&
                    uv == other.uv;
            }
        };

        struct Builder {
            std::vector<Vertex> vertices{};
            std::vector<uint32_t> indices{};

            void loadModel(
                const std::string& filepath);
        };

        RaptorModel(
            RaptorDevice& device,
            const RaptorModel::Builder& builder);

        ~RaptorModel();

        RaptorModel(const RaptorModel&) = delete;

        RaptorModel& operator=(
            const RaptorModel&) = delete;

        static std::unique_ptr<RaptorModel> createModelFromFile(
            RaptorDevice& device,
            const std::string& filepath);

        void bind(
            VkCommandBuffer commandBuffer);

        void draw(
            VkCommandBuffer commandBuffer);

    private:

        void createVertexBuffer(
            const std::vector<Vertex>& vertices);

        void createIndexBuffer(
            const std::vector<uint32_t>& indices);

        RaptorDevice& device;

        std::unique_ptr<RapBuffer> vertexBuffer;
        std::unique_ptr<RapBuffer> indexBuffer;

        uint32_t vertexCount = 0;
        uint32_t indexCount = 0;

        bool hasIndexBuffer = false;
    };

} // namespace rap