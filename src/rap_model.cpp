#include "rap_model.hpp"
#include "rap_utils.hpp"

// Libraries
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <tiny_gltf.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

// Standard library
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

    struct FloatAccessorData {
        const unsigned char* data = nullptr;
        std::size_t count = 0;
        std::size_t stride = 0;
    };


    [[noreturn]] void glbError(
        const std::string& message) {

        throw std::runtime_error(
            "GLB loader: " + message
        );
    }


    std::size_t getComponentSize(
        int componentType) {

        switch (componentType) {

        case TINYGLTF_COMPONENT_TYPE_BYTE:
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            return 1;

        case TINYGLTF_COMPONENT_TYPE_SHORT:
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            return 2;

        case TINYGLTF_COMPONENT_TYPE_INT:
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
        case TINYGLTF_COMPONENT_TYPE_FLOAT:
            return 4;

        case TINYGLTF_COMPONENT_TYPE_DOUBLE:
            return 8;

        default:
            glbError("Unsupported accessor component type.");
        }
    }


    const unsigned char* getAccessorData(
        const tinygltf::Model& model,
        const tinygltf::Accessor& accessor,
        std::size_t elementSize,
        std::size_t& stride) {

        if (accessor.sparse.isSparse) {
            glbError(
                "Sparse accessors are not supported by this loader."
            );
        }

        if (accessor.bufferView < 0 ||
            static_cast<std::size_t>(accessor.bufferView) >=
            model.bufferViews.size()) {

            glbError("Accessor references an invalid buffer view.");
        }

        const tinygltf::BufferView& view =
            model.bufferViews[
                static_cast<std::size_t>(accessor.bufferView)
            ];

        if (view.buffer < 0 ||
            static_cast<std::size_t>(view.buffer) >=
            model.buffers.size()) {

            glbError("Buffer view references an invalid buffer.");
        }

        const tinygltf::Buffer& buffer =
            model.buffers[
                static_cast<std::size_t>(view.buffer)
            ];

        if (view.byteOffset > buffer.data.size() ||
            view.byteLength > buffer.data.size() - view.byteOffset) {

            glbError("Buffer view exceeds its buffer.");
        }

        if (accessor.byteOffset > view.byteLength) {
            glbError("Accessor offset exceeds its buffer view.");
        }

        const int tinyStride = accessor.ByteStride(view);

        if (tinyStride < 0) {
            glbError("Invalid accessor byte stride.");
        }

        stride = tinyStride == 0
            ? elementSize
            : static_cast<std::size_t>(tinyStride);

        if (stride < elementSize) {
            glbError("Accessor stride is smaller than its element.");
        }

        if (accessor.count > 0) {

            const std::size_t available =
                view.byteLength - accessor.byteOffset;

            if (accessor.count - 1 >
                (std::numeric_limits<std::size_t>::max() -
                    elementSize) / stride) {

                glbError("Accessor size overflows addressable memory.");
            }

            const std::size_t required =
                (accessor.count - 1) * stride + elementSize;

            if (required > available) {
                glbError("Accessor data exceeds its buffer view.");
            }
        }

        return buffer.data.data() +
            view.byteOffset +
            accessor.byteOffset;
    }


    FloatAccessorData getFloatAccessor(
        const tinygltf::Model& model,
        int accessorIndex,
        int expectedType,
        int expectedComponents,
        const std::string& attributeName) {

        if (accessorIndex < 0 ||
            static_cast<std::size_t>(accessorIndex) >=
            model.accessors.size()) {

            glbError(
                attributeName + " references an invalid accessor."
            );
        }

        const tinygltf::Accessor& accessor =
            model.accessors[
                static_cast<std::size_t>(accessorIndex)
            ];

        if (accessor.componentType !=
            TINYGLTF_COMPONENT_TYPE_FLOAT) {

            glbError(
                attributeName + " must use 32-bit floating-point values."
            );
        }

        if (accessor.type != expectedType) {
            glbError(
                attributeName + " has an invalid vector type."
            );
        }

        if (accessor.count == 0) {
            glbError(attributeName + " has no elements.");
        }

        const std::size_t elementSize =
            sizeof(float) *
            static_cast<std::size_t>(expectedComponents);

        std::size_t stride = 0;

        const unsigned char* data =
            getAccessorData(
                model,
                accessor,
                elementSize,
                stride
            );

        return {
            data,
            accessor.count,
            stride
        };
    }


    void readFloat3(
        const FloatAccessorData& accessor,
        std::size_t index,
        glm::vec3& result) {

        float values[3]{};

        std::memcpy(
            values,
            accessor.data + index * accessor.stride,
            sizeof(values)
        );

        result = {
            values[0],
            values[1],
            values[2]
        };
    }


    void readFloat2(
        const FloatAccessorData& accessor,
        std::size_t index,
        glm::vec2& result) {

        float values[2]{};

        std::memcpy(
            values,
            accessor.data + index * accessor.stride,
            sizeof(values)
        );

        result = {
            values[0],
            values[1]
        };
    }


    std::uint32_t readGLBIndex(
        const tinygltf::Model& model,
        const tinygltf::Accessor& accessor,
        const unsigned char* data,
        std::size_t stride,
        std::size_t index) {

        const unsigned char* element =
            data + index * stride;

        switch (accessor.componentType) {

        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
            std::uint8_t value{};
            std::memcpy(&value, element, sizeof(value));
            return value;
        }

        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
            std::uint16_t value{};
            std::memcpy(&value, element, sizeof(value));
            return value;
        }

        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
            std::uint32_t value{};
            std::memcpy(&value, element, sizeof(value));
            return value;
        }

        default:
            (void)model;
            glbError(
                "Indices must use unsigned 8-bit, 16-bit, or 32-bit values."
            );
        }
    }


    void loadGLB(
        rap::RaptorModel::Builder& builder,
        const std::string& filepath) {

        tinygltf::TinyGLTF loader;
        tinygltf::Model model;

        std::string error;
        std::string warning;

        const bool loaded =
            loader.LoadBinaryFromFile(
                &model,
                &error,
                &warning,
                filepath
            );

        if (!loaded) {
            glbError(
                "Could not load '" + filepath + "'. " +
                warning + " " + error
            );
        }

        if (model.meshes.empty()) {
            glbError("The file contains no meshes.");
        }

        builder.vertices.clear();
        builder.indices.clear();

        for (const tinygltf::Mesh& mesh : model.meshes) {

            for (const tinygltf::Primitive& primitive :
                mesh.primitives) {

                // Raptor currently renders triangle lists.
                if (primitive.mode != -1 &&
                    primitive.mode != TINYGLTF_MODE_TRIANGLES) {

                    glbError(
                        "Only triangle-list primitives are supported."
                    );
                }

                const auto positionAttribute =
                    primitive.attributes.find("POSITION");

                if (positionAttribute ==
                    primitive.attributes.end()) {

                    glbError(
                        "A mesh primitive has no POSITION attribute."
                    );
                }

                const FloatAccessorData positions =
                    getFloatAccessor(
                        model,
                        positionAttribute->second,
                        TINYGLTF_TYPE_VEC3,
                        3,
                        "POSITION"
                    );

                FloatAccessorData normals{};
                FloatAccessorData uvs{};

                bool hasNormals = false;
                bool hasUVs = false;

                const auto normalAttribute =
                    primitive.attributes.find("NORMAL");

                if (normalAttribute !=
                    primitive.attributes.end()) {

                    normals = getFloatAccessor(
                        model,
                        normalAttribute->second,
                        TINYGLTF_TYPE_VEC3,
                        3,
                        "NORMAL"
                    );

                    if (normals.count != positions.count) {
                        glbError(
                            "NORMAL count does not match POSITION count."
                        );
                    }

                    hasNormals = true;
                }

                const auto uvAttribute =
                    primitive.attributes.find("TEXCOORD_0");

                if (uvAttribute !=
                    primitive.attributes.end()) {

                    uvs = getFloatAccessor(
                        model,
                        uvAttribute->second,
                        TINYGLTF_TYPE_VEC2,
                        2,
                        "TEXCOORD_0"
                    );

                    if (uvs.count != positions.count) {
                        glbError(
                            "TEXCOORD_0 count does not match POSITION count."
                        );
                    }

                    hasUVs = true;
                }

                if (positions.count >
                    std::numeric_limits<std::uint32_t>::max()) {

                    glbError(
                        "A primitive has too many vertices for uint32 indices."
                    );
                }

                if (builder.vertices.size() >
                    std::numeric_limits<std::uint32_t>::max() -
                    positions.count) {

                    glbError(
                        "The model has too many vertices for uint32 indices."
                    );
                }

                const std::uint32_t baseVertex =
                    static_cast<std::uint32_t>(
                        builder.vertices.size()
                        );

                // Read vertex attributes.
                for (std::size_t i = 0; i < positions.count; ++i) {

                    rap::RaptorModel::Vertex vertex{};

                    // Positions are required.
                    readFloat3(
                        positions,
                        i,
                        vertex.position
                    );

                    // Every vertex starts white.
                    vertex.color = {
                        1.0f,
                        1.0f,
                        1.0f
                    };

                    // Missing normals and UVs remain zero.
                    if (hasNormals) {
                        readFloat3(
                            normals,
                            i,
                            vertex.normal
                        );
                    }

                    if (hasUVs) {
                        readFloat2(
                            uvs,
                            i,
                            vertex.uv
                        );
                    }

                    builder.vertices.push_back(vertex);
                }

                // Convert every primitive into the same global
                // uint32 index buffer, including non-indexed primitives.
                if (primitive.indices >= 0) {

                    if (static_cast<std::size_t>(
                        primitive.indices) >= model.accessors.size()) {

                        glbError(
                            "Primitive references an invalid index accessor."
                        );
                    }

                    const tinygltf::Accessor& indexAccessor =
                        model.accessors[
                            static_cast<std::size_t>(
                                primitive.indices
                                )
                        ];

                    if (indexAccessor.type != TINYGLTF_TYPE_SCALAR) {
                        glbError(
                            "The index accessor must be a scalar."
                        );
                    }

                    if (indexAccessor.componentType !=
                        TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE &&
                        indexAccessor.componentType !=
                        TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT &&
                        indexAccessor.componentType !=
                        TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {

                        glbError(
                            "Unsupported index component type."
                        );
                    }

                    if (indexAccessor.count % 3 != 0) {
                        glbError(
                            "Triangle index count must be divisible by three."
                        );
                    }

                    const std::size_t componentSize =
                        getComponentSize(
                            indexAccessor.componentType
                        );

                    std::size_t indexStride = 0;

                    const unsigned char* indexData =
                        getAccessorData(
                            model,
                            indexAccessor,
                            componentSize,
                            indexStride
                        );

                    for (std::size_t i = 0;
                        i < indexAccessor.count;
                        ++i) {

                        const std::uint32_t localIndex =
                            readGLBIndex(
                                model,
                                indexAccessor,
                                indexData,
                                indexStride,
                                i
                            );

                        if (localIndex >= positions.count) {
                            glbError(
                                "An index references a nonexistent vertex."
                            );
                        }

                        builder.indices.push_back(
                            baseVertex + localIndex
                        );
                    }
                }
                else {

                    if (positions.count % 3 != 0) {
                        glbError(
                            "Non-indexed triangle vertex count must be divisible by three."
                        );
                    }

                    for (std::size_t i = 0;
                        i < positions.count;
                        ++i) {

                        builder.indices.push_back(
                            baseVertex +
                            static_cast<std::uint32_t>(i)
                        );
                    }
                }
            }
        }

        if (builder.vertices.empty() ||
            builder.indices.empty()) {

            glbError(
                "The file contains no usable triangle geometry."
            );
        }
    }

} // namespace


namespace std {

    template <>
    struct hash<rap::RaptorModel::Vertex> {

        size_t operator()(
            rap::RaptorModel::Vertex const& vertex) const {

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
            static_cast<std::uint32_t>(
                vertices.size()
                );

        assert(
            vertexCount >= 3 &&
            "Vertex count must be at least 3"
        );

        if (vertices.empty()) {
            throw std::runtime_error(
                "Cannot create a model with no vertices."
            );
        }

        const VkDeviceSize bufferSize =
            sizeof(Vertex) * vertexCount;

        const std::uint32_t vertexSize =
            sizeof(Vertex);

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
            const_cast<Vertex*>(vertices.data())
        );

        // Device-local vertex buffer
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
        const std::vector<std::uint32_t>& indices) {

        indexCount =
            static_cast<std::uint32_t>(
                indices.size()
                );

        hasIndexBuffer =
            indexCount > 0;

        if (!hasIndexBuffer) {
            return;
        }

        const VkDeviceSize bufferSize =
            sizeof(std::uint32_t) * indexCount;

        const std::uint32_t indexSize =
            sizeof(std::uint32_t);

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
            const_cast<std::uint32_t*>(indices.data())
        );

        // Device-local index buffer
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

        vkCmdBindVertexBuffers(
            commandBuffer,
            0,
            1,
            buffers,
            offsets
        );

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
        bindingDescriptions[0].stride = sizeof(Vertex);
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

        vertices.clear();
        indices.clear();

        // Detect file extension.
        const std::size_t dot =
            filepath.find_last_of('.');

        if (dot == std::string::npos) {
            throw std::runtime_error(
                "Model path has no file extension: " + filepath
            );
        }

        std::string extension =
            filepath.substr(dot);

        std::transform(
            extension.begin(),
            extension.end(),
            extension.begin(),
            [](unsigned char character) {
                return static_cast<char>(
                    std::tolower(character)
                    );
            }
        );

        if (extension == ".glb") {
            loadGLB(*this, filepath);
            return;
        }

        if (extension != ".obj") {
            throw std::runtime_error(
                "Unsupported model format '" + extension +
                "'. Supported formats: .obj, .glb"
            );
        }

        // Existing OBJ loading path.
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
                "OBJ loader: " + warn + err
            );
        }

        std::unordered_map<
            Vertex,
            std::uint32_t
        > uniqueVertices{};

        for (const auto& shape : shapes) {

            for (const auto& index :
                shape.mesh.indices) {

                Vertex vertex{};

                // Position
                if (index.vertex_index >= 0) {

                    const std::size_t positionIndex =
                        static_cast<std::size_t>(
                            index.vertex_index
                            ) * 3;

                    if (positionIndex + 2 >=
                        attrib.vertices.size()) {

                        throw std::runtime_error(
                            "OBJ loader: Invalid vertex position index."
                        );
                    }

                    vertex.position = {
                        attrib.vertices[positionIndex + 0],
                        attrib.vertices[positionIndex + 1],
                        attrib.vertices[positionIndex + 2]
                    };

                    // OBJ vertex colors, when present.
                    if (positionIndex + 2 <
                        attrib.colors.size()) {

                        vertex.color = {
                            attrib.colors[positionIndex + 0],
                            attrib.colors[positionIndex + 1],
                            attrib.colors[positionIndex + 2]
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

                    const std::size_t normalIndex =
                        static_cast<std::size_t>(
                            index.normal_index
                            ) * 3;

                    if (normalIndex + 2 >=
                        attrib.normals.size()) {

                        throw std::runtime_error(
                            "OBJ loader: Invalid normal index."
                        );
                    }

                    vertex.normal = {
                        attrib.normals[normalIndex + 0],
                        attrib.normals[normalIndex + 1],
                        attrib.normals[normalIndex + 2]
                    };
                }

                // UV
                if (index.texcoord_index >= 0) {

                    const std::size_t uvIndex =
                        static_cast<std::size_t>(
                            index.texcoord_index
                            ) * 2;

                    if (uvIndex + 1 >=
                        attrib.texcoords.size()) {

                        throw std::runtime_error(
                            "OBJ loader: Invalid texture coordinate index."
                        );
                    }

                    vertex.uv = {
                        attrib.texcoords[uvIndex + 0],
                        attrib.texcoords[uvIndex + 1]
                    };
                }

                // Deduplicate OBJ vertices.
                const auto found =
                    uniqueVertices.find(vertex);

                if (found == uniqueVertices.end()) {

                    if (vertices.size() >=
                        std::numeric_limits<std::uint32_t>::max()) {

                        throw std::runtime_error(
                            "OBJ loader: Too many vertices."
                        );
                    }

                    const std::uint32_t newIndex =
                        static_cast<std::uint32_t>(
                            vertices.size()
                            );

                    uniqueVertices.emplace(
                        vertex,
                        newIndex
                    );

                    vertices.push_back(vertex);

                    indices.push_back(newIndex);
                }
                else {
                    indices.push_back(found->second);
                }
            }
        }

        if (vertices.empty() || indices.empty()) {
            throw std::runtime_error(
                "OBJ loader: The file contains no usable geometry."
            );
        }
    }

} // namespace rap