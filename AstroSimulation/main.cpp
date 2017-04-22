#include "Visual.h"

int main()
{
	Visual visual("..\\saves\\Default.xml");
	Visual::initVisualControls(&visual);

	// Game loop
	while (!glfwWindowShouldClose(visual.window))
	{
		visual.update();
	}

	// Cleanup
	ImGui_ImplGlfwGL3_Shutdown();
	glfwTerminate();

	return 0;
}

