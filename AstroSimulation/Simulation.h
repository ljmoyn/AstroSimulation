#ifndef SIMULATION_H
#define SIMULATION_H

#include "Physics.h"
#include "Graphics.h"

#include "../imgui/imgui.h"
#include "ImguiUtil.h"

class Simulation
{
public:
	Simulation(std::string physicsSource);
	~Simulation();

	//void ShowMainUi();
	void update();

	void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
	void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
	void scroll_callback(GLFWwindow* window, double xpos, double ypos);
	void window_resize_callback(GLFWwindow* window, int x, int y);

	Physics physics;
	Graphics graphics;
	ImguiStatus imguiStatus;


private:
	double cursorPrevX;
	double cursorPrevY;
	bool leftMousePressed = false;
	bool rightMousePressed = false;
};
#endif