#include "Visual.h"

Visual::Visual(std::string simulationSource)
{
	Simulation::FromXml(&simulation, simulationSource);

	// Init GLFW
	glfwInit();
	// Set all the required options for GLFW
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	// Create a GLFWwindow object that we can use for GLFW's functions
	const GLFWvidmode * mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

	width = (mode->width) * 2 / 3;
	height = (mode->height) * 2 / 3;
	window = glfwCreateWindow(width, height, "Astro Simulation", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	// Set this to true so GLEW knows to use a modern approach to retrieving function pointers and extensions
	glewExperimental = GL_TRUE;
	// Initialize GLEW to setup the OpenGL Function pointers
	glewInit();

	// Setup ImGui binding
	ImGui_ImplGlfwGL3_Init(window, false);

	// Define the viewport dimensions
	glViewport(0, 0, width, height);

	// Setup OpenGL options
	glEnable(GL_DEPTH_TEST);
	camera = Camera();

	for (int i = 0; i < simulation.getCurrentObjects().size(); i++)
		lines.push_back({});

	xTranslate = 0.0;
	yTranslate = 0.0;
	zTranslate = -1000000.0;
	setView();
	model = glm::mat4();

	imguiStatus.saveFiles = imguiStatus.GetAllFilesInFolder("../saves");
	imguiStatus.selected.resize(imguiStatus.saveFiles.size());
	std::fill(imguiStatus.selected.begin(), imguiStatus.selected.end(), false);
	if (!imguiStatus.selected.empty())
		imguiStatus.selected[0] = true;
}

Visual::~Visual()
{
	// Terminate GLFW, clearing any resources allocated by GLFW.
	glfwTerminate();
}

//http://www.opengl.org.ru/docs/pg/0208.html
#define X .525731112119133606 
#define Z .850650808352039932

static GLfloat vdata[12][3] = {
	{ -X, 0.0, Z },{ X, 0.0, Z },{ -X, 0.0, -Z },{ X, 0.0, -Z },
	{ 0.0, Z, X },{ 0.0, Z, -X },{ 0.0, -Z, X },{ 0.0, -Z, -X },
	{ Z, X, 0.0 },{ -Z, X, 0.0 },{ Z, -X, 0.0 },{ -Z, -X, 0.0 }
};
static GLuint tindices[20][3] = {
	{ 0,4,1 },{ 0,9,4 },{ 9,5,4 },{ 4,5,8 },{ 4,8,1 },
	{ 8,10,1 },{ 8,3,10 },{ 5,3,8 },{ 5,2,3 },{ 2,7,3 },
	{ 7,10,3 },{ 7,6,10 },{ 7,11,6 },{ 11,0,6 },{ 0,1,6 },
	{ 6,1,10 },{ 9,0,11 },{ 9,11,2 },{ 9,2,5 },{ 7,2,11 } };

void Visual::drawVertices() {
	std::vector<SimulationObject> objects = simulation.getCurrentObjects();
	for (int i = 0; i < objects.size(); i++) {

		std::vector<float> offsets = simulation.GetFocusOffsets(objects);
		GLfloat vertices[72];
		int k = 0;
		for (int j = 0; j < 12; j++) {
			vertices[k] = 800.0f * (objects[i].position.GetBaseValue(0) - offsets[0] + vdata[j][0]);
			vertices[k + 1] = 800.0f * (objects[i].position.GetBaseValue(1) - offsets[1] + vdata[j][1]);
			vertices[k + 2] = 800.0f * (objects[i].position.GetBaseValue(2) - offsets[2] + vdata[j][2]);

			vertices[k + 3] = simulation.objectSettings[i].color[0];
			vertices[k + 4] = simulation.objectSettings[i].color[1];
			vertices[k + 5] = simulation.objectSettings[i].color[2];
			k += 6;
		}

		GLuint indices[60] = {
			0,4,1,
			0,9,4,
			9,5,4,
			4,5,8,
			4,8,1,
			8,10,1,
			8,3,10,
			5,3,8,
			5,2,3,
			2,7,3,
			7,10,3,
			7,6,10,
			7,11,6,
			11,0,6,
			0,1,6,
			6,1,10,
			9,0,11,
			9,11,2,
			9,2,5,
			7,2,11
		};

		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &EBO);
		// Bind the Vertex Array Object first, then bind and set vertex buffer(s) and attribute pointer(s).
		glBindVertexArray(VAO);

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STREAM_DRAW);

		// Position Attribute
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
		glEnableVertexAttribArray(0);

		// Color attribute
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
		glEnableVertexAttribArray(1);

		glBindBuffer(GL_ARRAY_BUFFER, 0); 
		glBindVertexArray(0); 

		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 60, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &VBO);
		glDeleteBuffers(1, &EBO);
	}
}

void Visual::updateLines(Simulation * simulation, std::vector<std::vector<GLfloat> > * lines, bool firstFrame) {
	std::vector<SimulationObject> currentObjects = simulation->getCurrentObjects();
	if (firstFrame) {
		*lines = {};
		for (int i = 0; i < simulation->getCurrentObjects().size(); i++) {
			std::vector<float> offsets = simulation->GetFocusOffsets(currentObjects);

			lines->push_back({
				currentObjects[i].position.GetBaseValue(0) - offsets[0],
				currentObjects[i].position.GetBaseValue(1) - offsets[1],
				currentObjects[i].position.GetBaseValue(2) - offsets[2],
				simulation->objectSettings[i].color[0],
				simulation->objectSettings[i].color[1],
				simulation->objectSettings[i].color[2]
			});
		}

	}
	else {
		for (int i = 0; i < currentObjects.size(); i++) {
			std::vector<float> offsets = simulation->GetFocusOffsets(currentObjects);

			(*lines)[i].push_back(currentObjects[i].position.GetBaseValue(0) - offsets[0]);
			(*lines)[i].push_back(currentObjects[i].position.GetBaseValue(1) - offsets[1]);
			(*lines)[i].push_back(currentObjects[i].position.GetBaseValue(2) - offsets[2]);

			(*lines)[i].push_back(simulation->objectSettings[i].color[0]);
			(*lines)[i].push_back(simulation->objectSettings[i].color[1]);
			(*lines)[i].push_back(simulation->objectSettings[i].color[2]);
		}
	}
}

void Visual::drawLines() {
	if (simulation.computedData.size() < 2)
		return;

	std::vector<SimulationObject> objects = simulation.getCurrentObjects();
	for (int i = 0; i < objects.size(); i++) {

		if (lines[i].empty())
			continue;

		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);

		glBindVertexArray(VAO);

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, lines[i].size() * sizeof(GLfloat), &lines[i][0], GL_STREAM_DRAW);

		// Position attribute
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
		glEnableVertexAttribArray(0);
		// Color attribute
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
		glEnableVertexAttribArray(1);


		glBindVertexArray(VAO);
		glPointSize(1.0);

		//+1 so that the line connects with the point
		glDrawArrays(GL_LINE_STRIP, 0, simulation.dataIndex + 1);
		glBindVertexArray(0);

		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &VBO);
	}
}

void Visual::update(Shader shader)
{
	// Check if any events have been activiated (key pressed, mouse moved etc.) and call corresponding response functions
	glfwPollEvents();

	ImGui_ImplGlfwGL3_NewFrame();

	ShowMainUi(&simulation, &lines, &camera, &imguiStatus, &xTranslate, &yTranslate, &zTranslate);

	if (!imguiStatus.isPaused && simulation.dataIndex + simulation.playbackSpeed > (int)simulation.computedData.size() - 1) {
		simulation.dataIndex = 0;
	}
	else if (!imguiStatus.isPaused && simulation.dataIndex + simulation.playbackSpeed < 0) {
		simulation.dataIndex = (int)simulation.computedData.size() - 1;
	}
	else if (!imguiStatus.isPaused) {
		simulation.dataIndex += simulation.playbackSpeed;
	}

	simulation.time = simulation.timestep.GetBaseValue() * simulation.dataIndex;

	// Clear the colorbuffer
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// todo: make shader a part of the class so I only need to initialize in constructor
	shader.Use();

	//set view dimensions
	//projection = glm::ortho(xRange[0], xRange[1], yRange[0], yRange[1], zRange[0], zRange[1]);
	projection = glm::mat4();

	projection = glm::infinitePerspective(
		(float)glm::radians(camera.Zoom), // view angle, usually 90° in radians
		(float)width / height, // aspect ratio
		0.1f // near clipping plane, should be > 0
	);

	setView();

	// Get their uniform location
	GLint modelLoc = glGetUniformLocation(shader.Program, "model");
	GLint viewLoc = glGetUniformLocation(shader.Program, "view");
	GLint projLoc = glGetUniformLocation(shader.Program, "projection");

	// Pass them to the shaders
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	drawVertices();
	drawLines();

	ImGui::Render();

	// Swap the screen buffers
	glfwSwapBuffers(window);
}

void Visual::setView() {
	view = glm::mat4();

	view = glm::translate(view, { xTranslate,yTranslate,zTranslate });

	//view = glm::translate(view, { -xTranslate,-yTranslate,0.0 });
	view = glm::rotate(view, glm::radians(camera.inclination), glm::vec3(1.f, 0.f, 0.f));
	view = glm::rotate(view, glm::radians(camera.azimuth), glm::vec3(0.f, 0.f, 1.f));
	//view = glm::translate(view, { xTranslate,yTranslate,0.0 });

	camera.Right = glm::column(view, 0).xyz();
	camera.Up = glm::column(view, 1).xyz();
	camera.Front = -glm::column(view, 2).xyz(); // minus because OpenGL camera looks towards negative Z.
	camera.Position = glm::column(view, 3).xyz();
}

//this is some crazy C++ magic to deal with the fact that glfwSet{...}Callback cannot take non-static class methods
//This was a problem because I want to access and change member variables from within the callback
//http://stackoverflow.com/questions/7676971/pointing-to-a-function-that-is-a-class-member-glfw-setkeycallback
void Visual::initVisualControls(Visual* visual)
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

	auto window_resize_callback = [](GLFWwindow* window, int x, int y)
	{
		static_cast<Visual*>(glfwGetWindowUserPointer(window))->window_resize_callback(window, x, y);
	};

	glfwSetWindowSizeCallback(visual->window, window_resize_callback);

	glfwSetKeyCallback(visual->window, ImGui_ImplGlfwGL3_KeyCallback);
	glfwSetCharCallback(visual->window, ImGui_ImplGlfwGL3_CharCallback);

}

void Visual::mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
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

void Visual::cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (leftMousePressed) {
		GLfloat xoffset = xpos - cursorPrevX;
		GLfloat yoffset = cursorPrevY - ypos;

		float fovY = tan(glm::radians(camera.Zoom / 2)) * 2 * -zTranslate;
		float fovX = fovY * ((float)width / (float)height);

		xTranslate += xoffset * fovX / width;
		yTranslate += yoffset * fovY / height;
	}

	if (rightMousePressed) {
		GLfloat xoffset = (xpos - cursorPrevX) / 4.0;
		GLfloat yoffset = (ypos - cursorPrevY) / 4.0;

		camera.inclination = glm::clamp(camera.inclination - yoffset, 0.f, 90.f);
		camera.azimuth = fmodf(camera.azimuth + xoffset, 360.f);
	}

	cursorPrevX = xpos;
	cursorPrevY = ypos;
}

void Visual::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	ImGuiIO& io = ImGui::GetIO();
	if (io.WantCaptureMouse) {
		ImGui_ImplGlfwGL3_ScrollCallback(window, xoffset, yoffset);
		return;
	}

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
	// zooming out 
	float zoomFactor = 1.1;

	// zooming in 
	if (yoffset > 0.0)
		zoomFactor = 1 / 1.1;

	//the width and height of the perspective view, at the depth of the cursor position 
	glm::vec2 fovXY = camera.getFovXY(cursorPosition[2] - zTranslate, (float)width / height);
	camera.setZoomFromFov(fovXY.y * zoomFactor, cursorPosition[2] - zTranslate);

	if (camera.Zoom > 45.0) {
		camera.Zoom = 45.0;
	}

	glm::vec2 newFovXY = camera.getFovXY(cursorPosition[2] - zTranslate, (float)width / height);

	//translate so that position under the cursor does not appear to move.
	xTranslate += (newFovXY.x - fovXY.x) * (winX / width - .5);
	yTranslate += (newFovXY.y - fovXY.y) * (winY / height - .5);
}

void Visual::window_resize_callback(GLFWwindow* _window, int x, int y)
{
	width = x;
	height = y;
	glViewport(0, 0, width, height);
}