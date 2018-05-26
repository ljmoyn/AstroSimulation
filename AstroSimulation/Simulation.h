#ifndef SIMULATION_H
#define SIMULATION_H

#include "Physics.h"
#include "Graphics.h"
#include "UserInterface.h"

class Simulation
{
public:
	Simulation(std::string physicsSource);
	~Simulation();

	void update();
	static void MouseButtonWrapper(GLFWwindow* window, int button, int action, int mods);
	void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

	static void CursorPositionWrapper(GLFWwindow* window, double xpos, double ypos);
	void CursorPositionCallback(GLFWwindow* window, double xpos, double ypos);

	static void ScrollWrapper(GLFWwindow* window, double xpos, double ypos);
	void ScrollCallback(GLFWwindow* window, double xpos, double ypos);
	static void WindowResizeWrapper(GLFWwindow* window, int x, int y);
	void WindowResizeCallback(GLFWwindow* window, int x, int y);

	Physics physics;
	Graphics graphics;
	UserInterface userInterface;

private:
	float GetScrollAdjustment(float a1, float b1, float deltaZ);

	double cursorPrevX;
	double cursorPrevY;
	bool leftMousePressed = false;
	bool rightMousePressed = false;
};
#endif