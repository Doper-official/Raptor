#pragma once

//my libs
#include "rap_device.hpp"
#include "buffers/rap_buffer.hpp"

//vulkan
#include <vulkan/vulkan.h>

//std
#include <cstdint>
#include <vector>
#include <array>

namespace rap {

	class RaptorCubeMap {
	public:

		RaptorCubeMap(
			RaptorDevice& device,
			uint32_t faceSize
			);

		void uploadFaces(
			const std::array<std::vector<unsigned char>, 6>& faces
		);

		~RaptorCubeMap();

		RaptorCubeMap(const RaptorCubeMap&) = delete;
		RaptorCubeMap& operator=(const RaptorCubeMap&) = delete;

		VkImage getImage() const { return image; }
		VkImageView getImageView() const { return imageView; }
		VkSampler getSampler() const { return sampler; }
		VkImageView getStorageImageView() const { return storageImageView; }
		uint32_t getFaceSize() const { return faceSize; }

	private:

		void createImage();
		void createImageView();
		void createSampler();
		void createStorageImageView();

		RaptorDevice& device;
		uint32_t faceSize;

		VkImage image = VK_NULL_HANDLE;
		VkDeviceMemory imageMemory = VK_NULL_HANDLE;
		VkImageView imageView = VK_NULL_HANDLE;
		VkSampler sampler = VK_NULL_HANDLE;
		VkImageView storageImageView = VK_NULL_HANDLE;
	};
}