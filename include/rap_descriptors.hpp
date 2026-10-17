
#pragma once

#include "rap_device.hpp"

// Vulkan
#include <vulkan/vulkan.h>

// std
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

namespace rap {

    class RaptorDescriptorWriter;

    class RaptorDescriptorSetLayout {
    public:
        class Builder {
        public:
            explicit Builder(RaptorDevice& raptorDevice)
                : raptorDevice{ raptorDevice } {
            }

            Builder& addBinding(
                uint32_t binding,
                VkDescriptorType descriptorType,
                VkShaderStageFlags stageFlags,
                uint32_t count = 1);

            std::unique_ptr<RaptorDescriptorSetLayout> build() const;

        private:
            RaptorDevice& raptorDevice;
            std::unordered_map<
                uint32_t,
                VkDescriptorSetLayoutBinding
            > bindings{};
        };

        RaptorDescriptorSetLayout(
            RaptorDevice& raptorDevice,
            std::unordered_map<
            uint32_t,
            VkDescriptorSetLayoutBinding
            > bindings);

        ~RaptorDescriptorSetLayout();

        RaptorDescriptorSetLayout(
            const RaptorDescriptorSetLayout&) = delete;

        RaptorDescriptorSetLayout& operator=(
            const RaptorDescriptorSetLayout&) = delete;

        VkDescriptorSetLayout getDescriptorSetLayout() const {
            return descriptorSetLayout;
        }

    private:
        RaptorDevice& raptorDevice;
        VkDescriptorSetLayout descriptorSetLayout{};
        std::unordered_map<
            uint32_t,
            VkDescriptorSetLayoutBinding
        > bindings{};

        friend class RaptorDescriptorWriter;
    };


    class RaptorDescriptorPool {
    public:
        class Builder {
        public:
            explicit Builder(RaptorDevice& raptorDevice)
                : raptorDevice{ raptorDevice } {
            }

            Builder& addPoolSize(
                VkDescriptorType descriptorType,
                uint32_t count);

            Builder& setPoolFlags(
                VkDescriptorPoolCreateFlags flags);

            Builder& setMaxSets(uint32_t count);

            std::unique_ptr<RaptorDescriptorPool> build() const;

        private:
            RaptorDevice& raptorDevice;
            std::vector<VkDescriptorPoolSize> poolSizes{};
            uint32_t maxSets = 1000;
            VkDescriptorPoolCreateFlags poolFlags = 0;
        };

        RaptorDescriptorPool(
            RaptorDevice& raptorDevice,
            uint32_t maxSets,
            VkDescriptorPoolCreateFlags poolFlags,
            const std::vector<VkDescriptorPoolSize>& poolSizes);

        ~RaptorDescriptorPool();

        RaptorDescriptorPool(const RaptorDescriptorPool&) = delete;

        RaptorDescriptorPool& operator=(
            const RaptorDescriptorPool&) = delete;

        bool allocateDescriptor(
            VkDescriptorSetLayout descriptorSetLayout,
            VkDescriptorSet& descriptor) const;

        void freeDescriptors(
            std::vector<VkDescriptorSet>& descriptors) const;

        void resetPool();

    private:
        RaptorDevice& raptorDevice;
        VkDescriptorPool descriptorPool{};

        friend class RaptorDescriptorWriter;
    };


    class RaptorDescriptorWriter {
    public:
        RaptorDescriptorWriter(
            RaptorDescriptorSetLayout& setLayout,
            RaptorDescriptorPool& pool);

        RaptorDescriptorWriter& writeBuffer(
            uint32_t binding,
            VkDescriptorBufferInfo* bufferInfo);

        RaptorDescriptorWriter& writeImage(
            uint32_t binding,
            VkDescriptorImageInfo* imageInfo);

        bool build(VkDescriptorSet& set);

        void overwrite(VkDescriptorSet& set);

    private:
        RaptorDescriptorSetLayout& setLayout;
        RaptorDescriptorPool& pool;
        std::vector<VkWriteDescriptorSet> writes{};
    };

} // namespace rap