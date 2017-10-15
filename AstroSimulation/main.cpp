#include "Simulation.h"

int main()
{
	Simulation simulation("..\\saves\\Default.xml");
	Simulation::initSimulationControls(&simulation);

	// Game loop
	while (!glfwWindowShouldClose(simulation.window))
	{
		simulation.update();
	}

	// Cleanup
	ImGui_ImplGlfwGL3_Shutdown();
	glfwTerminate();

	return 0;
}

