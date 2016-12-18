#include "Visual.h"
// The MAIN function, from here we start the application and run the game loop
int main()
{
	Visual visual("C:\\AstroWorkspace\\AstroSimulation\\saves\\Default.xml");
	Visual::initVisualControls(&visual);
	Shader shader("shader.vertex", "shader.fragment");

	// Game loop
	while (!glfwWindowShouldClose(visual.window))
	{
		visual.update(shader);
	}

	// Cleanup

	ImGui_ImplGlfwGL3_Shutdown();
	glfwTerminate();

	return 0;
}

