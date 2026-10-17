#include "rap_model.hpp"

#include "rap_utils.hpp"

// libs
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

// std
#include <cassert>
#include <cstddef>
#include <cstring>
#include <unordered_map>

namespace std {
    template <>
    struct hash<rap::RaptorModel::Vertex> {
        size_t operator()(rap::RaptorModel::Vertex const& vertex) const {
            size_t seed = 0;

            rap::hashCombine(
                seed,
                vertex.position,
                vertex.color,
                vertex.normal,
                vertex.uv
            );

            return seed;
        }
    };
}


namespace rap {

    RaptorModel::RaptorModel(
        RaptorDevice& device,
        const RaptorModel::Builder& builder)
        : device{ device } {

        createVertexBuffer(builder.vertices);
        createIndexBuffer(builder.indices);
    }


    RaptorModel::~RaptorModel() {}


    std::unique_ptr<RaptorModel> RaptorModel::createModelFromFile(
        RaptorDevice& device,
        const std::string& filepath) {

        Builder builder{};
        builder.loadModel(filepath);

        return std::make_unique<RaptorModel>(
            device,
            builder
        );
    }


    void RaptorModel::createVertexBuffer(
        const std::vector<Vertex>& vertices) {

        vertexCount =
            static_cast<uint32_t>(
                vertices.size());

        assert(
            vertexCount >= 3 &&
            "Vertex count must be atleast 3"
        );

        VkDeviceSize bufferSize =
            sizeof(Vertex) * vertexCount;

        uint32_t vertexSize =
            sizeof(vertices[0]);


        // Staging buffer

        RapBuffer stagingBuffer{
            device,
            vertexSize,
            vertexCount,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        };

        stagingBuffer.map();

        stagingBuffer.writeToBuffer(
            (void*)vertices.data()
        );


        // Vertex buffer

        vertexBuffer =
            std::make_unique<RapBuffer>(
                device,
                vertexSize,
                vertexCount,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            );


        device.copyBuffer(
            stagingBuffer.getBuffer(),
            vertexBuffer->getBuffer(),
            bufferSize
        );
    }


    void RaptorModel::createIndexBuffer(
        const std::vector<uint32_t>& indices) {

        indexCount =
            static_cast<uint32_t>(
                indices.size());

        hasIndexBuffer =
            indexCount > 0;

        if (!hasIndexBuffer) {
            return;
        }


        VkDeviceSize bufferSize =
            sizeof(indices[0]) * indexCount;

        uint32_t indexSize =
            sizeof(indices[0]);


        // Staging buffer

        RapBuffer stagingBuffer{
            device,
            indexSize,
            indexCount,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        };

        stagingBuffer.map();

        stagingBuffer.writeToBuffer(
            (void*)indices.data()
        );


        // Index buffer

        indexBuffer =
            std::make_unique<RapBuffer>(
                device,
                indexSize,
                indexCount,
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            );


        device.copyBuffer(
            stagingBuffer.getBuffer(),
            indexBuffer->getBuffer(),
            bufferSize
        );
    }


    void RaptorModel::bind(
        VkCommandBuffer commandBuffer) {

        VkBuffer buffers[] = {
            vertexBuffer->getBuffer()
        };

        VkDeviceSize offsets[] = {
            0
        };


        // Bind vertex buffer

        vkCmdBindVertexBuffers(
            commandBuffer,
            0,
            1,
            buffers,
            offsets
        );


        // Bind index buffer

        if (hasIndexBuffer) {

            vkCmdBindIndexBuffer(
                commandBuffer,
                indexBuffer->getBuffer(),
                0,
                VK_INDEX_TYPE_UINT32
            );
        }
    }


    void RaptorModel::draw(
        VkCommandBuffer commandBuffer) {

        if (hasIndexBuffer) {

            vkCmdDrawIndexed(
                commandBuffer,
                indexCount,
                1,
                0,
                0,
                0
            );
        }
        else {

            vkCmdDraw(
                commandBuffer,
                vertexCount,
                1,
                0,
                0
            );
        }
    }


    std::vector<VkVertexInputBindingDescription>
        RaptorModel::Vertex::getBindingDescriptions() {

        std::vector<VkVertexInputBindingDescription>
            bindingDescriptions(1);

        bindingDescriptions[0].binding = 0;

        bindingDescriptions[0].stride =
            sizeof(Vertex);

        bindingDescriptions[0].inputRate =
            VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescriptions;
    }


    std::vector<VkVertexInputAttributeDescription>
        RaptorModel::Vertex::getAttributeDescriptions() {

        std::vector<VkVertexInputAttributeDescription>
            attributeDescriptions{};

        attributeDescriptions.push_back({
            0,
            0,
            VK_FORMAT_R32G32B32_SFLOAT,
            offsetof(Vertex, position)
            });

        attributeDescriptions.push_back({
            1,
            0,
            VK_FORMAT_R32G32B32_SFLOAT,
            offsetof(Vertex, color)
            });

        attributeDescriptions.push_back({
            2,
            0,
            VK_FORMAT_R32G32B32_SFLOAT,
            offsetof(Vertex, normal)
            });

        attributeDescriptions.push_back({
            3,
            0,
            VK_FORMAT_R32G32_SFLOAT,
            offsetof(Vertex, uv)
            });

        return attributeDescriptions;
    }


    void RaptorModel::Builder::loadModel(
        const std::string& filepath) {

        tinyobj::attrib_t attrib;

        std::vector<tinyobj::shape_t> shapes;

        std::vector<tinyobj::material_t> materials;

        std::string warn;
        std::string err;


        if (!tinyobj::LoadObj(
            &attrib,
            &shapes,
            &materials,
            &warn,
            &err,
            filepath.c_str())) {

            throw std::runtime_error(
                warn + err
            );
        }


        vertices.clear();
        indices.clear();


        std::unordered_map<
            Vertex,
            uint32_t
        > uniqueVertices{};


        for (const auto& shape : shapes) {

            for (const auto& index :
                shape.mesh.indices) {

                Vertex vertex{};


                // Position

                if (index.vertex_index >= 0) {

                    vertex.position = {
                        attrib.vertices[
                            3 * index.vertex_index + 0
                        ],

                        attrib.vertices[
                            3 * index.vertex_index + 1
                        ],

                        attrib.vertices[
                            3 * index.vertex_index + 2
                        ]
                    };


                    // Vertex color

                    if (attrib.colors.size() >=
                        3 * (index.vertex_index + 1)) {

                        vertex.color = {
                            attrib.colors[
                                3 * index.vertex_index + 0
                            ],

                            attrib.colors[
                                3 * index.vertex_index + 1
                            ],

                            attrib.colors[
                                3 * index.vertex_index + 2
                            ]
                        };
                    }
                    else {

                        vertex.color = {
                            1.0f,
                            1.0f,
                            1.0f
                        };
                    }
                }


                // Normal

                if (index.normal_index >= 0) {

                    vertex.normal = {
                        attrib.normals[
                            3 * index.normal_index + 0
                        ],

                        attrib.normals[
                            3 * index.normal_index + 1
                        ],

                        attrib.normals[
                            3 * index.normal_index + 2
                        ]
                    };
                }


                // UV

                if (index.texcoord_index >= 0) {

                    vertex.uv = {
                        attrib.texcoords[
                            2 * index.texcoord_index + 0
                        ],

                        attrib.texcoords[
                            2 * index.texcoord_index + 1
                        ]
                    };
                }


                // Unique vertex

                if (uniqueVertices.count(vertex) == 0) {

                    uniqueVertices[vertex] =
                        static_cast<uint32_t>(
                            vertices.size());

                    vertices.push_back(vertex);
                }


                // Index

                indices.push_back(
                    uniqueVertices[vertex]
                );
            }
        }
    }

} // namespace rap