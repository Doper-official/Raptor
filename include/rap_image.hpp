#pragma once

#include "rap_device.hpp"
#include <vulkan/vulkan.h>

namespace rap {

	class RaptorImage {
	public:
		RaptorImage(
			RaptorDevice& device,
			VkExtent2D extent,
			VkFormat format,
			VkImageUsageFlags usage,
			VkImageAspectFlags aspect
			);
		~RaptorImage();

		RaptorImage(const RaptorImage&) = delete;
		RaptorImage& operator=(const RaptorImage&) = delete;

		VkImage getImage() const { return image; }
		VkImageView getImageView() const { return imageView; }
		VkFormat getFormat() const { return format; }
		VkExtent2D getExtent() const { return extent; }

	private:
		void createImage();
		void createImageView();

		RaptorDevice& device;
		VkExtent2D extent;
		VkFormat format;
		VkImageUsageFlags usage;
		VkImageAspectFlags aspect;

		VkImage image = VK_NULL_HANDLE;
		VkDeviceMemory imageMemory = VK_NULL_HANDLE;
		VkImageView imageView = VK_NULL_HANDLE;
	};

} // namespace rap

