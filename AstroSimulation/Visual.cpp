#include "Visual.h"

Visual::Visual(std::string simulationSource)
{
	// Initialize GLFW
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

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

	imguiStatus.textureFolders = imguiStatus.GetAllFoldersInFolder("../cubemaps/*");
	LoadTextures();

	// get simulation data
	Simulation::FromXml(&simulation, simulationSource, imguiStatus.textureFolders);

	for (int i = 0; i < simulation.getCurrentObjects().size(); i++)
		paths.push_back({});

	xTranslate = 0.0;
	yTranslate = 0.0;
	zTranslate = -1000000.0;
	setView();
	model = glm::mat4();
}

Visual::~Visual()
{
	// Terminate GLFW, clearing any resources allocated by GLFW.
	glfwTerminate();
}

void Visual::GetWindowCoordinates(glm::vec4 point, float * xWindow, float *yWindow)
{
	glm::vec4 clip = projection * view * model * point;
	float xNDC = clip[0] / clip[3];
	float yNDC = clip[1] / clip[3];
	float zNDC = clip[2] / clip[3];

	*xWindow = .5f * (float)width * xNDC + (0.0 + width / 2.0f);
	*yWindow = .5f * (float)width * yNDC + (0.0 + height / 2.0f);
}

//http://www.songho.ca/opengl/gl_transform.html
float Visual::GetPixelDiameter(glm::vec4 surfacePoint, glm::vec4 centerPoint)
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

void Visual::GetVertexAttributeData(bool drawAsPoint, std::vector<GLfloat>* positions, std::vector<GLfloat>* colors, std::vector<GLint> * textureIndices, std::vector<glm::mat4> * instanceModels, int * count)
{
	std::vector<SimulationObject> objects = simulation.getCurrentObjects();
	std::vector<float> offsets = simulation.GetFocusOffsets(objects);

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
			colors->push_back(simulation.objectSettings[i].color[0]);
			colors->push_back(simulation.objectSettings[i].color[1]);
			colors->push_back(simulation.objectSettings[i].color[2]);

			if (!drawAsPoint) {
				textureIndices->push_back(drawAsPoint ? -1 : simulation.objectSettings[i].textureIndex);

				instanceModel = glm::rotate(instanceModel, glm::radians(270.0f), glm::vec3(1.f, 0.f, 0.f));
				instanceModels->push_back(instanceModel);
			}
			
			(*count)++;
		}
	}
}

void Visual::drawPoints() {

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
	glDrawArrays(GL_POINTS, 0, 3*simulation.getCurrentObjects().size());
	glBindVertexArray(0);

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &positionVBO);
	glDeleteBuffers(1, &colorVBO);
}

void Visual::drawSpheres() {

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

void Visual::updatePaths(Simulation * simulation, std::vector<std::vector<GLfloat> > * paths, bool firstFrame) {
	std::vector<SimulationObject> currentObjects = simulation->getCurrentObjects();
	if (firstFrame) {
		*paths = {};
		for (int i = 0; i < simulation->getCurrentObjects().size(); i++) {
			std::vector<float> offsets = simulation->GetFocusOffsets(currentObjects);

			paths->push_back({
				currentObjects[i].position.GetBaseValue(0) - offsets[0],
				currentObjects[i].position.GetBaseValue(1) - offsets[1],
				currentObjects[i].position.GetBaseValue(2) - offsets[2],
			});
		}

	}
	else {
		for (int i = 0; i < currentObjects.size(); i++) {
			std::vector<float> offsets = simulation->GetFocusOffsets(currentObjects);

			(*paths)[i].push_back(currentObjects[i].position.GetBaseValue(0) - offsets[0]);
			(*paths)[i].push_back(currentObjects[i].position.GetBaseValue(1) - offsets[1]);
			(*paths)[i].push_back(currentObjects[i].position.GetBaseValue(2) - offsets[2]);
		}
	}
}

void Visual::drawLines() {
	if (simulation.computedData.size() < 2)
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

	std::vector<SimulationObject> objects = simulation.getCurrentObjects();
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
		glBufferData(GL_ARRAY_BUFFER, simulation.objectSettings[i].color.size() * sizeof(GLfloat), &simulation.objectSettings[i].color[0], GL_STREAM_DRAW);

		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
		glVertexAttribDivisor(1, 1);

		glPointSize(1.0);

		//+1 so that the line connects with the point
		//single instance, so I can pass just one copy of the color, and have it apply to evry vertex
		glDrawArraysInstanced(GL_LINE_STRIP, 0, simulation.dataIndex + 1,1);
		glBindVertexArray(0);

		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &vertexVBO);
		glDeleteBuffers(1, &colorVBO);

	}
}

void Visual::update()
{
	// Check if any events have been activiated (key pressed, mouse moved etc.) and call corresponding response functions
	glfwPollEvents();

	ImGui_ImplGlfwGL3_NewFrame();

	ShowMainUi(&simulation, &paths, &camera, &imguiStatus, &xTranslate, &yTranslate, &zTranslate);

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

void Visual::LoadTextures()
{
	std::vector<std::string> faces;

	for (int i = 0; i < imguiStatus.textureFolders.size(); i++) {
		std::string folder = imguiStatus.textureFolders[i];
		//source: http://planetpixelemporium.com/earth.html
		//converted from equirectangular to cubemap using this (blender) 
		//https://developers.theta360.com/en/forums/viewtopic.php?f=4&t=1981
		faces.push_back("../cubemaps/" + folder + "/right.png");
		faces.push_back("../cubemaps/" + folder + "/left.png");
		faces.push_back("../cubemaps/" + folder + "/top.png");
		faces.push_back("../cubemaps/" + folder + "/bottom.png");
		faces.push_back("../cubemaps/" + folder + "/back.png");
		faces.push_back("../cubemaps/" + folder + "/front.png");
	}
	glGenTextures(1, &cubemap);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, cubemap);

	glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY,
		0,                // level
		GL_RGB8,         // Internal format
		1024, 1024, faces.size(), // width,height,depth (must be multiple of 6, since each cubemap is 6 images)
		0,
		GL_RGB,          // format
		GL_UNSIGNED_BYTE, // type
		0);               // pointer to data

	int width, height;
	unsigned char* image;
	for (GLuint i = 0; i < faces.size(); i++)
	{
		//TODO: reduce load time. It's pretty awful currently.
		//https://gamedev.stackexchange.com/questions/126561/long-loading-times-on-large-png-file-using-c-opengl-soil
		image = SOIL_load_image(faces[i].c_str(), &width, &height, 0, SOIL_LOAD_RGB);
		glTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, 0, 0, i, width, height, 1, GL_RGB, GL_UNSIGNED_BYTE, image);

	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

}