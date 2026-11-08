
#include "systems/panorama_to_cubemap_system.hpp"

//std
#include <fstream>
#include <stdexcept>
#include <vector>
#include <cstdint>

namespace rap {

	namespace {

		std::vector<char> readFile(const std::string& filepath) {

			std::ifstream file{
				filepath,
				std::ios::ate | std::ios::binary
			};

			if (!file.is_open()) {
				throw std::runtime_error("Failed to open Compute shader: " + filepath);
			}

			const size_t fileSize =
				static_cast<size_t>(file.tellg());

			std::vector<char> buffer(fileSize);

			file.seekg(0);
			file.read(buffer.data(), fileSize);

			file.close();

			return buffer;
		}
	}

	PanoramaToCubeMapSystem::PanoramaToCubeMapSystem(
		RaptorDevice& device
	) : device{ device } {

		createDescriptorLayout();
		createDescriptorPool();
		createPipelineLayout();
		createPipeline();
	}

	PanoramaToCubeMapSystem::~PanoramaToCubeMapSystem() {

		vkDeviceWaitIdle(device.device());

		if (pipeline != VK_NULL_HANDLE) {
			vkDestroyPipeline(
				device.device(),
				pipeline,
				nullptr
			);
		}

		if (pipelineLayout != VK_NULL_HANDLE) {
			vkDestroyPipelineLayout(
				device.device(),
				pipelineLayout,
				nullptr
			);
		}
	}

	void PanoramaToCubeMapSystem::createDescriptorLayout() {

		descriptorSetLayout =
			RaptorDescriptorSetLayout::Builder{ device }
			.addBinding(
				0,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_SHADER_STAGE_COMPUTE_BIT
			)
			.addBinding(
				1,
				VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				VK_SHADER_STAGE_COMPUTE_BIT
			)
			.build();
	}

	void PanoramaToCubeMapSystem::createDescriptorPool() {

		descriptorPool =
			RaptorDescriptorPool::Builder{ device }
			.addPoolSize(
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				1
			)
			.addPoolSize(
				VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				1
			)
			.setMaxSets(1)
			.build();
	}

	void PanoramaToCubeMapSystem::createPipelineLayout() {

		VkDescriptorSetLayout setLayout =
			descriptorSetLayout->getDescriptorSetLayout();

		VkPipelineLayoutCreateInfo layoutInfo{};

		layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutInfo.setLayoutCount = 1;
		layoutInfo.pSetLayouts = &setLayout;

		if (vkCreatePipelineLayout(
			device.device(),
			&layoutInfo,
			nullptr,
			&pipelineLayout
		) != VK_SUCCESS) {
			throw std::runtime_error("Failed to Create panorama compute pipeline layout");
		}
	}

	void PanoramaToCubeMapSystem::createPipeline() {

		const auto code =
			readFile("shaders/panorama_to_cubemap.comp.spv");

		VkShaderModuleCreateInfo shaderModuleInfo{};

		shaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderModuleInfo.codeSize = code.size();
		shaderModuleInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
		
		VkShaderModule shaderModule = VK_NULL_HANDLE;

		if (vkCreateShaderModule(
			device.device(),
			&shaderModuleInfo,
			nullptr,
			&shaderModule
		) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Panorama Shader module!");
		}

		VkPipelineShaderStageCreateInfo shaderStage{};

		shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

		shaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		shaderStage.module = shaderModule;
		shaderStage.pName = "main";

		VkComputePipelineCreateInfo pipelineInfo{};

		pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;

		pipelineInfo.stage = shaderStage;
		pipelineInfo.layout = pipelineLayout;

		const VkResult result = vkCreateComputePipelines(
			device.device(),
			VK_NULL_HANDLE,
			1,
			&pipelineInfo,
			nullptr,
			&pipeline
		);

		vkDestroyShaderModule(
			device.device(),
			shaderModule,
			nullptr
		);

		if (result != VK_SUCCESS) {
			throw std::runtime_error("Failed to create panorama compute pipeline");
		}
	}

	void PanoramaToCubeMapSystem::convert(
		RaptorImage& panorama,
		RaptorCubeMap& cubemap
	) {

		//panorama input
		VkDescriptorImageInfo panoramaInfo{};

		panoramaInfo.sampler = panorama.getSampler();
		panoramaInfo.imageView = panorama.getImageView();
		panoramaInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		//cubemap output
		VkDescriptorImageInfo cubemapInfo{};

		cubemapInfo.imageView = cubemap.getStorageImageView();
		cubemapInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		RaptorDescriptorWriter writer{
			*descriptorSetLayout,
			*descriptorPool
		};

		writer.writeImage(0, &panoramaInfo);
		writer.writeImage(1, &cubemapInfo);

		if (!writer.build(descriptorSet)) {
			throw std::runtime_error("Failed to Create Panorama compute descriptor set");
		}

		VkCommandBuffer commandBuffer =
			device.beginSingleTimeCommands();

		//transition cubemap from undefined to general
		VkImageMemoryBarrier toGeneral{};

		toGeneral.sType =
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		toGeneral.srcAccessMask = 0;
		toGeneral.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

		toGeneral.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		toGeneral.newLayout = VK_IMAGE_LAYOUT_GENERAL;

		toGeneral.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		toGeneral.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		toGeneral.image = cubemap.getImage();

		toGeneral.subresourceRange.aspectMask =
			VK_IMAGE_ASPECT_COLOR_BIT;

		toGeneral.subresourceRange.baseMipLevel = 0;
		toGeneral.subresourceRange.levelCount = 1;
		toGeneral.subresourceRange.baseArrayLayer = 0;
		toGeneral.subresourceRange.layerCount = 6;

		vkCmdPipelineBarrier(
			commandBuffer,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			0,
			0,
			nullptr,
			0,
			nullptr,
			1,
			&toGeneral
		);

		//dispatch compute shader
		vkCmdBindPipeline(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			pipeline
		);

		vkCmdBindDescriptorSets(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			pipelineLayout,
			0,
			1,
			&descriptorSet,
			0,
			nullptr
		);

		const uint32_t faceSize = cubemap.getFaceSize();

		vkCmdDispatch(
			commandBuffer,
			(faceSize + 7) / 8,
			(faceSize + 7) / 8,
			6
		);

		//make Compute shader writes visible to fragment shader reads
		VkImageMemoryBarrier toShaderRead{};

		toShaderRead.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		toShaderRead.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		toShaderRead.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		toShaderRead.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		toShaderRead.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		toShaderRead.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		toShaderRead.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		toShaderRead.image = cubemap.getImage();

		toShaderRead.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		toShaderRead.subresourceRange.baseMipLevel = 0;
		toShaderRead.subresourceRange.levelCount = 1;
		toShaderRead.subresourceRange.baseArrayLayer = 0;
		toShaderRead.subresourceRange.layerCount = 6;

		vkCmdPipelineBarrier(
			commandBuffer,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0,
			0,
			nullptr,
			0,
			nullptr,
			1,
			&toShaderRead
		);

		device.endSingleTimeCommands(commandBuffer);
	}
}// namespace rap