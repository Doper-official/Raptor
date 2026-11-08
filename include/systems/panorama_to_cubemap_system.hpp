#pragma once

//my libs
#include "rap_device.hpp"
#include "rap_descriptors.hpp"
#include "rap_image.hpp"
#include "rap_cubemap.hpp"

//std
#include <memory>

//vulkan
#include <vulkan/vulkan.h>

namespace rap {

	class PanoramaToCubeMapSystem {
	public:

		explicit PanoramaToCubeMapSystem(
			RaptorDevice& device
		);

		~PanoramaToCubeMapSystem();

		PanoramaToCubeMapSystem(const PanoramaToCubeMapSystem&) = delete;
		PanoramaToCubeMapSystem& operator=(const PanoramaToCubeMapSystem&) = delete;

		void convert(
			RaptorImage& panorama,
			RaptorCubeMap& cubemap
		);

	private:

		void createDescriptorLayout();
		void createDescriptorPool();
		void createPipelineLayout();
		void createPipeline();

		RaptorDevice& device;

		std::unique_ptr<RaptorDescriptorSetLayout> descriptorSetLayout;
		std::unique_ptr<RaptorDescriptorPool> descriptorPool;

		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		VkPipeline pipeline = VK_NULL_HANDLE;

		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
	};
}