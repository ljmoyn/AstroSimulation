#include "Visual.h"

// The MAIN function, from here we start the application and run the game loop
int main()
{
	Visual visual("C:\\AstroWorkspace\\AstroSimulation\\saves\\Default.xml");
	initVisualControls(&visual);

	// Game loop
	while (!glfwWindowShouldClose(visual.window))
	{
		visual.update();
	}
	return 0;
}

