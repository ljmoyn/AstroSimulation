#ifndef VISUAL_H
#define VISUAL_H
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

#include "Simulation.h"
#include <vector>
#include "Camera.h"

#include "../imgui/imgui.h"
#include "ImguiUtil.h"
#include <stdio.h>
#include <sstream>
#include <math.h>

#include "Sphere.h"

class Visual
{
public:
	Visual(std::string simulationSource);
	~Visual();

	void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
	void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
	void scroll_callback(GLFWwindow* window, double xpos, double ypos);
	void window_resize_callback(GLFWwindow* window, int x, int y);
	//void ShowMainUi();
	void setView();
	void update();

	static void updatePaths(Simulation * simulation, std::vector<std::vector<GLfloat> > * paths, bool firstFrame);
	static void initVisualControls(Visual* visual);

	Simulation simulation;
	GLFWwindow* window;
	std::vector<std::vector<GLfloat> > paths;
	ImguiStatus imguiStatus;

	Camera camera;
	Sphere sphere = Sphere();


	
private:

	void drawObjects();
	void drawPoints();
	void drawSpheres();
	void drawLines();

	GLuint simulationObjectsShaderProgram;
	GLuint simulationPathsShaderProgram;

	//input are coordinates on a sphere
	float GetPixelDiameter(glm::vec4 surfacePoint, glm::vec4 centerPoint);
	void GetWindowCoordinates(glm::vec4 point, float * xWindow, float *yWindow);
	void GetVertexAttributeData(bool drawAsPoint, std::vector<GLfloat> * positions, std::vector<GLfloat> * colors, int * count);
	GLuint width = 800;
	GLuint height = 800;

	std::vector<GLuint> textures;
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


};
#endif