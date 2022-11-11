#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

//Default values for camera properties
const float FOV = 60.0f;
const glm::vec3 POSITION = glm::vec3(0.0f);
const float YAW = -90.0f;
const float PITCH = 0.0f;

class Camera
{
public:
	glm::vec3 camPos;
	float fov;

	glm::mat4 getViewMatrix() const
	{
		return glm::lookAt(camPos, camPos + viewDir, up);
	}
	
	Camera(glm::vec3 camPos = POSITION, float yaw = YAW, float pitch = PITCH, float fov = FOV) :
		camPos(camPos), fov(fov), yaw(yaw), pitch(pitch) {
		updateVectors();
	}

	void updateVectors() {
		viewDir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		viewDir.y = sin(glm::radians(pitch));
		viewDir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

		viewDir = glm::normalize(viewDir);

		right = glm::normalize(glm::cross(viewDir, worldUp));
		up = glm::normalize(glm::cross(right, viewDir));
	}

	//Prints the camera parameters in a format that can be copied and pasted into the camera constructor to get an identical
	// view between application runs for evaluations
	void printParameters() {
		printf("(glm::vec3(%f, %f, %f), %f, %f, %f)", camPos.x, camPos.y, camPos.z, yaw, pitch, fov);
	}


	virtual void processKeyboardInput(GLFWwindow* window, float delta) = 0;
	virtual void processMouseMovement(GLFWwindow* window, float dx, float dy) = 0;

protected:
	glm::vec3 viewDir = glm::vec3(0.0f);
	glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 right;
	glm::vec3 up;
	float yaw;
	float pitch;
};