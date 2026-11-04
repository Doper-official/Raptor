#include "rap_model.hpp"
#include "rap_utils.hpp"

// Libraries
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <tiny_gltf.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

// Standard library
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <functional>
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

        builder.textures.clear();

        for (const tinygltf::Image& image : model.images) {
            if (image.width <= 0 || image.height <= 0) {
                glbError("GLB contains an image with invalid Dimensions");
            }

            if (image.bits != 8 ||
                (image.component != 3 && image.component != 4)) {
                glbError("Raptor supports 8-bit RGB and RGBA textures only");
            }

            const std::size_t pixelCount =
                static_cast<std::size_t>(image.width) * image.height;

            if (image.image.size() != pixelCount * image.component) {
                glbError("GLB image data has an invalid size");
            }

            rap::RaptorModel::TextureData texture{};
            
            texture.width = static_cast<uint32_t>(image.width);
            texture.height = static_cast<uint32_t>(image.height);

            texture.pixels.resize(pixelCount * 4);

            for (std::size_t i = 0; i < pixelCount; ++i) {
                const std::size_t src = i * image.component;
                const std::size_t dst = i * 4;

                texture.pixels[dst + 0] = image.image[src + 0];
                texture.pixels[dst + 1] = image.image[src + 1];
                texture.pixels[dst + 2] = image.image[src + 2];

                //alpha color image preservation thing
                texture.pixels[dst + 3] =
                    image.component == 4 ? image.image[src + 3] : 255;
            }

            builder.textures.push_back(std::move(texture));
        }
        builder.materials.clear();

        for (const tinygltf::Material& gltfMaterial : model.materials) {
            rap::RaptorModel::Material material{};

            const int textureIndex =
                gltfMaterial.pbrMetallicRoughness
                .baseColorTexture.index;

            if (textureIndex >= 0) {
                if (static_cast<std::size_t>(textureIndex) >=
                    model.textures.size()) {
                    glbError("Material refrences an invalid texture");
                }

                const int imageIndex =
                    model.textures[
                        static_cast<std::size_t>(textureIndex)
                    ].source;

                if (imageIndex < 0 ||
                    static_cast<std::size_t>(imageIndex) >=
                    builder.textures.size()) {
                    glbError("Texture refrences an invalid image");
                }

                material.baseColorTextureIndex = imageIndex;
            }
            builder.materials.push_back(material);
        }

        builder.vertices.clear();
        builder.indices.clear();
        builder.primitive.clear();

        // glTF meshes are not scene objects by themselves. Nodes decide
        // where each mesh actually lives, so walk the active scene instead
        // of blindly drawing every mesh at the origin.
        auto makeNodeTransform = [](const tinygltf::Node& node) {
            if (node.matrix.size() == 16) {
                glm::mat4 matrix{ 1.0f };

                // glTF matrices are column-major, just like GLM.
                for (int column = 0; column < 4; ++column) {
                    for (int row = 0; row < 4; ++row) {
                        matrix[column][row] = static_cast<float>(
                            node.matrix[column * 4 + row]
                        );
                    }
                }

                return matrix;
            }

            glm::vec3 translation{ 0.0f };
            glm::vec3 scale{ 1.0f };
            glm::quat rotation{ 1.0f, 0.0f, 0.0f, 0.0f };

            if (node.translation.size() == 3) {
                translation = {
                    static_cast<float>(node.translation[0]),
                    static_cast<float>(node.translation[1]),
                    static_cast<float>(node.translation[2])
                };
            }

            if (node.scale.size() == 3) {
                scale = {
                    static_cast<float>(node.scale[0]),
                    static_cast<float>(node.scale[1]),
                    static_cast<float>(node.scale[2])
                };
            }

            if (node.rotation.size() == 4) {
                // glTF stores quaternions as x, y, z, w.
                rotation = glm::quat{
                    static_cast<float>(node.rotation[3]),
                    static_cast<float>(node.rotation[0]),
                    static_cast<float>(node.rotation[1]),
                    static_cast<float>(node.rotation[2])
                };

                const float rotationLength = glm::length(rotation);
                if (rotationLength > 0.000001f) {
                    rotation = glm::normalize(rotation);
                }
                else {
                    glbError("A node contains a zero-length rotation quaternion.");
                }
            }

            return glm::translate(glm::mat4{ 1.0f }, translation) *
                glm::mat4_cast(rotation) *
                glm::scale(glm::mat4{ 1.0f }, scale);
        };

        auto appendMesh = [&](int meshIndex, const glm::mat4& worldMatrix) {
            if (meshIndex < 0 ||
                static_cast<std::size_t>(meshIndex) >= model.meshes.size()) {
                glbError("A scene node references an invalid mesh.");
            }

            const glm::mat3 worldLinear{ worldMatrix };
            const float determinant = glm::determinant(worldLinear);

            if (std::abs(determinant) < 0.0000001f) {
                glbError("A mesh node has a singular transform (zero scale).");
            }

            // Normals are directions perpendicular to the surface. They
            // need inverse-transpose, not the position transform.
            const glm::mat3 normalMatrix =
                glm::transpose(glm::inverse(worldLinear));

            const bool mirrored = determinant < 0.0f;

            const tinygltf::Mesh& mesh =
                model.meshes[static_cast<std::size_t>(meshIndex)];

            for (const tinygltf::Primitive& primitive : mesh.primitives) {
                if (builder.indices.size() >
                    std::numeric_limits<std::uint32_t>::max()) {
                    glbError("The model has too many indices for uint32 draws.");
                }

                const uint32_t firstIndex =
                    static_cast<uint32_t>(builder.indices.size());

                if (primitive.mode != -1 &&
                    primitive.mode != TINYGLTF_MODE_TRIANGLES) {
                    glbError("Only triangle-list primitives are supported.");
                }

                const auto positionAttribute =
                    primitive.attributes.find("POSITION");

                if (positionAttribute == primitive.attributes.end()) {
                    glbError("A mesh primitive has no POSITION attribute.");
                }

                const FloatAccessorData positions = getFloatAccessor(
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

                if (normalAttribute != primitive.attributes.end()) {
                    normals = getFloatAccessor(
                        model,
                        normalAttribute->second,
                        TINYGLTF_TYPE_VEC3,
                        3,
                        "NORMAL"
                    );

                    if (normals.count != positions.count) {
                        glbError("NORMAL count does not match POSITION count.");
                    }
                    hasNormals = true;
                }

                const auto uvAttribute =
                    primitive.attributes.find("TEXCOORD_0");

                if (uvAttribute != primitive.attributes.end()) {
                    uvs = getFloatAccessor(
                        model,
                        uvAttribute->second,
                        TINYGLTF_TYPE_VEC2,
                        2,
                        "TEXCOORD_0"
                    );

                    if (uvs.count != positions.count) {
                        glbError("TEXCOORD_0 count does not match POSITION count.");
                    }
                    hasUVs = true;
                }

                if (positions.count > std::numeric_limits<std::uint32_t>::max() ||
                    builder.vertices.size() >
                        std::numeric_limits<std::uint32_t>::max() - positions.count) {
                    glbError("The model has too many vertices for uint32 indices.");
                }

                // Gather local indices first. We need them to generate a
                // fallback normal if a model forgot to export NORMAL.
                std::vector<std::uint32_t> localIndices;

                if (primitive.indices >= 0) {
                    if (static_cast<std::size_t>(primitive.indices) >=
                        model.accessors.size()) {
                        glbError("Primitive references an invalid index accessor.");
                    }

                    const tinygltf::Accessor& indexAccessor =
                        model.accessors[static_cast<std::size_t>(primitive.indices)];

                    if (indexAccessor.type != TINYGLTF_TYPE_SCALAR) {
                        glbError("The index accessor must be a scalar.");
                    }

                    if (indexAccessor.componentType != TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE &&
                        indexAccessor.componentType != TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT &&
                        indexAccessor.componentType != TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
                        glbError("Indices must use unsigned 8-bit, 16-bit, or 32-bit values.");
                    }

                    if (indexAccessor.count % 3 != 0) {
                        glbError("Triangle index count must be divisible by three.");
                    }

                    const std::size_t componentSize =
                        getComponentSize(indexAccessor.componentType);
                    std::size_t indexStride = 0;
                    const unsigned char* indexData = getAccessorData(
                        model,
                        indexAccessor,
                        componentSize,
                        indexStride
                    );

                    localIndices.reserve(indexAccessor.count);
                    for (std::size_t i = 0; i < indexAccessor.count; ++i) {
                        const std::uint32_t localIndex = readGLBIndex(
                            model,
                            indexAccessor,
                            indexData,
                            indexStride,
                            i
                        );

                        if (localIndex >= positions.count) {
                            glbError("An index references a nonexistent vertex.");
                        }
                        localIndices.push_back(localIndex);
                    }
                }
                else {
                    if (positions.count % 3 != 0) {
                        glbError("Non-indexed triangle vertex count must be divisible by three.");
                    }

                    localIndices.reserve(positions.count);
                    for (std::size_t i = 0; i < positions.count; ++i) {
                        localIndices.push_back(static_cast<std::uint32_t>(i));
                    }
                }

                // A reflected node transform reverses triangle winding.
                // Flip each triangle so its winding still agrees with its
                // transformed normal.
                if (mirrored) {
                    for (std::size_t i = 0; i < localIndices.size(); i += 3) {
                        std::swap(localIndices[i + 1], localIndices[i + 2]);
                    }
                }

                const std::uint32_t baseVertex =
                    static_cast<std::uint32_t>(builder.vertices.size());

                std::vector<glm::vec3> generatedNormalSums(positions.count, glm::vec3{ 0.0f });
                std::vector<glm::vec3> transformedPositions(positions.count);
                std::vector<glm::vec3> transformedNormals(positions.count, glm::vec3{ 0.0f });
                std::vector<glm::vec2> texcoords(positions.count, glm::vec2{ 0.0f });

                for (std::size_t i = 0; i < positions.count; ++i) {
                    glm::vec3 position{};
                    readFloat3(positions, i, position);
                    transformedPositions[i] =
                        glm::vec3(worldMatrix * glm::vec4(position, 1.0f));

                    if (hasNormals) {
                        glm::vec3 normal{};
                        readFloat3(normals, i, normal);

                        if (glm::dot(normal, normal) > 0.0000001f) {
                            transformedNormals[i] = glm::normalize(normalMatrix * normal);
                        }
                    }

                    if (hasUVs) {
                        readFloat2(uvs, i, texcoords[i]);
                    }
                }

                // Compute smooth fallback normals only where needed. This
                // also repairs zero/invalid entries without flattening the
                // good normals already supplied by the asset.
                for (std::size_t i = 0; i < localIndices.size(); i += 3) {
                    const std::uint32_t a = localIndices[i + 0];
                    const std::uint32_t b = localIndices[i + 1];
                    const std::uint32_t c = localIndices[i + 2];

                    const glm::vec3 edgeA = transformedPositions[b] - transformedPositions[a];
                    const glm::vec3 edgeB = transformedPositions[c] - transformedPositions[a];
                    const glm::vec3 faceNormal = glm::cross(edgeA, edgeB);

                    if (glm::dot(faceNormal, faceNormal) > 0.0000001f) {
                        generatedNormalSums[a] += faceNormal;
                        generatedNormalSums[b] += faceNormal;
                        generatedNormalSums[c] += faceNormal;
                    }
                }

                for (std::size_t i = 0; i < positions.count; ++i) {
                    rap::RaptorModel::Vertex vertex{};
                    vertex.position = transformedPositions[i];
                    vertex.color = { 1.0f, 1.0f, 1.0f };
                    vertex.uv = texcoords[i];

                    const glm::vec3 generated = generatedNormalSums[i];
                    if (glm::dot(transformedNormals[i], transformedNormals[i]) > 0.0000001f) {
                        vertex.normal = glm::normalize(transformedNormals[i]);
                    }
                    else if (glm::dot(generated, generated) > 0.0000001f) {
                        vertex.normal = glm::normalize(generated);
                    }
                    else {
                        vertex.normal = { 0.0f, 1.0f, 0.0f };
                    }

                    builder.vertices.push_back(vertex);
                }

                if (localIndices.size() >
                    std::numeric_limits<std::uint32_t>::max() - builder.indices.size()) {
                    glbError("The model has too many indices for uint32 draws.");
                }

                for (const std::uint32_t localIndex : localIndices) {
                    builder.indices.push_back(baseVertex + localIndex);
                }

                if (primitive.material < -1 ||
                    (primitive.material >= 0 &&
                     static_cast<std::size_t>(primitive.material) >= model.materials.size())) {
                    glbError("A primitive references an invalid material.");
                }

                rap::RaptorModel::Primitive modelPrimitive{};
                modelPrimitive.firstIndex = firstIndex;
                modelPrimitive.indexCount =
                    static_cast<std::uint32_t>(builder.indices.size()) - firstIndex;
                modelPrimitive.materialIndex = primitive.material;
                builder.primitive.push_back(modelPrimitive);
            }
        };

        std::vector<bool> activeNodes(model.nodes.size(), false);

        std::function<void(int, const glm::mat4&)> visitNode;
        visitNode = [&](int nodeIndex, const glm::mat4& parentMatrix) {
            if (nodeIndex < 0 ||
                static_cast<std::size_t>(nodeIndex) >= model.nodes.size()) {
                glbError("A scene references an invalid node.");
            }

            const std::size_t index = static_cast<std::size_t>(nodeIndex);
            if (activeNodes[index]) {
                glbError("The glTF scene graph contains a node cycle.");
            }

            activeNodes[index] = true;
            const tinygltf::Node& node = model.nodes[index];
            const glm::mat4 worldMatrix = parentMatrix * makeNodeTransform(node);

            if (node.mesh >= 0) {
                appendMesh(node.mesh, worldMatrix);
            }

            for (const int child : node.children) {
                visitNode(child, worldMatrix);
            }

            activeNodes[index] = false;
        };

        bool visitedScene = false;

        if (!model.scenes.empty()) {
            int sceneIndex = model.defaultScene;
            if (sceneIndex < 0 ||
                static_cast<std::size_t>(sceneIndex) >= model.scenes.size()) {
                sceneIndex = 0;
            }

            const tinygltf::Scene& scene =
                model.scenes[static_cast<std::size_t>(sceneIndex)];

            for (const int rootNode : scene.nodes) {
                visitNode(rootNode, glm::mat4{ 1.0f });
                visitedScene = true;
            }
        }
        else if (!model.nodes.empty()) {
            // Some small GLBs omit the scenes array. Find root nodes from
            // the node graph and traverse them anyway.
            std::vector<bool> hasParent(model.nodes.size(), false);
            for (const tinygltf::Node& node : model.nodes) {
                for (const int child : node.children) {
                    if (child >= 0 && static_cast<std::size_t>(child) < hasParent.size()) {
                        hasParent[static_cast<std::size_t>(child)] = true;
                    }
                }
            }

            for (std::size_t i = 0; i < model.nodes.size(); ++i) {
                if (!hasParent[i]) {
                    visitNode(static_cast<int>(i), glm::mat4{ 1.0f });
                    visitedScene = true;
                }
            }
        }

        // Backwards-compatible fallback for unusual mesh-only GLBs.
        if (!visitedScene) {
            for (std::size_t i = 0; i < model.meshes.size(); ++i) {
                appendMesh(static_cast<int>(i), glm::mat4{ 1.0f });
            }
        }

        if (builder.vertices.empty() || builder.indices.empty()) {
            glbError("The active scene contains no usable triangle geometry.");
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
        : device{ device },
        primitives{ builder.primitive },
        materials{ builder.materials } {

        createVertexBuffer(builder.vertices);
        createIndexBuffer(builder.indices);

        textures.reserve(builder.textures.size());

        for (const TextureData& textureData : builder.textures) {
            textures.push_back(
                std::make_unique<RaptorImage>(
                    device,
                    VkExtent2D{ textureData.width, textureData.height },
                    VK_FORMAT_R8G8B8A8_SRGB,
                    VK_IMAGE_ASPECT_COLOR_BIT,
                    textureData.pixels
                )
            );
        }
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

	VkDescriptorImageInfo RaptorModel::getTextureDescriptorInfo(
		size_t index) const {
		if (index >= textures.size()) {
			throw std::runtime_error(
				" glbTexture index out of range."
			);
		}
		
		VkDescriptorImageInfo imageInfo{};
		imageInfo.sampler = textures[index]->getSampler();
		imageInfo.imageView = textures[index]->getImageView();
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		return imageInfo;
	}

    void RaptorModel::drawPrimitive(
        VkCommandBuffer commandBuffer,
        uint32_t firstIndex,
        uint32_t indexCount
    ) {

		if (!hasIndexBuffer || indexCount == 0) {
            return;
		}

        vkCmdDrawIndexed(
            commandBuffer,
            indexCount,
            1,
            firstIndex,
            0,
            0
        );
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