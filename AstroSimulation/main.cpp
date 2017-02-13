#include "Visual.h"

int main()
{
	Visual visual("..\\saves\\Default.xml");
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

