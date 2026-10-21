#include "rap_image.hpp"

//std
#include <stdexcept>
#include <cassert>

namespace rap {

	RaptorImage::RaptorImage(
		RaptorDevice& device,
		VkExtent2D extent,
		VkFormat format,
		VkImageUsageFlags usage,
		VkImageAspectFlags aspect
	) : device{ device }, extent{ extent }, format{ format }, usage{ usage }, aspect{ aspect } {

		assert(extent.width != 0 && "excuse me sir the extent.width is 0, msg from rap_image.cpp btw");
		assert(extent.height != 0 && "yea so uhm the extent.height is 0, fix from rap_image.cpp");

		createImage();

		try {
			createImageView();
		}
		catch(...){
			vkDestroyImage(device.device(), image, nullptr);
			vkFreeMemory(device.device(), imageMemory, nullptr);

			image = VK_NULL_HANDLE;
			imageMemory = VK_NULL_HANDLE;

			throw;
		}
	}

	RaptorImage::~RaptorImage() {
		if (imageView != VK_NULL_HANDLE) {
			vkDestroyImageView(
				device.device(), imageView, nullptr);
		}

		if (image != VK_NULL_HANDLE) {
			vkDestroyImage(
				device.device(), image, nullptr);
		}

		if (imageMemory != VK_NULL_HANDLE) {
			vkFreeMemory(
				device.device(), imageMemory, nullptr);
		}
	}

	void RaptorImage::createImage() {

		//imageInfo
		VkImageCreateInfo imageInfo{};
		imageInfo.sType =
			VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;


		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = extent.width;
		imageInfo.extent.height = extent.height;
		imageInfo.extent.depth = 1;

		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;

		imageInfo.format = format;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;

		imageInfo.initialLayout =
			VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;

		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode =
			VK_SHARING_MODE_EXCLUSIVE;

		//device make image
		device.createImageWithInfo(
			imageInfo,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			image,
			imageMemory
		);
	}

	void RaptorImage::createImageView() {
		
		//viewInfo
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType =
			VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;

		viewInfo.subresourceRange.aspectMask = aspect;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(device.device(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create RaptorImage imageView");
		}
	}

}