#include "rap_cubemap.hpp"

//std
#include <stdexcept>
#include <cstring>
#include <limits>

namespace rap {
    RaptorCubeMap::RaptorCubeMap(
        RaptorDevice& device,
        uint32_t faceSize
    ) : device{ device }, faceSize{ faceSize } {

        if (faceSize == 0) {
            throw std::invalid_argument(
                "Cubemap face size cannot be zero"
            );
        }

        try {
            createImage();
            createImageView();
            createStorageImageView();
            createSampler();
        }
        catch (...) {
            if (sampler != VK_NULL_HANDLE) {
                vkDestroySampler(
                    device.device(), sampler, nullptr
                );
            }

            if (imageView != VK_NULL_HANDLE) {
                vkDestroyImageView(
                    device.device(), imageView, nullptr
                );
            }

			if (storageImageView != VK_NULL_HANDLE) {
				vkDestroyImageView(
					device.device(), storageImageView, nullptr
				);
			}

            if (image != VK_NULL_HANDLE) {
                vkDestroyImage(
                    device.device(), image, nullptr
                );
            }

            if (imageMemory != VK_NULL_HANDLE) {
                vkFreeMemory(
                    device.device(), imageMemory, nullptr
                );
            }

            throw;
        }
    }

    RaptorCubeMap::~RaptorCubeMap() {

		if (sampler != VK_NULL_HANDLE) {
			vkDestroySampler(
				device.device(), sampler, nullptr
			);
		}

        if (imageView != VK_NULL_HANDLE) {
			vkDestroyImageView(
				device.device(), imageView, nullptr
			);
        }

        if (storageImageView != VK_NULL_HANDLE) {
            vkDestroyImageView(
				device.device(), storageImageView, nullptr
            );
        }

        if (image != VK_NULL_HANDLE) {
            vkDestroyImage(
                device.device(), image, nullptr
            );
        }

        if (imageMemory != VK_NULL_HANDLE) {
            vkFreeMemory(
                device.device(), imageMemory, nullptr
            );
        }
    }

    void RaptorCubeMap::uploadFaces(
        const std::array<std::vector<unsigned char>, 6>& faces
    ) {
        const VkDeviceSize faceBytes =
            static_cast<VkDeviceSize>(faceSize) * faceSize * 4;

        if (faceBytes > std::numeric_limits<VkDeviceSize>::max() / 6 ||
            faceBytes * 6 > std::numeric_limits<size_t>::max()) {
            throw std::runtime_error("CubeMap is too large");
        }

        const VkDeviceSize totalBytes = faceBytes * 6;

        std::vector<unsigned char> combinedPixels;
        combinedPixels.reserve(static_cast<size_t>(totalBytes));

        for (const auto& face : faces) {
            if (face.size() != static_cast<size_t>(faceBytes)) {
                throw std::runtime_error("Cubemap has an invalid size");
            }

            combinedPixels.insert(
                combinedPixels.end(),
                face.begin(),
                face.end()
            );
        }

        RapBuffer stagingBuffer{
            device,
            totalBytes,
            2,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        };

        if (stagingBuffer.map() != VK_SUCCESS) {
            throw std::runtime_error(
                "Failed to map cubemap staging buffer"
            );
        }

        stagingBuffer.writeToBuffer(
            combinedPixels.data(),
            totalBytes
        );

        VkCommandBuffer commandBuffer =
            device.beginSingleTimeCommands();

        // Transition cubemap to transfer destination layout

        VkImageMemoryBarrier toTransfer{};

        toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        toTransfer.srcAccessMask = 0;
        toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        toTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toTransfer.image = image;

        toTransfer.subresourceRange.aspectMask =
            VK_IMAGE_ASPECT_COLOR_BIT;

        toTransfer.subresourceRange.baseMipLevel = 0;
        toTransfer.subresourceRange.levelCount = 1;
        toTransfer.subresourceRange.baseArrayLayer = 0;
        toTransfer.subresourceRange.layerCount = 6;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &toTransfer
        );

        // Copy all six cubemap faces

        VkBufferImageCopy copyRegion{};

        copyRegion.bufferOffset = 0;
        copyRegion.bufferRowLength = 0;
        copyRegion.bufferImageHeight = 0;

        copyRegion.imageSubresource.aspectMask =
            VK_IMAGE_ASPECT_COLOR_BIT;

        copyRegion.imageSubresource.mipLevel = 0;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount = 6;

        copyRegion.imageOffset = { 0, 0, 0 };
        copyRegion.imageExtent = { faceSize, faceSize, 1 };

        vkCmdCopyBufferToImage(
            commandBuffer,
            stagingBuffer.getBuffer(),
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &copyRegion
        );

        // Transition cubemap to shader read-only layout

        VkImageMemoryBarrier toShaderRead{};

        toShaderRead.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        toShaderRead.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        toShaderRead.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        toShaderRead.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        toShaderRead.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        toShaderRead.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toShaderRead.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toShaderRead.image = image;

        toShaderRead.subresourceRange.aspectMask =
            VK_IMAGE_ASPECT_COLOR_BIT;

        toShaderRead.subresourceRange.baseMipLevel = 0;
        toShaderRead.subresourceRange.levelCount = 1;
        toShaderRead.subresourceRange.baseArrayLayer = 0;
        toShaderRead.subresourceRange.layerCount = 6;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &toShaderRead
        );

        // Submit commands

        device.endSingleTimeCommands(commandBuffer);
    }

    void RaptorCubeMap::createImage() {

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;

        imageInfo.extent = {
            faceSize,
            faceSize,
            1
        };

        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 6;

        imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage =
            VK_IMAGE_USAGE_STORAGE_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT |
            VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        device.createImageWithInfo(
            imageInfo,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            image,
            imageMemory
        );
    }

    void RaptorCubeMap::createImageView() {

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;

        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 6;

        if (vkCreateImageView(
            device.device(),
            &viewInfo,
            nullptr,
            &imageView
        ) != VK_SUCCESS) {
            throw std::runtime_error(
                "Failed to create cubemap Image view"
            );
        }
    }

    void RaptorCubeMap::createSampler() {

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;

        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;

        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;

        if (vkCreateSampler(
            device.device(),
            &samplerInfo,
            nullptr,
            &sampler
        ) != VK_SUCCESS) {
            throw std::runtime_error(
                "Failed to create cubemap Sampler"
            );
        }

    }

    void RaptorCubeMap::createStorageImageView() {

        VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
		viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;

		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 6;

        if (vkCreateImageView(
            device.device(),
            &viewInfo,
            nullptr,
            &storageImageView
        ) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create cubemap storage Image view");
        }
    }

}// namespace rap