#include "Simulation.h"

//initialize using xml file
Simulation::Simulation(char filename[])
{

	timestep.value = .25;
	timestep.unitIndex = 2;

	totalTime.value = .1;
	totalTime.unitIndex = 0;

	playbackSpeed = 1;

	pugi::xml_document doc;

	pugi::xml_parse_result result = doc.load_file(filename);
	time = doc.select_node("/SavedState/Simulation/Time").node().text().as_float();

	std::vector<SimulationObject> objects = {};
	for (auto currentObjectNode : doc.select_nodes("/SavedState/Simulation/Objects/Object"))
	{
		std::string name = currentObjectNode.node().child("Name").text().as_string();
		float mass = currentObjectNode.node().child("Mass").text().as_float();
		float x = currentObjectNode.node().child("Position").child("x").text().as_float();
		float y = currentObjectNode.node().child("Position").child("y").text().as_float();
		float z = currentObjectNode.node().child("Position").child("z").text().as_float();
		std::array<float, 3> position = { x, y, z };

		float Vx = currentObjectNode.node().child("Velocity").child("Vx").text().as_float();
		float Vy = currentObjectNode.node().child("Velocity").child("Vy").text().as_float();
		float Vz = currentObjectNode.node().child("Velocity").child("Vz").text().as_float();
		std::array<float, 3> velocity = { Vx, Vy, Vz };

		bool showHistory = currentObjectNode.node().child("Settings").child("ShowHistory").text().as_bool();
		std::string displayType = currentObjectNode.node().child("Settings").child("DisplayType").text().as_string();

		std::string colorString = currentObjectNode.node().child("Settings").child("Color").text().as_string();
		ObjectSettings settings(showHistory, displayType, colorString);
		SimulationObject currentObject(name, mass, position, velocity);

		objects.push_back(currentObject);
		objectSettings.push_back(settings);

	}

	computedData.push_back(objects);
	dataIndex = 0;
}

Simulation::~Simulation()
{
}

void Simulation::step(float dt) {
	if (dt == 0)
		return;

	switch (selectedAlgorithm) {
	case VELOCITY_VERLET:
	case RUNGE_KUTTA:
	case RK_ADAPTIVE_STEPSIZE:
		velocityVerlet(dt);
	}

	time += dt;
}

std::vector<SimulationObject> Simulation::getCurrentObjects() {
	return computedData[dataIndex];
}

#//returns 2d vector containing the net acceleration on each object
std::vector<std::vector<float>> Simulation::getAccelerations(std::vector<SimulationObject> objects) {
	if (objects.empty())
		objects = computedData[dataIndex];

	std::vector<std::vector<float>> accelerations = {};
	for (int i = 0; i < objects.size(); i++) {
		std::vector<float> acceleration = { 0,0,0 };
		for (int j = 0; j < objects.size(); j++) {
			if (i != j) {
				std::array<float, 3> x1, x2;
				std::copy(std::begin(objects[i].position), std::end(objects[i].position), std::begin(x1));
				std::copy(std::begin(objects[j].position), std::end(objects[j].position), std::begin(x2));

				float r = sqrt(pow(x1[0] - x2[0], 2) + pow(x1[1] - x2[1], 2) + pow(x1[2] - x2[2], 2));
				std::array<float, 3> rUnit = { (x1[0] - x2[0]) / r, (x1[1] - x2[1]) / r, (x1[2] - x2[2]) / r };
				for (int k = 0; k < 3; k++)
					acceleration[k] += -G * objects[j].mass * rUnit[k] / pow(r, 2);
			}
		}
		accelerations.push_back(acceleration);
	}

	return accelerations;
}

//source: http://physics.ucsc.edu/~peter/242/leapfrog.pdf
void Simulation::velocityVerlet(float dt) {
	std::vector<SimulationObject> currentObjects = computedData[dataIndex];
	std::vector<std::vector<float>> accelerations = getAccelerations(currentObjects);

	for (int i = 0; i < currentObjects.size(); i++) {
		for (int j = 0; j < 3; j++) {
			//get velocities of objects at + 1/2 timestep.
			currentObjects[i].velocity[j] += .5 * dt * accelerations[i][j];
			//get position at +1 timestep, using velocity at half timestep
			currentObjects[i].position[j] += dt * currentObjects[i].velocity[j];
		}
	}

	accelerations = getAccelerations(currentObjects);
	for (int i = 0; i < currentObjects.size(); i++) {
		for (int j = 0; j < 3; j++) {
			currentObjects[i].velocity[j] += .5 * dt * accelerations[i][j];
		}
	}

	computedData.push_back(currentObjects);
	dataIndex++;
}