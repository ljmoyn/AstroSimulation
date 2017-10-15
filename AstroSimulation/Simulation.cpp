#include "Simulation.h"

Simulation::Simulation(std::string physicsSource)
{
	// Initialize GLFW
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

	// Create a GLFWwindow and setup viewport
	const GLFWvidmode * mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	width = (mode->width) * 2 / 3;
	height = (mode->height) * 2 / 3;
	window = glfwCreateWindow(width, height, "Astro Simulation", nullptr, nullptr);
	glfwMakeContextCurrent(window);
	glViewport(0, 0, width, height);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	// Set this to true so GLEW knows to use a modern approach to retrieving function pointers and extensions
	glewExperimental = GL_TRUE;
	// Initialize GLEW
	glewInit();

	// Initialize shaders. Must call after other GL initializations
	Shader::InitShader(&pointsShaderProgram, "PointVertex.shader", "PointFragment.shader");
	Shader::InitShader(&objectsShaderProgram, "ObjectVertex.shader", "ObjectFragment.shader");
	Shader::InitShader(&pathsShaderProgram, "PathVertex.shader", "PathFragment.shader");

	// Initialize ImGui
	ImGui_ImplGlfwGL3_Init(window, false);
	imguiStatus.saveFiles = imguiStatus.GetAllFilesInFolder("../saves");
	imguiStatus.selected.resize(imguiStatus.saveFiles.size());
	std::fill(imguiStatus.selected.begin(), imguiStatus.selected.end(), false);
	if (!imguiStatus.selected.empty())
		imguiStatus.selected[0] = true;

	// barely used. Should probably refactor
	camera = Camera();
	//GLint flags; glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
	//if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
	//{
	//	glEnable(GL_DEBUG_OUTPUT);
	//	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

	//	glDebugMessageCallback(glDebugOutput, nullptr);
	//	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
	//}
	imguiStatus.textureFolders = imguiStatus.GetAllFoldersInFolder("../cubemaps/*");
	LoadTextures();

	// get physics data
	Physics::FromXml(&physics, physicsSource, imguiStatus.textureFolders);

	for (int i = 0; i < physics.getCurrentObjects().size(); i++)
		paths.push_back({});

	xTranslate = 0.0;
	yTranslate = 0.0;
	zTranslate = -1000000.0;
	setView();
	model = glm::mat4();
}

Simulation::~Simulation()
{
	// Terminate GLFW, clearing any resources allocated by GLFW.
	glfwTerminate();
}

void Simulation::GetWindowCoordinates(glm::vec4 point, float * xWindow, float *yWindow)
{
	glm::vec4 clip = projection * view * model * point;
	float xNDC = clip[0] / clip[3];
	float yNDC = clip[1] / clip[3];
	float zNDC = clip[2] / clip[3];

	*xWindow = .5f * (float)width * xNDC + (0.0 + width / 2.0f);
	*yWindow = .5f * (float)width * yNDC + (0.0 + height / 2.0f);
}

//http://www.songho.ca/opengl/gl_transform.html
float Simulation::GetPixelDiameter(glm::vec4 surfacePoint, glm::vec4 centerPoint)
{
	float dx = surfacePoint.x - centerPoint.x;
	float dy = surfacePoint.y - centerPoint.y;
	float dz = surfacePoint.z - centerPoint.z;

	float radius = std::powf(dx*dx + dy*dy + dz*dz, .5);
	glm::vec3 scaledRight = radius * glm::normalize(camera.Right);

	float xWindowSurface, yWindowSurface, xWindowCenter, yWindowCenter;
	GetWindowCoordinates(centerPoint + glm::vec4(scaledRight, 0), &xWindowSurface, &yWindowSurface);
	GetWindowCoordinates(centerPoint, &xWindowCenter, &yWindowCenter);

	dx = xWindowSurface - xWindowCenter;
	dy = yWindowSurface - yWindowCenter;
	return 2.0f * std::powf(dx*dx + dy*dy, .5);
}

void Simulation::GetVertexAttributeData(bool drawAsPoint, std::vector<GLfloat>* positions, std::vector<GLfloat>* colors, std::vector<GLint> * textureIndices, std::vector<glm::mat4> * instanceModels, int * count)
{
	std::vector<PhysObject> objects = physics.getCurrentObjects();
	std::vector<float> offsets = physics.GetFocusOffsets(objects);

	for (int i = 0; i < objects.size(); i++) {
		bool drawAsSphere = false;
		glm::vec3 position = {
			objects[i].position.GetBaseValue(0) - offsets[0],
			objects[i].position.GetBaseValue(1) - offsets[1],
			objects[i].position.GetBaseValue(2) - offsets[2]
		};

		float r = objects[i].radius.GetBaseValue();

		glm::mat4 instanceModel = glm::mat4();
		instanceModel = glm::scale(instanceModel, glm::vec3(r, r, r));

		glm::vec4 centerPoint = instanceModel * glm::vec4(position, 1.0);
		glm::vec4 surfacePoint = instanceModel * glm::vec4(position + glm::vec3(sphere.vertices[0], sphere.vertices[1], sphere.vertices[2]), 1.0);
		float diameter = GetPixelDiameter(surfacePoint, centerPoint);

		if ((drawAsPoint && diameter < 3) || (!drawAsPoint && diameter >= 3)) {
			positions->push_back(position[0]);
			positions->push_back(position[1]);
			positions->push_back(position[2]);
			colors->push_back(physics.objectSettings[i].color[0]);
			colors->push_back(physics.objectSettings[i].color[1]);
			colors->push_back(physics.objectSettings[i].color[2]);

			if (!drawAsPoint) {
				textureIndices->push_back(drawAsPoint ? -1 : physics.objectSettings[i].textureIndex);

				instanceModel = glm::rotate(instanceModel, glm::radians(270.0f), glm::vec3(1.f, 0.f, 0.f));
				instanceModels->push_back(instanceModel);
			}
			
			(*count)++;
		}
	}
}

void Simulation::drawPoints() {

	std::vector<GLfloat> positions = {};
	std::vector<GLfloat> colors = {};

	int numPoints = 0;
	GetVertexAttributeData(true, &positions, &colors, nullptr, nullptr, &numPoints);
	if (numPoints == 0)
		return;

	glUseProgram(pointsShaderProgram);

	// Get their uniform location
	GLint viewLoc = glGetUniformLocation(pointsShaderProgram, "view");
	GLint projLoc = glGetUniformLocation(pointsShaderProgram, "projection");

	// Pass them to the shaders
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	// Position attribute
	glGenBuffers(1, &positionVBO);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, positionVBO);
	glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(GLfloat), &positions[0], GL_STREAM_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	// Color attribute
	glGenBuffers(1, &colorVBO);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, colorVBO);
	glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(GLfloat), &colors[0], GL_STREAM_DRAW);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);

	glPointSize(3.0);
	glDrawArrays(GL_POINTS, 0, 3*physics.getCurrentObjects().size());
	glBindVertexArray(0);

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &positionVBO);
	glDeleteBuffers(1, &colorVBO);
}

void Simulation::drawSpheres() {

	glUseProgram(objectsShaderProgram);

	// Get their uniform location
	GLint viewLoc = glGetUniformLocation(objectsShaderProgram, "view");
	GLint projLoc = glGetUniformLocation(objectsShaderProgram, "projection");

	// Pass them to the shaders
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	std::vector<GLfloat> positions = {};
	std::vector<GLfloat> colors = {};
	std::vector<GLint> textureIndices = {};
	std::vector<glm::mat4> instanceModels = {};
	int numSpheres = 0;
	GetVertexAttributeData(false, &positions, &colors, &textureIndices, &instanceModels, &numSpheres);

	if (numSpheres == 0)
		return;

	// Bind the Vertex Array Object first, then bind and set vertex buffer(s) and attribute pointer(s).
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	// Vertex Attribute
	glGenBuffers(1, &sphereVBO);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
	glBufferData(GL_ARRAY_BUFFER, sphere.vertices.size() * sizeof(GLfloat), &(sphere.vertices[0]), GL_STREAM_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);

	// Color attribute
	glGenBuffers(1, &colorVBO);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, colorVBO);
	glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(GLfloat), &colors[0], GL_STREAM_DRAW);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glVertexAttribDivisor(1, 1);

	// Position attribute
	glGenBuffers(1, &positionVBO);
	glEnableVertexAttribArray(2);
	glBindBuffer(GL_ARRAY_BUFFER, positionVBO);
	glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(GLfloat), &positions[0], GL_STREAM_DRAW);

	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glVertexAttribDivisor(2, 1);

	// texture index attribute
	glGenBuffers(1, &textureIndexVBO);
	glEnableVertexAttribArray(3);
	glBindBuffer(GL_ARRAY_BUFFER, textureIndexVBO);
	glBufferData(GL_ARRAY_BUFFER, textureIndices.size() * sizeof(GLfloat), &textureIndices[0], GL_STREAM_DRAW);

	glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 1 * sizeof(GLfloat), (GLvoid*)0);
	glVertexAttribDivisor(3, 1);

	// orientation attribute

	glGenBuffers(1, &instanceModelsVBO);
	glBindBuffer(GL_ARRAY_BUFFER, instanceModelsVBO);
	glBufferData(GL_ARRAY_BUFFER, instanceModels.size() * sizeof(glm::mat4), &instanceModels[0], GL_STREAM_DRAW);

	//the largest single type we can pass to the shader is a vec4, so pass four of them to get a mat4
	GLsizei vec4Size = sizeof(glm::vec4);
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(glm::vec4), (void*)0);
	glVertexAttribDivisor(4, 1);

	glEnableVertexAttribArray(5);
	glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(glm::vec4), (void*)(sizeof(glm::vec4)));
	glVertexAttribDivisor(5, 1);

	glEnableVertexAttribArray(6);
	glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(glm::vec4), (void*)(2 * sizeof(glm::vec4)));
	glVertexAttribDivisor(6, 1);

	glEnableVertexAttribArray(7);
	glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(glm::vec4), (void*)(3 * sizeof(glm::vec4)));
	glVertexAttribDivisor(7, 1);

	// textures
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, cubemap);
	glUniform1i(glGetUniformLocation(objectsShaderProgram, "cubemap"), 0);

	// element buffer 
	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphere.indices.size() * sizeof(GLfloat), &sphere.indices[0], GL_STREAM_DRAW);

	// draw the elements
	glDrawElementsInstanced(GL_TRIANGLES, sphere.indices.size(), GL_UNSIGNED_INT, 0, numSpheres);

	// unbind VAO
	glBindVertexArray(0);

	// cleanup buffers
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &sphereVBO);
	glDeleteBuffers(1, &colorVBO);
	glDeleteBuffers(1, &positionVBO);
	glDeleteBuffers(1, &textureCoordinateVBO);
	glDeleteBuffers(1, &textureIndexVBO);
	glDeleteBuffers(1, &instanceModelsVBO);

	glDeleteBuffers(1, &EBO);
}

void Simulation::updatePaths(Physics * physics, std::vector<std::vector<GLfloat> > * paths, bool firstFrame) {
	std::vector<PhysObject> currentObjects = physics->getCurrentObjects();
	if (firstFrame) {
		*paths = {};
		for (int i = 0; i < physics->getCurrentObjects().size(); i++) {
			std::vector<float> offsets = physics->GetFocusOffsets(currentObjects);

			paths->push_back({
				currentObjects[i].position.GetBaseValue(0) - offsets[0],
				currentObjects[i].position.GetBaseValue(1) - offsets[1],
				currentObjects[i].position.GetBaseValue(2) - offsets[2],
			});
		}

	}
	else {
		for (int i = 0; i < currentObjects.size(); i++) {
			std::vector<float> offsets = physics->GetFocusOffsets(currentObjects);

			(*paths)[i].push_back(currentObjects[i].position.GetBaseValue(0) - offsets[0]);
			(*paths)[i].push_back(currentObjects[i].position.GetBaseValue(1) - offsets[1]);
			(*paths)[i].push_back(currentObjects[i].position.GetBaseValue(2) - offsets[2]);
		}
	}
}

void Simulation::drawLines() {
	if (physics.computedData.size() < 2)
		return;

	glUseProgram(pathsShaderProgram);
	// Get uniform location
	GLint modelLoc = glGetUniformLocation(pathsShaderProgram, "model");
	GLint viewLoc = glGetUniformLocation(pathsShaderProgram, "view");
	GLint projLoc = glGetUniformLocation(pathsShaderProgram, "projection");

	// Pass them to the shaders
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	std::vector<PhysObject> objects = physics.getCurrentObjects();
	for (int i = 0; i < objects.size(); i++) {

		if (paths[i].empty())
			continue;

		glGenVertexArrays(1, &VAO);
		glBindVertexArray(VAO);

		//Vertex attribute
		glGenBuffers(1, &vertexVBO);
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexVBO);
		glBufferData(GL_ARRAY_BUFFER, paths[i].size() * sizeof(GLfloat), &paths[i][0], GL_STREAM_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);

		// Color attribute
		glGenBuffers(1, &colorVBO);
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, colorVBO);
		glBufferData(GL_ARRAY_BUFFER, physics.objectSettings[i].color.size() * sizeof(GLfloat), &physics.objectSettings[i].color[0], GL_STREAM_DRAW);

		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
		glVertexAttribDivisor(1, 1);

		glPointSize(1.0);

		//+1 so that the line connects with the point
		//single instance, so I can pass just one copy of the color, and have it apply to evry vertex
		glDrawArraysInstanced(GL_LINE_STRIP, 0, physics.dataIndex + 1,1);
		glBindVertexArray(0);

		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &vertexVBO);
		glDeleteBuffers(1, &colorVBO);

	}
}

void Simulation::update()
{
	// Check if any events have been activiated (key pressed, mouse moved etc.) and call corresponding response functions
	glfwPollEvents();

	ImGui_ImplGlfwGL3_NewFrame();

	ShowMainUi(&physics, &paths, &camera, &imguiStatus, &xTranslate, &yTranslate, &zTranslate);

	if (!imguiStatus.isPaused && physics.dataIndex + physics.playbackSpeed > (int)physics.computedData.size() - 1) {
		physics.dataIndex = 0;
	}
	else if (!imguiStatus.isPaused && physics.dataIndex + physics.playbackSpeed < 0) {
		physics.dataIndex = (int)physics.computedData.size() - 1;
	}
	else if (!imguiStatus.isPaused) {
		physics.dataIndex += physics.playbackSpeed;
	}

	physics.time = physics.timestep.GetBaseValue() * physics.dataIndex;

	// Clear the colorbuffer
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	projection = glm::mat4();

	projection = glm::infinitePerspective(
		(float)glm::radians(camera.Zoom), // view angle, usually 90° in radians
		(float)width / height, // aspect ratio
		0.1f // near clipping plane, should be > 0
	);

	setView();

	drawSpheres();
	drawPoints();
	drawLines();

	ImGui::Render();

	// Swap the screen buffers
	glfwSwapBuffers(window);
}

void Simulation::setView() {
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
void Simulation::initSimulationControls(Simulation* simulation)
{
	glfwSetWindowUserPointer(simulation->window, simulation);

	auto mouse_button_callback = [](GLFWwindow* window, int button, int action, int mods)
	{
		static_cast<Simulation*>(glfwGetWindowUserPointer(window))->mouse_button_callback(window, button, action, mods);
	};

	glfwSetMouseButtonCallback(simulation->window, mouse_button_callback);

	auto cursor_position_callback = [](GLFWwindow* window, double xpos, double ypos)
	{
		static_cast<Simulation*>(glfwGetWindowUserPointer(window))->cursor_position_callback(window, xpos, ypos);
	};

	glfwSetCursorPosCallback(simulation->window, cursor_position_callback);

	auto scroll_callback = [](GLFWwindow* window, double xoffset, double yoffset)
	{
		static_cast<Simulation*>(glfwGetWindowUserPointer(window))->scroll_callback(window, xoffset, yoffset);
	};

	glfwSetScrollCallback(simulation->window, scroll_callback);

	auto window_resize_callback = [](GLFWwindow* window, int x, int y)
	{
		static_cast<Simulation*>(glfwGetWindowUserPointer(window))->window_resize_callback(window, x, y);
	};

	glfwSetWindowSizeCallback(simulation->window, window_resize_callback);

	glfwSetKeyCallback(simulation->window, ImGui_ImplGlfwGL3_KeyCallback);
	glfwSetCharCallback(simulation->window, ImGui_ImplGlfwGL3_CharCallback);

}

void Simulation::mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
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

void Simulation::cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
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

void Simulation::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
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

void Simulation::window_resize_callback(GLFWwindow* _window, int x, int y)
{
	width = x;
	height = y;
	glViewport(0, 0, width, height);
}

void Simulation::LoadTextures()
{
	std::vector<std::string> faces;

	for (int i = 0; i < imguiStatus.textureFolders.size(); i++) {
		std::string folder = imguiStatus.textureFolders[i];

		//source: http://planetpixelemporium.com/earth.html
		//converted from equirectangular to cubemap using this (blender) 
		//https://developers.theta360.com/en/forums/viewtopic.php?f=4&t=1981
		faces.push_back("../cubemaps/" + folder + "/right_PNG_DXT1_1.dds");
		faces.push_back("../cubemaps/" + folder + "/left_PNG_DXT1_1.dds");
		faces.push_back("../cubemaps/" + folder + "/top_PNG_DXT1_1.dds");
		faces.push_back("../cubemaps/" + folder + "/bottom_PNG_DXT1_1.dds");
		faces.push_back("../cubemaps/" + folder + "/back_PNG_DXT1_1.dds");
		faces.push_back("../cubemaps/" + folder + "/front_PNG_DXT1_1.dds");
	}

	glGenTextures(1, &cubemap);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, cubemap);

	int width, height, mipmapCount, format;
	unsigned char * image;
	for (GLuint i = 0; i < faces.size(); i++)
	{
		image = LoadDDS(faces[i], &format, &mipmapCount, &width, &height);

		unsigned int blockSize = (format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) ? 8 : 16;
		unsigned int offset = 0;

		/* load the mipmaps */
		for (unsigned int level = 0; level < mipmapCount + 1 && (width || height); level++)
		{
			int size = ((width + 3) / 4)*((height + 3) / 4)*blockSize;

			//allocate memory for each level
			if (i == 0) {
				glCompressedTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY,
					level,                       // level
					format,                      // Internal format
					width, height, faces.size(), // width,height,depth
					0,						     // border
					faces.size() * size,         // imageSize
					0);                          // pointer to data
			}

			glCompressedTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, level, 0, 0, i, width, height, 1, format, size, image + offset);

			offset += size;
			width /= 2;
			height /= 2;
		}

		free(image);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}


//http://www.opengl-tutorial.org/beginners-tutorials/tutorial-5-a-textured-cube/
#define FOURCC_DXT1 0x31545844 // Equivalent to "DXT1" in ASCII
#define FOURCC_DXT3 0x33545844 // Equivalent to "DXT3" in ASCII
#define FOURCC_DXT5 0x35545844 // Equivalent to "DXT5" in ASCII
unsigned char * Simulation::LoadDDS(std::string imagepath, int * format, int * mipmapCount, int * width, int * height)
{
	unsigned char header[124];

	FILE *fp;

	/* try to open the file */
	fopen_s(&fp, imagepath.c_str(), "rb");
	if (fp == NULL)
		return 0;

	/* verify the type of file */
	char filecode[4];
	fread(filecode, 1, 4, fp);
	if (strncmp(filecode, "DDS ", 4) != 0) {
		fclose(fp);
		return 0;
	}

	/* get the surface desc */
	fread(&header, 124, 1, fp);

	*height = *(unsigned int*)&(header[8]);
	*width = *(unsigned int*)&(header[12]);
	*mipmapCount = *(unsigned int*)&(header[24]);

	//linear size is the size of the base image, not including mipmaps
	//should be equivalent to ((width + 3) / 4)*((height + 3) / 4)*blockSize
	unsigned int linearSize = *(unsigned int*)&(header[16]);
	unsigned int fourCC = *(unsigned int*)&(header[80]);

	unsigned char * buffer;
	/* how big is it going to be including all mipmaps? */
	int imageSize = *mipmapCount > 1 ? linearSize * 2 : linearSize;
	buffer = (unsigned char*)malloc(imageSize * sizeof(unsigned char));
	fread(buffer, 1, imageSize, fp);
	/* close the file pointer */
	fclose(fp);

	switch (fourCC)
	{
	case FOURCC_DXT1:
		*format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
		break;
	case FOURCC_DXT3:
		*format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
		break;
	case FOURCC_DXT5:
		*format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		break;
	default:
		free(buffer);
		return 0;
	}

	return buffer;
}

void Simulation::CheckGlError(std::string message)
{
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR)
	{
		std::string error;
		switch (errorCode)
		{
		case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
		case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
		case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
		case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
		case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
		case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
		}
		std::cout << message << " : " << error << std::endl;
	}
}

//void APIENTRY Simulation::glDebugOutput(GLenum source ,GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
//{
//	// ignore non-significant error/warning codes
//	if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;
//
//	std::cout << "---------------" << std::endl;
//	std::cout << "Debug message (" << id << "): " << message << std::endl;
//
//	switch (source)
//	{
//	case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
//	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
//	case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
//	case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
//	case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
//	case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
//	} std::cout << std::endl;
//
//	switch (type)
//	{
//	case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
//	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
//	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
//	case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
//	case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
//	case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
//	case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
//	case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
//	case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
//	} std::cout << std::endl;
//
//	switch (severity)
//	{
//	case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
//	case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
//	case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
//	case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
//	} std::cout << std::endl;
//	std::cout << std::endl;
//}
