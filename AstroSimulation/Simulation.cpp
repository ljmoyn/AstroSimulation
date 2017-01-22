#include "Simulation.h"

Simulation::Simulation()
{
}

Simulation::~Simulation()
{
}

void Simulation::FromXml(Simulation *sim, std::string filename)
{
	sim->playbackSpeed = 1;

	pugi::xml_document doc;

	pugi::xml_parse_result result = doc.load_file(filename.c_str());
	sim->time = doc.select_node("/SavedState/Simulation/Time").node().text().as_float();
	sim->objectSettings = {};
	std::vector<SimulationObject> objects = {};
	for (auto currentObjectNode : doc.select_nodes("/SavedState/Simulation/Objects/Object"))
	{
		std::string name = currentObjectNode.node().child("Name").text().as_string();
		float mass = currentObjectNode.node().child("Mass").text().as_float();
		float x = currentObjectNode.node().child("Position").child("x").text().as_float();
		float y = currentObjectNode.node().child("Position").child("y").text().as_float();
		float z = currentObjectNode.node().child("Position").child("z").text().as_float();
		float position[3] = { x, y, z };

		float Vx = currentObjectNode.node().child("Velocity").child("Vx").text().as_float();
		float Vy = currentObjectNode.node().child("Velocity").child("Vy").text().as_float();
		float Vz = currentObjectNode.node().child("Velocity").child("Vz").text().as_float();
		float velocity[3] = { Vx, Vy, Vz };

		bool showHistory = currentObjectNode.node().child("Settings").child("ShowHistory").text().as_bool();
		std::string displayType = currentObjectNode.node().child("Settings").child("DisplayType").text().as_string();

		std::string colorString = currentObjectNode.node().child("Settings").child("Color").text().as_string();
		ObjectSettings settings(showHistory, displayType, colorString);
		SimulationObject currentObject(name, mass, position, velocity);

		objects.push_back(currentObject);
		sim->objectSettings.push_back(settings);

	}

	sim->computedData = {objects};
	sim->dataIndex = 0;
}

void Simulation::ToXml(Simulation simulation, std::string filename) {
	pugi::xml_document xml;
	pugi::xml_node root = xml.append_child("SavedState");
	pugi::xml_node simulationNode = root.append_child("Simulation");
	simulationNode.append_child("Time").append_child(pugi::node_pcdata).set_value(std::to_string(simulation.time).c_str());

	pugi::xml_node objectsNode = simulationNode.append_child("Objects");
	std::vector<SimulationObject> objects = simulation.getCurrentObjects();
	for (int i = 0; i < objects.size(); i++)
	{
		pugi::xml_node objectNode = objectsNode.append_child("Object");
		objectNode.append_child("Name").append_child(pugi::node_pcdata).set_value(objects[i].name.c_str());
		objectNode.append_child("Mass").append_child(pugi::node_pcdata).set_value(std::to_string(objects[i].mass.value).c_str());

		pugi::xml_node position = objectNode.append_child("Position");
		pugi::xml_node velocity = objectNode.append_child("Velocity");
		pugi::xml_node settings = objectNode.append_child("Settings");

		float convertedPosition[3], convertedVelocity[3];
		objects[i].position.GetConvertedValue(convertedPosition);
		objects[i].velocity.GetConvertedValue(convertedVelocity);

		position.append_child("x").append_child(pugi::node_pcdata).set_value(std::to_string(convertedPosition[0]).c_str());
		position.append_child("y").append_child(pugi::node_pcdata).set_value(std::to_string(convertedPosition[1]).c_str());
		position.append_child("z").append_child(pugi::node_pcdata).set_value(std::to_string(convertedPosition[2]).c_str());

		velocity.append_child("Vx").append_child(pugi::node_pcdata).set_value(std::to_string(convertedVelocity[0]).c_str());
		velocity.append_child("Vy").append_child(pugi::node_pcdata).set_value(std::to_string(convertedVelocity[1]).c_str());
		velocity.append_child("Vz").append_child(pugi::node_pcdata).set_value(std::to_string(convertedVelocity[2]).c_str());

		settings.append_child("ShowHistory").append_child(pugi::node_pcdata).set_value(std::to_string(simulation.objectSettings[i].showHistory).c_str());
		settings.append_child("DisplayType").append_child(pugi::node_pcdata).set_value(simulation.objectSettings[i].TypeToString().c_str());

		std::string colorString = std::to_string(simulation.objectSettings[i].color[0]) + "," +
			std::to_string(simulation.objectSettings[i].color[1]) + "," +
			std::to_string(simulation.objectSettings[i].color[2]);
		settings.append_child("Color").append_child(pugi::node_pcdata).set_value(colorString.c_str());
	}

	xml.save_file(filename.c_str());
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
				float x1[3], x2[3];
				objects[i].position.GetConvertedValue(x1);
				objects[j].position.GetConvertedValue(x2);

				float r = sqrt(pow(x1[0] - x2[0], 2) + pow(x1[1] - x2[1], 2) + pow(x1[2] - x2[2], 2));
				float rUnit[3] = { (x1[0] - x2[0]) / r, (x1[1] - x2[1]) / r, (x1[2] - x2[2]) / r };
				for (int k = 0; k < 3; k++)
					acceleration[k] += -G * objects[j].mass.value * rUnit[k] / pow(r, 2);
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
			currentObjects[i].velocity.value[j] += .5 * dt * accelerations[i][j];
			//get position at +1 timestep, using velocity at half timestep
			currentObjects[i].position.value[j] += dt * currentObjects[i].velocity.value[j];
		}
	}

	accelerations = getAccelerations(currentObjects);
	for (int i = 0; i < currentObjects.size(); i++) {
		for (int j = 0; j < 3; j++) {
			currentObjects[i].velocity.value[j] += .5 * dt * accelerations[i][j];
		}
	}

	computedData.push_back(currentObjects);
	dataIndex++;
}