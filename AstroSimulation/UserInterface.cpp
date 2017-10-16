#include "UserInterface.h"

//this is some crazy C++ magic to deal with the fact that glfwSet{...}Callback cannot take non-static class methods
//This was a problem because I want to access and change member variables from within the callback
//http://stackoverflow.com/questions/7676971/pointing-to-a-function-that-is-a-class-member-glfw-setkeycallback
void UserInterface::initSimulationControls(Simulation* simulation)
{
	glfwSetWindowUserPointer(simulation->graphics.window, simulation);

	auto mouse_button_callback = [](GLFWwindow* window, int button, int action, int mods)
	{
		static_cast<Simulation*>(glfwGetWindowUserPointer(window))->mouse_button_callback(window, button, action, mods);
	};

	glfwSetMouseButtonCallback(simulation->graphics.window, mouse_button_callback);

	auto cursor_position_callback = [](GLFWwindow* window, double xpos, double ypos)
	{
		static_cast<Simulation*>(glfwGetWindowUserPointer(window))->cursor_position_callback(window, xpos, ypos);
	};

	glfwSetCursorPosCallback(simulation->graphics.window, cursor_position_callback);

	auto scroll_callback = [](GLFWwindow* window, double xoffset, double yoffset)
	{
		static_cast<Simulation*>(glfwGetWindowUserPointer(window))->scroll_callback(window, xoffset, yoffset);
	};

	glfwSetScrollCallback(simulation->graphics.window, scroll_callback);

	auto window_resize_callback = [](GLFWwindow* window, int x, int y)
	{
		static_cast<Simulation*>(glfwGetWindowUserPointer(window))->window_resize_callback(window, x, y);
	};

	glfwSetWindowSizeCallback(simulation->graphics.window, window_resize_callback);

	glfwSetKeyCallback(simulation->graphics.window, ImGui_ImplGlfwGL3_KeyCallback);
	glfwSetCharCallback(simulation->graphics.window, ImGui_ImplGlfwGL3_CharCallback);
}