#include "keyboard_movement_controller.hpp"

#include <limits>
#include <cmath>

#include <glm/gtc/constants.hpp>

namespace rap {

	void KeyboardMovementController::moveInPlaneXZ(
		GLFWwindow* window,
		float dt,
		RaptorGameObject& gameObject) {

		// -------------------------
		// Mouse look
		// -------------------------

		double mouseX;
		double mouseY;

		glfwGetCursorPos(window, &mouseX, &mouseY);

		if (firstMouse) {
			lastMouseX = mouseX;
			lastMouseY = mouseY;
			firstMouse = false;
		}

		double mouseDeltaX = mouseX - lastMouseX;
		double mouseDeltaY = mouseY - lastMouseY;

		lastMouseX = mouseX;
		lastMouseY = mouseY;

		// Mouse sensitivity.
		const float mouseSensitivity = 0.002f;

		gameObject.transform.rotation.y +=
			static_cast<float>(mouseDeltaX) * mouseSensitivity;

		gameObject.transform.rotation.x -=
			static_cast<float>(mouseDeltaY) * mouseSensitivity;

		// Limit pitch to about +/- 85 degrees.
		gameObject.transform.rotation.x =
			glm::clamp(gameObject.transform.rotation.x, -1.5f, 1.5f);

		// Keep yaw within 0 -> 2PI.
		gameObject.transform.rotation.y =
			glm::mod(gameObject.transform.rotation.y, glm::two_pi<float>());

		// -------------------------
// Movement
// -------------------------

		float yaw = gameObject.transform.rotation.y;

		const glm::vec3 forwardDir{
			std::sin(yaw),
			0.f,
			std::cos(yaw)
		};

		const glm::vec3 rightDir{
			forwardDir.z,
			0.f,
			-forwardDir.x
		};

		const glm::vec3 upDir{
			0.f,
			1.f,
			0.f
		};

		glm::vec3 moveDir{ 0.f };

		if (glfwGetKey(window, keys.moveForward) == GLFW_PRESS)
			moveDir += forwardDir;

		if (glfwGetKey(window, keys.moveBackward) == GLFW_PRESS)
			moveDir -= forwardDir;

		if (glfwGetKey(window, keys.moveRight) == GLFW_PRESS)
			moveDir += rightDir;

		if (glfwGetKey(window, keys.moveLeft) == GLFW_PRESS)
			moveDir -= rightDir;

		if (glfwGetKey(window, keys.moveUp) == GLFW_PRESS)
			moveDir -= upDir;

		if (glfwGetKey(window, keys.moveDown) == GLFW_PRESS)
			moveDir += upDir;

		if (glm::dot(moveDir, moveDir) >
			std::numeric_limits<float>::epsilon()) {

			moveDir = glm::normalize(moveDir);

			gameObject.transform.translation +=
				moveSpeed * dt * moveDir;
		}
	}
}