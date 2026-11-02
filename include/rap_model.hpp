#pragma once

#include "rap_device.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include "buffers/rap_buffer.hpp"
#include "rap_image.hpp"

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

        struct TextureData {
            uint32_t width = 0;
            uint32_t height = 0;
            std::vector<unsigned char> pixels;
        };

        struct Primitive {
            uint32_t firstIndex = 0;
            uint32_t indexCount = 0;
            int32_t materialIndex = -1;
        };

        struct Material {
            int32_t baseColorTextureIndex = -1;
        };

        struct Builder {
            std::vector<Vertex> vertices{};
            std::vector<uint32_t> indices{};
            std::vector<TextureData> textures{};
            std::vector<Primitive> primitive{};
            std::vector<Material> materials{};

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

		const std::vector<Primitive>& getPrimitives() const {
			return primitives;
		}

		const std::vector<Material>& getMaterials() const {
			return materials;
		}

        size_t getTextureCount() const {
            return textures.size();
        }

        VkDescriptorImageInfo getTextureDescriptorInfo(
            size_t index) const;

        void drawPrimitive(
			VkCommandBuffer commandBuffer,
			uint32_t firstIndex,
			uint32_t indexCount);

    private:

        void createVertexBuffer(
            const std::vector<Vertex>& vertices);

        void createIndexBuffer(
            const std::vector<uint32_t>& indices);

        RaptorDevice& device;

        std::unique_ptr<RapBuffer> vertexBuffer;
        std::unique_ptr<RapBuffer> indexBuffer;
        std::vector<std::unique_ptr<RaptorImage>> textures;
        std::vector<Primitive> primitives;
		std::vector<Material> materials;

        uint32_t vertexCount = 0;
        uint32_t indexCount = 0;

        bool hasIndexBuffer = false;
    };

} // namespace rap