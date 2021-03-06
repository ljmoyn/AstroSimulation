#include "Simulation.h"
#include "UserInterface.h"
int main()
{
	Simulation simulation("..\\saves\\Default.xml");

	// Game loop
	while (!glfwWindowShouldClose(simulation.graphics.window))
	{
		simulation.update();
	}

	// Cleanup
	ImGui_ImplGlfwGL3_Shutdown();
	glfwTerminate();

	return 0;
}

