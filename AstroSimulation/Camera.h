#pragma once

// Std. Includes
#include <vector>

// GL Includes
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>


// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
enum Camera_Movement {
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT
};

// Default camera values
const GLfloat YAW = 0.0f;
const GLfloat PITCH = 0.0f;
const GLfloat SPEED = 3.0f;
const GLfloat SENSITIVTY = 0.25f;
const GLfloat ZOOM = 90.0f;


// An abstract camera class that processes input and calculates the corresponding Eular Angles, Vectors and Matrices for use in OpenGL
class Camera
{
public:
	// Camera Attributes
	glm::vec3 Position;
	glm::vec3 Front;
	glm::vec3 Up;
	glm::vec3 Right;
	glm::vec3 WorldUp;
	glm::vec3 Target;
	GLfloat azimuth;
	GLfloat inclination;

	// Camera options
	GLfloat MovementSpeed;
	GLfloat MouseSensitivity;
	GLfloat Zoom;

	// Constructor with vectors
	Camera()
	{
		Position = { 0.0,0.0,0.0 };
		Up = { 0.0,1.0,0.0 };
		WorldUp = { 0.0,1.0,0.0 };
		Front = { 0.0,0.0,-1.0 };
		Right = { 1.0,0.0,0.0 };
		Target = { 0.0,0.0,0.0 };

		azimuth = 1.0;
		inclination = 1.0;

		Zoom = 90.0f;
	}

	glm::vec2 getFovXY(float depth, float aspectRatio) {
		float fovY = tan(glm::radians(Zoom / 2)) * depth;
		float fovX = fovY * aspectRatio;

		return glm::vec2{ 2*fovX , 2*fovY };
	}

	//you have a desired fov, and you want to set the zoom to achieve that.
	//factor of 1/2 inside the atan because we actually need the half-fov. Keep full-fov as input for consistency 
	void setZoomFromFov(float fovY, float depth) {
		Zoom = glm::degrees(2 * atan(fovY / (2 * depth)));
	}
};