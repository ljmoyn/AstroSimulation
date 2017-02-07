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
	void update(Shader shader);
	void drawVertices();
	void drawLines();
	static void updateLines(Simulation * simulation, std::vector<std::vector<GLfloat> > * lines, bool firstFrame);
	static void initVisualControls(Visual* visual);

	Simulation simulation;
	GLFWwindow* window;
	std::vector<std::vector<GLfloat> > lines;
	ImguiStatus imguiStatus;

	Camera camera;
private:
	GLuint width = 800;
	GLuint height = 800;

	GLuint VBO;
	GLuint VAO;

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