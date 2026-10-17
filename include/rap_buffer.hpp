#pragma once

#include "rap_device.hpp"

namespace rap {

    class RapBuffer {
    public:

        RapBuffer(
            RaptorDevice& device,
            VkDeviceSize instanceSize,
            uint32_t instanceCount,
            VkBufferUsageFlags usageFlags,
            VkMemoryPropertyFlags memoryPropertyFlags,
            VkDeviceSize minOffsetAlignment = 1);

        ~RapBuffer();


        RapBuffer(const RapBuffer&) = delete;
        RapBuffer& operator=(const RapBuffer&) = delete;


        VkResult map(
            VkDeviceSize size = VK_WHOLE_SIZE,
            VkDeviceSize offset = 0);

        void unmap();


        void writeToBuffer(
            void* data,
            VkDeviceSize size = VK_WHOLE_SIZE,
            VkDeviceSize offset = 0);


        VkResult flush(
            VkDeviceSize size = VK_WHOLE_SIZE,
            VkDeviceSize offset = 0);


        VkDescriptorBufferInfo descriptorInfo(
            VkDeviceSize size = VK_WHOLE_SIZE,
            VkDeviceSize offset = 0);


        VkResult invalidate(
            VkDeviceSize size = VK_WHOLE_SIZE,
            VkDeviceSize offset = 0);


        void writeToIndex(
            void* data,
            int index);

        VkResult flushIndex(
            int index);

        VkDescriptorBufferInfo descriptorInfoForIndex(
            int index);

        VkResult invalidateIndex(
            int index);


        VkBuffer getBuffer() const {
            return buffer;
        }

        void* getMappedMemory() const {
            return mapped;
        }

        uint32_t getInstanceCount() const {
            return instanceCount;
        }

        VkDeviceSize getInstanceSize() const {
            return instanceSize;
        }

        VkDeviceSize getAlignmentSize() const {
            return alignmentSize;
        }

        VkBufferUsageFlags getUsageFlags() const {
            return usageFlags;
        }

        VkMemoryPropertyFlags getMemoryPropertyFlags() const {
            return memoryPropertyFlags;
        }

        VkDeviceSize getBufferSize() const {
            return bufferSize;
        }


    private:

        static VkDeviceSize getAlignment(
            VkDeviceSize instanceSize,
            VkDeviceSize minOffsetAlignment);


        RaptorDevice& raptorDevice;

        void* mapped = nullptr;

        VkBuffer buffer = VK_NULL_HANDLE;

        VkDeviceMemory memory = VK_NULL_HANDLE;


        VkDeviceSize bufferSize = 0;

        uint32_t instanceCount = 0;

        VkDeviceSize instanceSize = 0;

        VkDeviceSize alignmentSize = 0;


        VkBufferUsageFlags usageFlags = 0;

        VkMemoryPropertyFlags memoryPropertyFlags = 0;
    };

} // namespace rap