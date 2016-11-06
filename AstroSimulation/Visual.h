#pragma once
// GLEW
#define GLEW_STATIC
#include <GL/glew.h>
#include <GL/GLU.h>

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

//For some reason, everything seems to get clipped on a far plane, despite using infinite perspective
//probably some GL depth rule that I don't know about
//set this max depth value to stop zooming out before it gets to that far plane. 
#define MAX_DEPTH -4.0*pow(10,6) 

class Visual
{
public:
	Visual(char simulationSource[]);
	~Visual();

	void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
	void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
	void scroll_callback(GLFWwindow* window, double xpos, double ypos);

	void setView();
	void update();

	Simulation simulation;
	GLFWwindow* window;

private:
	const GLuint width = 800;
	const GLuint height = 800;

	GLuint VBO;
	GLuint VAO;

	std::vector<GLfloat> vertices;

	// Deltatime
	GLfloat deltaTime = 0.0f;	// Time between current frame and last frame
	GLfloat lastFrame = 0.0f;  	// Time of last frame

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

	bool updateView = false;
	Camera camera;
};

Visual::Visual(char simulationSource[]) 
	: simulation(simulationSource)
{
	// Init GLFW
	glfwInit();
	// Set all the required options for GLFW
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	// Create a GLFWwindow object that we can use for GLFW's functions
	window = glfwCreateWindow(width, height, "LearnOpenGL", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	// Set this to true so GLEW knows to use a modern approach to retrieving function pointers and extensions
	glewExperimental = GL_TRUE;
	// Initialize GLEW to setup the OpenGL Function pointers
	glewInit();

	// Define the viewport dimensions
	glViewport(0, 0, width, height);

	// Setup OpenGL options
	glEnable(GL_DEPTH_TEST);

	vertices = {};
	for (int i = 0; i < simulation.objects.size(); i++) {
		vertices.push_back(simulation.objects[i].position[0]);
		vertices.push_back(simulation.objects[i].position[1]);
		vertices.push_back(simulation.objects[i].position[2]);

		vertices.push_back(simulation.objectSettings[i].color[0]);
		vertices.push_back(simulation.objectSettings[i].color[1]);
		vertices.push_back(simulation.objectSettings[i].color[2]);
	}

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), &vertices[0], GL_STATIC_DRAW);

	// Position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	// Color attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	glBindVertexArray(0); // Unbind VAO

	camera = Camera();

	xTranslate = 0.0;
	yTranslate = 0.0;
	zTranslate = -1000000.0;
	setView();
	model = glm::mat4();

}

Visual::~Visual()
{
	// Properly de-allocate all resources once they've outlived their purpose
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	// Terminate GLFW, clearing any resources allocated by GLFW.
	glfwTerminate();
}

void Visual::update() 
{
	// Calculate deltatime of current frame
	GLfloat currentFrame = glfwGetTime();
	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;

	// Check if any events have been activiated (key pressed, mouse moved etc.) and call corresponding response functions
	glfwPollEvents();

	// Render
	// Clear the colorbuffer
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Build and compile our shader program
	// todo: make shader a part of the class so I only need to initialize in constructor
	Shader shader("shader.vertex", "shader.fragment");

	// Activate shader
	shader.Use();
	
	//set view dimensions
	//projection = glm::ortho(xRange[0], xRange[1], yRange[0], yRange[1], zRange[0], zRange[1]);
	projection = glm::mat4();

	projection = glm::infinitePerspective(
		(float)glm::radians(camera.Zoom), // view angle, usually 90° in radians
		(float)width / height, // aspect ratio
		0.1f // near clipping plane, should be > 0
	);

	if(updateView)
		setView();

	// Get their uniform location
	GLint modelLoc = glGetUniformLocation(shader.Program, "model");
	GLint viewLoc = glGetUniformLocation(shader.Program, "view");
	GLint projLoc = glGetUniformLocation(shader.Program, "projection");
	// Pass them to the shaders
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	// Draw container
	glBindVertexArray(VAO);
	glPointSize(2.0);
	glDrawArrays(GL_POINTS, 0, 6 * simulation.objects.size());
	glBindVertexArray(0);

	// Swap the screen buffers
	glfwSwapBuffers(window);
}

	void Visual::setView() {
		view = glm::mat4();

		view = glm::translate(view, { xTranslate,yTranslate,zTranslate });

		std::cout << camera.inclination << " " << camera.azimuth << std::endl;
		view = glm::translate(view, { -xTranslate,-yTranslate,0.0 });
		view = glm::rotate(view, glm::radians(camera.inclination), glm::vec3(1.f, 0.f, 0.f));
		view = glm::rotate(view, glm::radians(camera.azimuth), glm::vec3(0.f, 0.f, 1.f));
		view = glm::translate(view, { xTranslate,yTranslate,0.0 });


		camera.Right = glm::column(view, 0).xyz();
		camera.Up = glm::column(view, 1).xyz();
		camera.Front = -glm::column(view, 2).xyz(); // minus because OpenGL camera looks towards negative Z.
		camera.Position = glm::column(view, 3).xyz();

		updateView = false;
	}

//this is some crazy C++ magic to deal with the fact that glfwSet{...}Callback cannot take non-static class methods
//This was a problem because I want to access and change member variables from within the callback
//http://stackoverflow.com/questions/7676971/pointing-to-a-function-that-is-a-class-member-glfw-setkeycallback
void initVisualControls (Visual* visual)
{
	glfwSetWindowUserPointer(visual->window, visual);

	auto mouse_button_callback = [](GLFWwindow* window, int button, int action, int mods)
	{
		static_cast<Visual*>(glfwGetWindowUserPointer(window))->mouse_button_callback(window, button, action, mods);
	};

	glfwSetMouseButtonCallback(visual->window, mouse_button_callback);

	auto cursor_position_callback = [](GLFWwindow* window, double xpos, double ypos)
	{
		static_cast<Visual*>(glfwGetWindowUserPointer(window))->cursor_position_callback(window, xpos, ypos);
	};

	glfwSetCursorPosCallback(visual->window, cursor_position_callback);

	auto scroll_callback = [](GLFWwindow* window, double xoffset, double yoffset)
	{
		static_cast<Visual*>(glfwGetWindowUserPointer(window))->scroll_callback(window, xoffset, yoffset);
	};

	glfwSetScrollCallback(visual->window, scroll_callback);

}

void Visual::mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		leftMousePressed = true;
	}

	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
		leftMousePressed = false;
	}

	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
		rightMousePressed = true;
	}

	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
		rightMousePressed = false;
	}

}

void Visual::cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (leftMousePressed) {
		GLfloat xoffset = xpos - cursorPrevX;
		GLfloat yoffset = cursorPrevY - ypos;

		float fovY = tan(glm::radians(camera.Zoom / 2)) * 2 * -zTranslate;
		float fovX = fovY * (width / height);

		xTranslate += xoffset * fovX / width;
		yTranslate += yoffset * fovY / height;

		updateView = true;
	}

	if (rightMousePressed) {
		GLfloat xoffset = (xpos - cursorPrevX) / 4.0;
		GLfloat yoffset = (ypos - cursorPrevY) / 4.0;

		camera.inclination = glm::clamp(camera.inclination - yoffset, 0.f, 90.f);
		camera.azimuth = fmodf(camera.azimuth + xoffset, 360.f);

		updateView = true;
	}

	cursorPrevX = xpos;
	cursorPrevY = ypos;
}

void Visual::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	glm::mat4 modelview = view*model;
	glm::vec4 viewport = { 0.0, 0.0, width, height };

	float winX = cursorPrevX;
	float winY = viewport[3] - cursorPrevY;
	float winZ;

	glReadPixels(winX, winY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ);
	glm::vec3 screenCoords = { winX, winY, winZ };

	glm::vec3 cursorPosition = glm::unProject(screenCoords, modelview, projection, viewport);

	if (isinf(cursorPosition[2]) || isnan(cursorPosition[2])) {
		cursorPosition[2] = 0.0;
	}
	
	float zoomFactor = 1.1;
	// = zooming in 
	if (yoffset > 0.0)
		zoomFactor = 1/1.1;
	
	//the width and height of the perspective view, at the depth of the cursor position 
	glm::vec2 fovXY = camera.getFovXY(cursorPosition[2] - zTranslate, width / height);
	camera.setZoomFromFov(fovXY.y * zoomFactor, cursorPosition[2] - zTranslate);

	if (camera.Zoom > 45.0 && zTranslate * 2 > MAX_DEPTH) {
		camera.Zoom = 45.0;
	}

	//don't want fov to be greater than 90, so cut it in half and move the world farther away from the camera to compensate
	//not working...
	//if (camera.Zoom > 45.0 && zTranslate*2 > MAX_DEPTH) {
	//	float prevZoom = camera.Zoom;
	//	camera.Zoom *= .5;

	//	zTranslate *= 2;
	//	std::cout << zTranslate << std::endl;
	//	//need increased distance between camera and world origin, so that view does not appear to change when fov is reduced
	//	//zTranslate = cursorPosition.z - tan(glm::radians(prevZoom)) / tan(glm::radians(camera.Zoom) * (cursorPosition.z - zTranslate));
	//}
	//else if (camera.Zoom > 45.0) {
	//	camera.Zoom = 45.0;
	//}

	glm::vec2 newFovXY = camera.getFovXY(cursorPosition[2] - zTranslate, width / height);

	//translate so that position under the cursor does not appear to move.
	xTranslate += (newFovXY.x - fovXY.x) * (winX / width - .5);
	yTranslate += (newFovXY.y - fovXY.y) * (winY / height - .5);

	updateView = true;
}