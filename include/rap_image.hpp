#pragma once

#include "buffers/rap_buffer.hpp"
#include "rap_device.hpp"
#include <vulkan/vulkan.h>

//std
#include <cstdint>
#include <vector>

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
		//function overload
		RaptorImage(
			RaptorDevice& device,
			VkExtent2D extent,
			VkFormat format,
			VkImageAspectFlags aspect,
			const std::vector<unsigned char>& pixels
		);

		~RaptorImage();

		RaptorImage(const RaptorImage&) = delete;
		RaptorImage& operator=(const RaptorImage&) = delete;

		VkImage getImage() const { return image; }
		VkImageView getImageView() const { return imageView; }
		VkFormat getFormat() const { return format; }
		VkExtent2D getExtent() const { return extent; }
		VkSampler getSampler() const { return sampler; }

	private:
		void createImage();
		void createImageView();
		void createSampler();
		void uploadPixels(const std::vector<unsigned char>& pixels);
		void transitionImageLayout(
			VkImageLayout oldLayout, 
			VkImageLayout newLayout);

		RaptorDevice& device;
		VkExtent2D extent;
		VkFormat format;
		VkImageUsageFlags usage;
		VkImageAspectFlags aspect;

		VkImage image = VK_NULL_HANDLE;
		VkDeviceMemory imageMemory = VK_NULL_HANDLE;
		VkImageView imageView = VK_NULL_HANDLE;
		VkSampler sampler = VK_NULL_HANDLE;
	};

} // namespace rap

