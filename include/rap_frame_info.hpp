#pragma once

//my libs
#include "rap_camera.hpp"
#include "rap_game_object.hpp"
//lib
#include <vulkan/vulkan.h>

namespace rap {

#define MAX_LIGHTS 10
	struct PointLight {
		glm::vec4 position{}; //ignore w ig
		glm::vec4 color{}; // w is the lihgt intensity

	};

	struct GlobalUbo {
		alignas(16) glm::mat4 projection{ 1.f };
		alignas(16) glm::mat4 view{ 1.f };
		alignas(16) glm::mat4 inverseView{ 1.f };
		alignas(16) glm::vec4 ambientLightColor{ 1.f, 1.f, 1.f ,.01f }; // w is same as below (i will regret not putting actuall comments so oops)
		PointLight pointLights[MAX_LIGHTS];
		int numLights;
	};

	struct FrameInfo {
		int frameIndex;
		float frameTime;
		VkCommandBuffer commandBuffer;
		RaptorCamera& camera;
		VkDescriptorSet globalDescriptorSet;
		RaptorGameObject::Map& gameObjects;
	};

}//namespace rap