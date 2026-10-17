#include "rap_descriptors.hpp"

// std
#include <cassert>
#include <stdexcept>
#include <vector>

namespace rap {

    // *************** Descriptor Set Layout Builder *********************

    RaptorDescriptorSetLayout::Builder&
        RaptorDescriptorSetLayout::Builder::addBinding(
            uint32_t binding,
            VkDescriptorType descriptorType,
            VkShaderStageFlags stageFlags,
            uint32_t count) {

        assert(
            bindings.count(binding) == 0 &&
            "Binding already in use"
        );

        VkDescriptorSetLayoutBinding layoutBinding{};

        layoutBinding.binding = binding;
        layoutBinding.descriptorType = descriptorType;
        layoutBinding.descriptorCount = count;
        layoutBinding.stageFlags = stageFlags;

        bindings[binding] = layoutBinding;

        return *this;
    }

    std::unique_ptr<RaptorDescriptorSetLayout>
        RaptorDescriptorSetLayout::Builder::build() const {
        return std::make_unique<RaptorDescriptorSetLayout>(
            raptorDevice,
            bindings
        );
    }

    // *************** Descriptor Set Layout *********************

    RaptorDescriptorSetLayout::RaptorDescriptorSetLayout(
        RaptorDevice& raptorDevice,
        std::unordered_map<
        uint32_t,
        VkDescriptorSetLayoutBinding
        > bindings)
        : raptorDevice{ raptorDevice },
        bindings{ bindings } {

        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};

        for (const auto& kv : bindings) {
            setLayoutBindings.push_back(kv.second);
        }

        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};

        descriptorSetLayoutInfo.sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

        descriptorSetLayoutInfo.bindingCount =
            static_cast<uint32_t>(setLayoutBindings.size());

        descriptorSetLayoutInfo.pBindings =
            setLayoutBindings.data();

        if (vkCreateDescriptorSetLayout(
            raptorDevice.device(),
            &descriptorSetLayoutInfo,
            nullptr,
            &descriptorSetLayout
        ) != VK_SUCCESS) {

            throw std::runtime_error(
                "Failed to create descriptor set layout!"
            );
        }
    }

    RaptorDescriptorSetLayout::~RaptorDescriptorSetLayout() {
        if (descriptorSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(
                raptorDevice.device(),
                descriptorSetLayout,
                nullptr
            );
        }
    }

    // *************** Descriptor Pool Builder *********************

    RaptorDescriptorPool::Builder&
        RaptorDescriptorPool::Builder::addPoolSize(
            VkDescriptorType descriptorType,
            uint32_t count) {

        poolSizes.push_back({
            descriptorType,
            count
            });

        return *this;
    }

    RaptorDescriptorPool::Builder&
        RaptorDescriptorPool::Builder::setPoolFlags(
            VkDescriptorPoolCreateFlags flags) {

        poolFlags = flags;

        return *this;
    }

    RaptorDescriptorPool::Builder&
        RaptorDescriptorPool::Builder::setMaxSets(
            uint32_t count) {

        maxSets = count;

        return *this;
    }

    std::unique_ptr<RaptorDescriptorPool>
        RaptorDescriptorPool::Builder::build() const {

        return std::make_unique<RaptorDescriptorPool>(
            raptorDevice,
            maxSets,
            poolFlags,
            poolSizes
        );
    }

    // *************** Descriptor Pool *********************

    RaptorDescriptorPool::RaptorDescriptorPool(
        RaptorDevice& raptorDevice,
        uint32_t maxSets,
        VkDescriptorPoolCreateFlags poolFlags,
        const std::vector<VkDescriptorPoolSize>& poolSizes)
        : raptorDevice{ raptorDevice } {

        VkDescriptorPoolCreateInfo descriptorPoolInfo{};

        descriptorPoolInfo.sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;

        descriptorPoolInfo.poolSizeCount =
            static_cast<uint32_t>(poolSizes.size());

        descriptorPoolInfo.pPoolSizes = poolSizes.data();

        descriptorPoolInfo.maxSets = maxSets;

        descriptorPoolInfo.flags = poolFlags;

        if (vkCreateDescriptorPool(
            raptorDevice.device(),
            &descriptorPoolInfo,
            nullptr,
            &descriptorPool
        ) != VK_SUCCESS) {

            throw std::runtime_error(
                "Failed to create descriptor pool!"
            );
        }
    }

    RaptorDescriptorPool::~RaptorDescriptorPool() {
        if (descriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(
                raptorDevice.device(),
                descriptorPool,
                nullptr
            );
        }
    }

    bool RaptorDescriptorPool::allocateDescriptor(
        VkDescriptorSetLayout descriptorSetLayout,
        VkDescriptorSet& descriptor
    ) const {

        VkDescriptorSetAllocateInfo allocInfo{};

        allocInfo.sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

        allocInfo.descriptorPool = descriptorPool;

        allocInfo.pSetLayouts = &descriptorSetLayout;

        allocInfo.descriptorSetCount = 1;

        if (vkAllocateDescriptorSets(
            raptorDevice.device(),
            &allocInfo,
            &descriptor
        ) != VK_SUCCESS) {

            return false;
        }

        return true;
    }

    void RaptorDescriptorPool::freeDescriptors(
        std::vector<VkDescriptorSet>& descriptors
    ) const {

        if (descriptors.empty()) {
            return;
        }

        VkResult result = vkFreeDescriptorSets(
            raptorDevice.device(),
            descriptorPool,
            static_cast<uint32_t>(descriptors.size()),
            descriptors.data()
        );

        if (result != VK_SUCCESS) {
            throw std::runtime_error(
                "Failed to free descriptor sets!"
            );
        }
    }

    void RaptorDescriptorPool::resetPool() {
        if (vkResetDescriptorPool(
            raptorDevice.device(),
            descriptorPool,
            0
        ) != VK_SUCCESS) {

            throw std::runtime_error(
                "Failed to reset descriptor pool!"
            );
        }
    }

    // *************** Descriptor Writer *********************

    RaptorDescriptorWriter::RaptorDescriptorWriter(
        RaptorDescriptorSetLayout& setLayout,
        RaptorDescriptorPool& pool)
        : setLayout{ setLayout },
        pool{ pool } {
    }

    RaptorDescriptorWriter&
        RaptorDescriptorWriter::writeBuffer(
            uint32_t binding,
            VkDescriptorBufferInfo* bufferInfo) {

        assert(
            setLayout.bindings.count(binding) == 1 &&
            "Layout does not contain specified binding"
        );

        const auto& bindingDescription =
            setLayout.bindings.at(binding);

        assert(
            bindingDescription.descriptorCount == 1 &&
            "Binding single descriptor info, but binding expects multiple"
        );

        VkWriteDescriptorSet write{};

        write.sType =
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

        write.descriptorType =
            bindingDescription.descriptorType;

        write.dstBinding = binding;

        write.pBufferInfo = bufferInfo;

        write.descriptorCount = 1;

        writes.push_back(write);

        return *this;
    }

    RaptorDescriptorWriter&
        RaptorDescriptorWriter::writeImage(
            uint32_t binding,
            VkDescriptorImageInfo* imageInfo) {

        assert(
            setLayout.bindings.count(binding) == 1 &&
            "Layout does not contain specified binding"
        );

        const auto& bindingDescription =
            setLayout.bindings.at(binding);

        assert(
            bindingDescription.descriptorCount == 1 &&
            "Binding single descriptor info, but binding expects multiple"
        );

        VkWriteDescriptorSet write{};

        write.sType =
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

        write.descriptorType =
            bindingDescription.descriptorType;

        write.dstBinding = binding;

        write.pImageInfo = imageInfo;

        write.descriptorCount = 1;

        writes.push_back(write);

        return *this;
    }

    bool RaptorDescriptorWriter::build(
        VkDescriptorSet& set) {

        const bool success = pool.allocateDescriptor(
            setLayout.getDescriptorSetLayout(),
            set
        );

        if (!success) {
            return false;
        }

        overwrite(set);

        return true;
    }

    void RaptorDescriptorWriter::overwrite(
        VkDescriptorSet& set) {

        for (auto& write : writes) {
            write.dstSet = set;
        }

        vkUpdateDescriptorSets(
            pool.raptorDevice.device(),
            static_cast<uint32_t>(writes.size()),
            writes.data(),
            0,
            nullptr
        );
    }

} // namespace rap