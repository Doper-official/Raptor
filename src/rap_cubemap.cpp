#include "rap_cubemap.hpp"

//std
#include <stdexcept>

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
            VK_IMAGE_USAGE_SAMPLED_BIT;

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