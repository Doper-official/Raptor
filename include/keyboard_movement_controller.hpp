#pragma once

#include "rap_game_object.hpp"
#include "rap_window.hpp"

namespace rap {

	class KeyboardMovementController {

	public:
		struct KeyMapping {
			int moveLeft = GLFW_KEY_A;
			int moveRight = GLFW_KEY_D;
			int moveForward = GLFW_KEY_W;
			int moveBackward = GLFW_KEY_S;
			int moveUp = GLFW_KEY_E;
			int moveDown = GLFW_KEY_Q;
		};

		void moveInPlaneXZ(GLFWwindow* window, float dt, RaptorGameObject& gameObject);

		KeyMapping keys{};

		float moveSpeed{ 3.f };
		float turnSpeed{ 1.5f };

	private:
		bool firstMouse = true;
		double lastMouseX = 0.0;
		double lastMouseY = 0.0;
	};

}