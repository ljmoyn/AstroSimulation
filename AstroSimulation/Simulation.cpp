#include "Simulation.h"

Simulation::Simulation(std::string physicsSource)
{
	userInterface.InitUserInterface(graphics.window);
	graphics.LoadTextures(userInterface.textureFolders);

	// get physics data
	Physics::FromXml(&physics, physicsSource, userInterface.textureFolders);
	userInterface.InitObjectDataWindows(physics.getCurrentObjects());

	for (int i = 0; i < physics.getCurrentObjects().size(); i++)
		physics.paths.push_back({});

	//glfw requires static functions for callbacks. Store this object in the UserPointer,
	//set static wrapper function as callbacks, getting the actual callbacks from the UserPointer so I can access all the data in the callbacks
	glfwSetWindowUserPointer(graphics.window, this);

	glfwSetMouseButtonCallback(graphics.window, MouseButtonWrapper);
	glfwSetCursorPosCallback(graphics.window, CursorPositionWrapper);
	glfwSetScrollCallback(graphics.window, ScrollWrapper);
	glfwSetWindowSizeCallback(graphics.window, WindowResizeWrapper);

	glfwSetKeyCallback(graphics.window, ImGui_ImplGlfwGL3_KeyCallback);
	glfwSetCharCallback(graphics.window, ImGui_ImplGlfwGL3_CharCallback);
}

Simulation::~Simulation()
{
	// Terminate GLFW, clearing any resources allocated by GLFW.
	glfwTerminate();
}

void Simulation::update()
{
	// Check if any events have been activiated (key pressed, mouse moved etc.) and call corresponding response functions
	glfwPollEvents();

	ImGui_ImplGlfwGL3_NewFrame();

	userInterface.ShowMainUi(&physics, &graphics);

	if (!userInterface.isPaused && physics.dataIndex + physics.playbackSpeed > (int)physics.computedData.size() - 1) {
		physics.dataIndex = 0;
	}
	else if (!userInterface.isPaused && physics.dataIndex + physics.playbackSpeed < 0) {
		physics.dataIndex = (int)physics.computedData.size() - 1;
	}
	else if (!userInterface.isPaused) {
		physics.dataIndex += physics.playbackSpeed;
	}

	physics.time = physics.timestep.GetBaseValue() * physics.dataIndex;

	// Clear the colorbuffer
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	graphics.projection = glm::mat4();

	graphics.projection = glm::infinitePerspective(
		(float)glm::radians(graphics.camera.Zoom), // view angle, usually 90° in radians
		(float)graphics.width / graphics.height, // aspect ratio
		0.1f // near clipping plane, should be > 0
	);

	if (graphics.followObject != "") {
		PhysObject object = physics.GetObjectByName(graphics.followObject);
		graphics.xTranslate = -object.position.GetBaseValue(0);
	    graphics.yTranslate = -object.position.GetBaseValue(1);
	}

	graphics.setView();

	graphics.drawSpheres(&physics);
	graphics.drawPoints(&physics);
	graphics.drawLines(&physics);
	graphics.drawDebugLine();

	ImGui::Render();

	// Swap the screen buffers
	glfwSwapBuffers(graphics.window);
}

void Simulation::MouseButtonWrapper(GLFWwindow* window, int button, int action, int mods)
{
	void *data = glfwGetWindowUserPointer(window);
	Simulation *simulation = static_cast<Simulation *>(data);

	simulation->MouseButtonCallback(window, button, action, mods);
}

void Simulation::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	ImGuiIO& io = ImGui::GetIO();
	if (io.WantCaptureMouse) {
		ImGui_ImplGlfwGL3_MouseButtonCallback(window, button, action, mods);
		return;
	}


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

void Simulation::CursorPositionWrapper(GLFWwindow * window, double xpos, double ypos)
{
	void *data = glfwGetWindowUserPointer(window);
	Simulation *simulation = static_cast<Simulation *>(data);

	simulation->CursorPositionCallback(window, xpos,ypos);
}

void Simulation::CursorPositionCallback(GLFWwindow* window, double xpos, double ypos)
{
	if (leftMousePressed) {
		GLfloat xoffset = xpos - cursorPrevX;
		GLfloat yoffset = cursorPrevY - ypos;

		float fovY = tan(glm::radians(graphics.camera.Zoom / 2)) * 2 * -graphics.zTranslate;
		float fovX = fovY * ((float)graphics.width / (float)graphics.height);

		graphics.xTranslate += xoffset * fovX / graphics.width;
		graphics.yTranslate += yoffset * fovY / graphics.height;
	}

	if (rightMousePressed) {
		GLfloat xoffset = (xpos - cursorPrevX) / 4.0;
		GLfloat yoffset = (ypos - cursorPrevY) / 4.0;

		graphics.camera.inclination = glm::clamp(graphics.camera.inclination - yoffset, 0.f, 90.f);
		graphics.camera.azimuth = fmodf(graphics.camera.azimuth + xoffset, 360.f);
	}

	cursorPrevX = xpos;
	cursorPrevY = ypos;
}

void Simulation::ScrollWrapper(GLFWwindow * window, double xpos, double ypos)
{
	void *data = glfwGetWindowUserPointer(window);
	Simulation *simulation = static_cast<Simulation *>(data);

	simulation->ScrollCallback(window, xpos, ypos);

}

void Simulation::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	ImGuiIO& io = ImGui::GetIO();
	if (io.WantCaptureMouse) {
		ImGui_ImplGlfwGL3_ScrollCallback(window, xoffset, yoffset);
		return;
	}

	glm::mat4 modelview = graphics.view*graphics.model;
	glm::vec4 viewport = { 0.0, 0.0, graphics.width, graphics.height };

	float winX = cursorPrevX;
	float winY = viewport[3] - cursorPrevY;
	float winZ;

	glReadPixels(winX, winY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ);
	glm::vec3 screenCoords = { winX, winY, winZ };

	glm::vec3 cursorPosition = glm::unProject(screenCoords, modelview, graphics.projection, viewport);

	if (isinf(cursorPosition[2]) || isnan(cursorPosition[2])) {
		cursorPosition[2] = 0.0;
	}

	cursorPosition = { 597.84, 438.7, -15.2 };

	// zooming out 
	float zoomDirection = 1.0f;
	float deltaZ = .05;
	// zooming in 
	if (yoffset > 0.0)
		zoomDirection = -1.0f;
	

	//std::cout << cursorPosition[0] << " " << cursorPosition[1] << " " << cursorPosition[2] << " " << std::endl;

	//the width and height of the perspective view, at the depth of the cursor position 
	glm::vec2 fovXY = graphics.camera.getFovXY(cursorPosition[2] - graphics.zTranslate, (float)graphics.width / graphics.height);

	float prevZTranslate = graphics.zTranslate;

	float x = cursorPosition[0] - graphics.xTranslate;
	float y = cursorPosition[1] - graphics.yTranslate;
	float z = cursorPosition[2] - graphics.zTranslate;
	deltaZ = .1;// sqrtf(x*x + y*y + z*z);
	if (zoomDirection < 0) 
	{
		graphics.debugPoints.push_back(graphics.xTranslate);
		graphics.debugPoints.push_back(graphics.yTranslate);
		graphics.debugPoints.push_back(graphics.zTranslate);
	}
    //translate so that position under the cursor does not appear to move.
	graphics.xTranslate -= zoomDirection * deltaZ * x;
	if (zoomDirection < 0) 
	{
		graphics.debugPoints.push_back(graphics.xTranslate);
		graphics.debugPoints.push_back(graphics.yTranslate);
		graphics.debugPoints.push_back(graphics.zTranslate);
	}

	graphics.yTranslate -= zoomDirection * deltaZ * y;
	if (zoomDirection < 0) 
	{
		graphics.debugPoints.push_back(graphics.xTranslate);
		graphics.debugPoints.push_back(graphics.yTranslate);
		graphics.debugPoints.push_back(graphics.zTranslate);
	}
	graphics.zTranslate -= zoomDirection * deltaZ * z;

	if (zoomDirection < 0) 
	{
		graphics.debugPoints.push_back(graphics.xTranslate);
		graphics.debugPoints.push_back(graphics.yTranslate);
		graphics.debugPoints.push_back(graphics.zTranslate);
	}


	std::cout << graphics.xTranslate << " " << graphics.yTranslate << " " << graphics.zTranslate << " " << std::endl;

}

float Simulation::GetScrollAdjustment(float a1, float b1, float deltaZ)
{
	float c = deltaZ * tan(glm::radians(graphics.camera.Zoom / 2.0f));
	float a2 = a1 - c;
	float b2 = b1 - c;

	float r2 = a2 / (a2 + b2);
	float d = (a1 - r2) * (a2 + b2);

	return d;
}


//void Simulation::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
//{
//	ImGuiIO& io = ImGui::GetIO();
//	if (io.WantCaptureMouse) {
//		ImGui_ImplGlfwGL3_ScrollCallback(window, xoffset, yoffset);
//		return;
//	}
//
//	glm::mat4 modelview = graphics.view*graphics.model;
//	glm::vec4 viewport = { 0.0, 0.0, graphics.width, graphics.height };
//
//	float winX = cursorPrevX;
//	float winY = viewport[3] - cursorPrevY;
//	float winZ;
//
//	glReadPixels(winX, winY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ);
//	glm::vec3 screenCoords = { winX, winY, winZ };
//
//	glm::vec3 cursorPosition = glm::unProject(screenCoords, modelview, graphics.projection, viewport);
//
//	if (isinf(cursorPosition[2]) || isnan(cursorPosition[2])) {
//		cursorPosition[2] = 0.0;
//	}
//
//	cursorPosition = { 597.84, 438.7, -15.2 };
//
//	// zooming out 
//	float zoomDirection = 1.0f;
//	float deltaZ = .1;
//	// zooming in 
//	if (yoffset > 0.0)
//		zoomDirection = -1.0f;
//
//
//	//std::cout << graphics.zTranslate << std::endl;
//
//	//the width and height of the perspective view, at the depth of the cursor position 
//	glm::vec2 fovXY = graphics.camera.getFovXY(cursorPosition[2] - graphics.zTranslate, (float)graphics.width / graphics.height);
//
//	float prevZTranslate = graphics.zTranslate;
//	graphics.zTranslate += zoomDirection * deltaZ * graphics.zTranslate;
//
//	//zooming in
//	if (zoomDirection < 0) {
//		deltaZ = (prevZTranslate - graphics.zTranslate) / graphics.zTranslate;
//	}
//
//	glm::vec2 newFovXY = graphics.camera.getFovXY(cursorPosition[2] - graphics.zTranslate, (float)graphics.width / graphics.height);
//
//
//	float a1x = 1 - winX / graphics.width;
//	float b1x = winX / graphics.width;
//	float dx = GetScrollAdjustment(a1x, b1x, deltaZ);
//
//
//	float a1y = 1 - winY / graphics.height;
//	float b1y = winY / graphics.height;
//	float dy = GetScrollAdjustment(a1y, b1y, deltaZ);
//
//
//	float c = deltaZ * tan(glm::radians(graphics.camera.Zoom / 2.0f));
//	float a2 = a1x - c;
//	float b2 = b1x - c;
//
//	float r2 = a2 / (a2 + b2);
//	float d = (a1x - r2) * (a2 + b2);
//	std::cout << a1x << " " << r2 + d << std::endl;
//
//	//translate so that position under the cursor does not appear to move.
//	graphics.xTranslate += zoomDirection * newFovXY[0] * dx;
//	graphics.yTranslate += zoomDirection * newFovXY[1] * dy;
//}

//void Simulation::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
//{
//	ImGuiIO& io = ImGui::GetIO();
//	if (io.WantCaptureMouse) {
//		ImGui_ImplGlfwGL3_ScrollCallback(window, xoffset, yoffset);
//		return;
//	}
//
//	glm::mat4 modelview = graphics.view*graphics.model;
//	glm::vec4 viewport = { 0.0, 0.0, graphics.width, graphics.height };
//
//	float winX = cursorPrevX;
//	float winY = viewport[3] - cursorPrevY;
//	float winZ;
//
//	glReadPixels(winX, winY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ);
//	glm::vec3 screenCoords = { winX, winY, winZ };
//
//	glm::vec3 cursorPosition = glm::unProject(screenCoords, modelview, graphics.projection, viewport);
//
//	if (isinf(cursorPosition[2]) || isnan(cursorPosition[2])) {
//		cursorPosition[2] = 0.0;
//	}
//
//	cursorPosition = { 597.84, 438.7, -15.2 };
//
//	// zooming out 
//	float zoomDirection = 1.0f;
//	float deltaZ = .1;
//	// zooming in 
//	if (yoffset > 0.0)
//		zoomDirection = -1.0f;
//
//
//	//std::cout << graphics.zTranslate << std::endl;
//
//	//the width and height of the perspective view, at the depth of the cursor position 
//	glm::vec2 fovXY = graphics.camera.getFovXY(cursorPosition[2] - graphics.zTranslate, (float)graphics.width / graphics.height);
//
//	float prevZTranslate = graphics.zTranslate;
//	graphics.zTranslate += zoomDirection * deltaZ * graphics.zTranslate;
//
//	//zooming in
//	if (zoomDirection < 0) {
//		deltaZ = (prevZTranslate - graphics.zTranslate) / graphics.zTranslate;
//	}
//
//	glm::vec2 newFovXY = graphics.camera.getFovXY(cursorPosition[2] - graphics.zTranslate, (float)graphics.width / graphics.height);
//
//	float tangent = tan(glm::radians(graphics.camera.Zoom / 2.0f));
//
//	float upperSegment = 1 - winX / graphics.width;
//	float lowerSegment = winX / graphics.width;
//
//	//float xAdjustment = -2.0f * tangent * deltaZ * upperSegment + tangent * deltaZ - upperSegment;
//	float xAdjustment = upperSegment * (upperSegment + lowerSegment - 2.0f * tan(glm::radians(graphics.camera.Zoom / 2.0f)) * deltaZ) / (upperSegment + lowerSegment) + tan(glm::radians(graphics.camera.Zoom / 2.0f)) * deltaZ - upperSegment;
//
//	upperSegment = 1 - winY / graphics.height;
//	lowerSegment = winY / graphics.height;
//
//	float yAdjustment = upperSegment * (upperSegment + lowerSegment - 2.0f * tan(glm::radians(graphics.camera.Zoom / 2.0f)) * deltaZ) / (upperSegment + lowerSegment) + tan(glm::radians(graphics.camera.Zoom / 2.0f)) * deltaZ - upperSegment;
//	//float yAdjustment = -2.0f * tangent * deltaZ * upperSegment + tangent * deltaZ - upperSegment;
//
//	//translate so that position under the cursor does not appear to move.
//	graphics.xTranslate += zoomDirection * newFovXY[0] * xAdjustment;
//	graphics.yTranslate += zoomDirection * newFovXY[1] * yAdjustment;
//}

void Simulation::WindowResizeWrapper(GLFWwindow * window, int x, int y)
{
	void *data = glfwGetWindowUserPointer(window);
	Simulation *simulation = static_cast<Simulation *>(data);

	simulation->WindowResizeCallback(window, x,y);
}

void Simulation::WindowResizeCallback(GLFWwindow* window, int x, int y)
{
	graphics.width = x;
	graphics.height = y;
	glViewport(0, 0, graphics.width, graphics.height);
}
