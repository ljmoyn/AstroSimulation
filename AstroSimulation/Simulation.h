#ifndef SIMULATION_H
#define SIMULATION_H
// GLEW
#define GLEW_STATIC
#include <GL/glew.h>
#include <GL/GLU.H>

// GLFW
#include <GLFW/glfw3.h>

// Other Libs
#include <soil/SOIL.h>
// GLM Mathematics
#define GLM_SWIZZLE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include <algorithm>

#include "Shader.h"

#include "Physics.h"
#include <vector>
#include "Camera.h"

#include "../imgui/imgui.h"
#include "ImguiUtil.h"
#include <stdio.h>
#include <sstream>
#include <math.h>

#include "Sphere.h"

class Simulation
{
public:
	Simulation(std::string physicsSource);
	~Simulation();

	void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
	void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
	void scroll_callback(GLFWwindow* window, double xpos, double ypos);
	void window_resize_callback(GLFWwindow* window, int x, int y);
	//void ShowMainUi();
	void setView();
	void update();

	static void updatePaths(Physics * physics, std::vector<std::vector<GLfloat> > * paths, bool firstFrame);
	static void initSimulationControls(Simulation* simulation);

	Physics physics;
	GLFWwindow* window;
	std::vector<std::vector<GLfloat> > paths;
	ImguiStatus imguiStatus;

	Camera camera;
	Sphere sphere = Sphere();
private:

	void LoadTextures();
	unsigned char* LoadDDS(std::string imagePath, int * format, int * mipmapCount, int * width, int * height);

	void drawPoints();
	void drawSpheres();
	void drawLines();

	GLuint pointsShaderProgram;
	GLuint objectsShaderProgram;
	GLuint pathsShaderProgram;

	//input are coordinates on a sphere
	float GetPixelDiameter(glm::vec4 surfacePoint, glm::vec4 centerPoint);
	void GetWindowCoordinates(glm::vec4 point, float * xWindow, float *yWindow);
	void GetVertexAttributeData(bool drawAsPoint, std::vector<GLfloat> * positions, std::vector<GLfloat> * colors, std::vector<GLint> * textureCoordinates, std::vector<glm::mat4> * objectOrientations, int * count);
	GLuint width = 800;
	GLuint height = 800;

	GLuint cubemap;
	GLuint textures;
	GLuint instanceModelsVBO;
	GLuint textureCoordinateVBO;
	GLuint textureIndexVBO;
	GLuint sphereVBO;
	GLuint positionVBO;
	GLuint colorVBO;
	GLuint vertexVBO;
	GLuint VAO;
	GLuint EBO;

	double cursorPrevX;
	double cursorPrevY;
	bool leftMousePressed = false;
	bool rightMousePressed = false;

	glm::mat4 model;
	glm::mat4 projection;
	glm::mat4 view;
	GLfloat xTranslate;
	GLfloat yTranslate;
	GLfloat zTranslate;

	void CheckGlError(std::string message = "");
	//static void APIENTRY Simulation::glDebugOutput(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam);
};
#endif