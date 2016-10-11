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
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"

#include "Simulation.h"
#include <vector>

class Visual
{
public:
	Visual(char simulationSource[]);
	~Visual();

	void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
	void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
	void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
	void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

	void UpdateModelViewProjection();
	void do_movement();
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

	bool keys[1024];
	GLfloat elevation = 90.0f;
	GLfloat azimuth = 0.0f;

	double dx = 0.0;
	double dy = 0.0;
	double cursorPrevX;
	double cursorPrevY;
	bool leftMousePressed = false;

	glm::vec2 xRange = { -5000.0,5000.0 };
	glm::vec2 yRange = { -5000.0,5000.0 };
	glm::vec2 zRange = { -5000.0,5000.0 };

	glm::vec3 zoomTranslate = { 0.0, 0.0, 0.0 };

	glm::mat4 model;
	glm::mat4 projection;
	glm::mat4 view;

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

}

Visual::~Visual()
{
	// Properly de-allocate all resources once they've outlived their purpose
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	// Terminate GLFW, clearing any resources allocated by GLFW.
	glfwTerminate();
}

// Is called whenever a key is pressed/released via GLFW
void Visual::key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
			keys[key] = true;
		else if (action == GLFW_RELEASE)
			keys[key] = false;
	}
}

void Visual::mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		leftMousePressed = true;
	}

	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
		leftMousePressed = false;
	}

}

void Visual::cursor_position_callback(GLFWwindow* window, double xpos, double ypos) 
{
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	double scaleX = (xRange[1] - xRange[0]) / viewport[2];
	double scaleY = (yRange[1] - yRange[0]) / viewport[3];

	if (leftMousePressed) {
		//positions in pixels * scale = position in world coordinates
		dx = scaleX * (xpos - cursorPrevX);
		dy = scaleY * (cursorPrevY - ypos);

		xRange[0] -= dx;
		xRange[1] -= dx;

		yRange[0] -= dy;
		yRange[1] -= dy;
	}

	cursorPrevX = xpos;
	cursorPrevY = ypos;
}
//
void Visual::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	double zoomFactor = 1.1;
	if (yoffset > 0)
		zoomFactor = 1 / 1.1;

	UpdateModelViewProjection();
	glm::mat4 modelview = view*model;
	glm::vec4 viewport = { 0.0, 0.0, width, height };
	
	double winX = cursorPrevX;//282.0;
	double winY = width - cursorPrevY;//735.0;
	//double winZ;
	//glReadPixels(winX, winY, 1.0, 1.0, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ);

	//GLdouble posX, posY, posZ;
	glm::vec3 screenCoords = { winX, winY, 0.0 };
	glm::vec3 position = glm::unProject(screenCoords, modelview, projection, viewport);

	std::cout << position[0] << " " << position[1] << std::endl;

	double leftSegment = (position.x - xRange[0]) * zoomFactor;
	xRange[0] = position.x - leftSegment;

	double rightSegment = (xRange[1] - position.x) * zoomFactor;
	xRange[1] = position.x + rightSegment;

	double bottomSegment = (position.y - yRange[0]) * zoomFactor;
	yRange[0] = position.y - bottomSegment;

	double topSegment = (yRange[1] - position.y) * zoomFactor;
	yRange[1] = position.y + topSegment;
}

void Visual::do_movement()
{
	// Camera controls
	GLfloat cameraSpeed = 200.0f * deltaTime;
	if (keys[GLFW_KEY_W]) {
		elevation += cameraSpeed;
		if (elevation > 90.0f)
			elevation = 90.0f;
	}
	if (keys[GLFW_KEY_S]) {
		elevation -= cameraSpeed;
		if (elevation < 0.0f)
			elevation = 0.0f;
	}

	if (keys[GLFW_KEY_D]) {
		azimuth += cameraSpeed;
		if (azimuth > 360.0f)
			azimuth = 360.0f;
	}
	if (keys[GLFW_KEY_A]) {
		azimuth -= cameraSpeed;
		if (azimuth < 0.0f)
			azimuth = 0.0f;
	}
}

void Visual::update() 
{
	// Calculate deltatime of current frame
	GLfloat currentFrame = glfwGetTime();
	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;

	// Check if any events have been activiated (key pressed, mouse moved etc.) and call corresponding response functions
	glfwPollEvents();
	do_movement();


	// Render
	// Clear the colorbuffer
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Build and compile our shader program
	// todo: make shader a part of the class so I only need to initialize in constructor
	Shader shader("shader.vertex", "shader.fragment");

	// Activate shader
	shader.Use();

	//model = glm::rotate(model, glm::radians((GLfloat)glfwGetTime() * 50.0f), glm::vec3(0.5f, 1.0f, 0.0f));

	UpdateModelViewProjection();

	// Camera/View transformation
	//GLfloat radius = 1000;
	//GLfloat camX = cos(glfwGetTime()) * radius;
	//GLfloat camY = sin(glfwGetTime()) * radius;
	//view = glm::lookAt(glm::vec3(0.0f, camX, camY), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

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

void Visual::UpdateModelViewProjection() {
	model = glm::mat4();
	projection = glm::mat4();
	view = glm::mat4();

	projection = glm::ortho(xRange[0], xRange[1], yRange[0], yRange[1], zRange[0], zRange[1]);
	//projection = glm::perspective(glm::radians(45.0f), (GLfloat)WIDTH / (GLfloat)HEIGHT, 0.1f, 5000.0f);

	//model = glm::translate(model, glm::vec3(dx, dy, 0.0));

	double midX = xRange[0] + (xRange[1] - xRange[0]) / 2;
	double midY = yRange[0] + (yRange[1] - yRange[0]) / 2;

	//model = glm::translate(model, glm::vec3(-midX, -midY, 0.0));
	model = glm::rotate(model, glm::radians((GLfloat)elevation - 90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians((GLfloat)azimuth), glm::vec3(0.0f, 0.0f, 1.0f));
	//model = glm::translate(model, glm::vec3(midX, midY, 0.0));

}

//this is some crazy C magic to deal with the fact that glfwSet{...}Callback cannot take non-static class methods
//This was a problem because I want to access and change member variables from within the callback
//http://stackoverflow.com/questions/7676971/pointing-to-a-function-that-is-a-class-member-glfw-setkeycallback
void initVisualControls (Visual* visual)
{

	glfwSetWindowUserPointer(visual->window, visual);

	auto key_callback = [](GLFWwindow* window, int key, int scancode, int action, int mode)
	{
		static_cast<Visual*>(glfwGetWindowUserPointer(window))->key_callback(window, key, scancode, action, mode);
	};

	glfwSetKeyCallback(visual->window, key_callback);

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