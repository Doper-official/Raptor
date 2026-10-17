/*
 * Encapsulates a Vulkan buffer
 */

#include "rap_buffer.hpp"

 // std
#include <cassert>
#include <cstring>

namespace rap {

    VkDeviceSize RapBuffer::getAlignment(
        VkDeviceSize instanceSize,
        VkDeviceSize minOffsetAlignment) {

        if (minOffsetAlignment > 0) {
            return (instanceSize + minOffsetAlignment - 1) &
                ~(minOffsetAlignment - 1);
        }

        return instanceSize;
    }


    RapBuffer::RapBuffer(
        RaptorDevice& device,
        VkDeviceSize instanceSize,
        uint32_t instanceCount,
        VkBufferUsageFlags usageFlags,
        VkMemoryPropertyFlags memoryPropertyFlags,
        VkDeviceSize minOffsetAlignment)

        : raptorDevice{ device },
        instanceSize{ instanceSize },
        instanceCount{ instanceCount },
        usageFlags{ usageFlags },
        memoryPropertyFlags{ memoryPropertyFlags } {

        alignmentSize =
            getAlignment(
                instanceSize,
                minOffsetAlignment
            );

        bufferSize =
            alignmentSize * instanceCount;

        device.createBuffer(
            bufferSize,
            usageFlags,
            memoryPropertyFlags,
            buffer,
            memory
        );
    }


    RapBuffer::~RapBuffer() {

        unmap();

        vkDestroyBuffer(
            raptorDevice.device(),
            buffer,
            nullptr
        );

        vkFreeMemory(
            raptorDevice.device(),
            memory,
            nullptr
        );
    }


    VkResult RapBuffer::map(
        VkDeviceSize size,
        VkDeviceSize offset) {

        assert(
            buffer &&
            memory &&
            "Called map on buffer before create"
        );

		return vkMapMemory(raptorDevice.device(), memory, offset, size, 0, &mapped);
    }


    void RapBuffer::unmap() {

        if (mapped) {

            vkUnmapMemory(
                raptorDevice.device(),
                memory
            );

            mapped = nullptr;
        }
    }


    void RapBuffer::writeToBuffer(
        void* data,
        VkDeviceSize size,
        VkDeviceSize offset) {

        assert(
            mapped &&
            "Cannot copy to unmapped buffer"
        );

        if (size == VK_WHOLE_SIZE) {

            memcpy(
                mapped,
                data,
                bufferSize
            );
        }
        else {

            char* memOffset =
                static_cast<char*>(mapped);

            memOffset += offset;

            memcpy(
                memOffset,
                data,
                size
            );
        }
    }


    VkResult RapBuffer::flush(
        VkDeviceSize size,
        VkDeviceSize offset) {

        VkMappedMemoryRange mappedRange{};

        mappedRange.sType =
            VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;

        mappedRange.memory =
            memory;

        mappedRange.offset =
            offset;

        mappedRange.size =
            size;

        return vkFlushMappedMemoryRanges(
            raptorDevice.device(),
            1,
            &mappedRange
        );
    }


    VkResult RapBuffer::invalidate(
        VkDeviceSize size,
        VkDeviceSize offset) {

        VkMappedMemoryRange mappedRange{};

        mappedRange.sType =
            VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;

        mappedRange.memory =
            memory;

        mappedRange.offset =
            offset;

        mappedRange.size =
            size;

        return vkInvalidateMappedMemoryRanges(
            raptorDevice.device(),
            1,
            &mappedRange
        );
    }


    VkDescriptorBufferInfo RapBuffer::descriptorInfo(
        VkDeviceSize size,
        VkDeviceSize offset) {

        return VkDescriptorBufferInfo{
            buffer,
            offset,
            size
        };
    }


    void RapBuffer::writeToIndex(
        void* data,
        int index) {

        writeToBuffer(
            data,
            instanceSize,
            index * alignmentSize
        );
    }


    VkResult RapBuffer::flushIndex(
        int index) {

        return flush(
            alignmentSize,
            index * alignmentSize
        );
    }


    VkDescriptorBufferInfo RapBuffer::descriptorInfoForIndex(
        int index) {

        return descriptorInfo(
            alignmentSize,
            index * alignmentSize
        );
    }


    VkResult RapBuffer::invalidateIndex(
        int index) {

        return invalidate(
            alignmentSize,
            index * alignmentSize
        );
    }

} // namespace rap