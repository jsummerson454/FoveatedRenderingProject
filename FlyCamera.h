#pragma once

#include "camera.h"
#include <algorithm>

class FlyCamera : public Camera 
{
public:
	// Inherited via Camera
	void processKeyboardInput(GLFWwindow* window, float dt) override {
		float x = 0, y = 0, z = 0;
		float velocity = 2.0f;

		float d = velocity * dt;
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
			camPos += viewDir * d;
		}
		else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
			camPos -= viewDir * d;
		}
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
			camPos += right * d;
		}
		else if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
			camPos -= right * d;
		}
		camPos = camPos + glm::vec3(x, y, z);
	}

	void processMouseMovement(GLFWwindow* window, float dx, float dy) override {
		yaw += dx * mouseSensitivity;
		pitch += dy * mouseSensitivity;

		//clamp pitch so that the camera doesn't flip
		pitch = std::max(-89.0f, std::min(89.0f, pitch));

		updateVectors();		
	}

	FlyCamera(glm::vec3 camPos = POSITION, float yaw = YAW, float pitch = PITCH, float fov = FOV) :
		Camera(camPos, yaw, pitch, fov) {}

private:
	float mouseSensitivity = 0.1f;
};