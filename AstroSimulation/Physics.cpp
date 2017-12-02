#include "Physics.h"

Physics::Physics()
{
}

Physics::~Physics()
{
}

void Physics::FromXml(Physics *physics, std::string filename, std::vector<std::string> textureFolders)
{
	physics->playbackSpeed = 1;

	pugi::xml_document doc;

	pugi::xml_parse_result result = doc.load_file(filename.c_str());
	physics->time = doc.select_node("/SavedState/Physics/Time").node().text().as_float();
	physics->objectSettings = {};
	std::vector<PhysObject> objects = {};
	for (auto currentObjectNode : doc.select_nodes("/SavedState/Physics/Objects/PhysObject"))
	{
		std::string name = currentObjectNode.node().child("Name").text().as_string();
		float mass = currentObjectNode.node().child("Mass").text().as_float();
		float rotationPeriod = currentObjectNode.node().child("RotationPeriod").text().as_float();
		float axialTilt = currentObjectNode.node().child("AxialTilt").text().as_float();

		float x = currentObjectNode.node().child("Position").child("x").text().as_float();
		float y = currentObjectNode.node().child("Position").child("y").text().as_float();
		float z = currentObjectNode.node().child("Position").child("z").text().as_float();
		float position[3] = { x, y, z };

		float Vx = currentObjectNode.node().child("Velocity").child("Vx").text().as_float();
		float Vy = currentObjectNode.node().child("Velocity").child("Vy").text().as_float();
		float Vz = currentObjectNode.node().child("Velocity").child("Vz").text().as_float();
		float velocity[3] = { Vx, Vy, Vz };

		float radius = currentObjectNode.node().child("Radius").text().as_float();
		//don't allow invalid data. if radius data not included at all, pugixml defaults to 0. 
		if (radius <= 0)
			radius = 1.0;

		bool showHistory = currentObjectNode.node().child("Settings").child("ShowHistory").text().as_bool();
		std::string displayType = currentObjectNode.node().child("Settings").child("DisplayType").text().as_string();

		std::string colorString = currentObjectNode.node().child("Settings").child("Color").text().as_string();
		std::string texture = currentObjectNode.node().child("Settings").child("Texture").text().as_string();
		int textureIndex = -1;
		if (texture != "") {
			for (int i = 0; i < textureFolders.size(); i++) {
				if (textureFolders[i] == texture) {
					textureIndex = i;
					break;
				}
			}
		}

		ObjectSettings settings(showHistory, displayType, colorString, textureIndex);
		PhysObject currentObject(name, mass, position, velocity, radius, rotationPeriod, axialTilt);

		objects.push_back(currentObject);
		physics->objectSettings.push_back(settings);

	}

	physics->computedData = {objects};
	physics->dataIndex = 0;
	physics->objectFocus = 0;
}

void Physics::ToXml(Physics physics, std::string filename) {
	pugi::xml_document xml;
	pugi::xml_node root = xml.append_child("SavedState");
	pugi::xml_node physicsNode = root.append_child("Physics");
	physicsNode.append_child("Time").append_child(pugi::node_pcdata).set_value(std::to_string(physics.time).c_str());

	pugi::xml_node objectsNode = physicsNode.append_child("Objects");
	std::vector<PhysObject> objects = physics.getCurrentObjects();
	for (int i = 0; i < objects.size(); i++)
	{
		pugi::xml_node objectNode = objectsNode.append_child("PhysObject");
		objectNode.append_child("Name").append_child(pugi::node_pcdata).set_value(objects[i].name.c_str());
		objectNode.append_child("Mass").append_child(pugi::node_pcdata).set_value(std::to_string(objects[i].mass.value).c_str());
		objectNode.append_child("RotationPeriod").append_child(pugi::node_pcdata).set_value(std::to_string(objects[i].rotationPeriod.value).c_str());
		objectNode.append_child("AxialTilt").append_child(pugi::node_pcdata).set_value(std::to_string(objects[i].axialTilt.value).c_str());

		pugi::xml_node position = objectNode.append_child("Position");
		pugi::xml_node velocity = objectNode.append_child("Velocity");
		pugi::xml_node settings = objectNode.append_child("Settings");

		float convertedPosition[3], convertedVelocity[3];
		objects[i].position.GetBaseValue(convertedPosition);
		objects[i].velocity.GetBaseValue(convertedVelocity);

		position.append_child("x").append_child(pugi::node_pcdata).set_value(std::to_string(convertedPosition[0]).c_str());
		position.append_child("y").append_child(pugi::node_pcdata).set_value(std::to_string(convertedPosition[1]).c_str());
		position.append_child("z").append_child(pugi::node_pcdata).set_value(std::to_string(convertedPosition[2]).c_str());

		velocity.append_child("Vx").append_child(pugi::node_pcdata).set_value(std::to_string(convertedVelocity[0]).c_str());
		velocity.append_child("Vy").append_child(pugi::node_pcdata).set_value(std::to_string(convertedVelocity[1]).c_str());
		velocity.append_child("Vz").append_child(pugi::node_pcdata).set_value(std::to_string(convertedVelocity[2]).c_str());

		settings.append_child("ShowHistory").append_child(pugi::node_pcdata).set_value(std::to_string(physics.objectSettings[i].showHistory).c_str());
		settings.append_child("DisplayType").append_child(pugi::node_pcdata).set_value(physics.objectSettings[i].TypeToString().c_str());

		std::string colorString = std::to_string(physics.objectSettings[i].color[0]) + "," +
			std::to_string(physics.objectSettings[i].color[1]) + "," +
			std::to_string(physics.objectSettings[i].color[2]);
		settings.append_child("Color").append_child(pugi::node_pcdata).set_value(colorString.c_str());
	}

	xml.save_file(filename.c_str());
}

void Physics::step(float dt) {
	if (dt == 0)
		return;

	std::vector<PhysObject> currentObjects = computedData[dataIndex];
	switch (selectedAlgorithm) {
		case VELOCITY_VERLET:
		case RUNGE_KUTTA:
		case RK_ADAPTIVE_STEPSIZE:
			velocityVerlet(dt, &currentObjects);
	}

	for (int i = 0; i < currentObjects.size(); i++) {
		currentObjects[i].rotationDegrees += dt * 360.0f / currentObjects[i].rotationPeriod.GetBaseValue();
	}

	computedData.push_back(currentObjects);
	dataIndex++;

	time += dt;
}

std::vector<PhysObject> Physics::getCurrentObjects() {
	return computedData[dataIndex];
}

#//returns 2d vector containing the net acceleration on each object
std::vector<std::vector<float>> Physics::getAccelerations(std::vector<PhysObject> * objects) {
	if (objects->empty())
		*objects = computedData[dataIndex];

	std::vector<std::vector<float>> accelerations = {};
	for (int i = 0; i < objects->size(); i++) {
		std::vector<float> acceleration = { 0,0,0 };
		for (int j = 0; j < objects->size(); j++) {
			if (i != j) {
				float x1[3], x2[3];
				std::copy(std::begin((*objects)[i].position.value), std::end((*objects)[i].position.value), std::begin(x1));
				std::copy(std::begin((*objects)[j].position.value), std::end((*objects)[j].position.value), std::begin(x2));

				float r = sqrt(pow(x1[0] - x2[0], 2) + pow(x1[1] - x2[1], 2) + pow(x1[2] - x2[2], 2));
				float rUnit[3] = { (x1[0] - x2[0]) / r, (x1[1] - x2[1]) / r, (x1[2] - x2[2]) / r };
				for (int k = 0; k < 3; k++)
					acceleration[k] += -G * (*objects)[j].mass.value * rUnit[k] / pow(r, 2);
			}
		}
		accelerations.push_back(acceleration);
	}

	return accelerations;
}

std::vector<std::map<std::string, int> > Physics::ConvertObjectsToBaseUnits(std::vector<PhysObject>* objects)
{
	std::vector<std::map<std::string,int> > originalUnits = {};
	for (int i = 0; i < objects->size(); i++) {
		std::map<std::string, int> units;

		// first insert function version (single parameter):
		units.insert(std::pair<std::string, int>("mass", (*objects)[i].mass.unitIndex));
		units.insert(std::pair<std::string, int>("position", (*objects)[i].position.unitIndex));
		units.insert(std::pair<std::string, int>("velocity", (*objects)[i].velocity.unitIndex));
		originalUnits.push_back(units);

		(*objects)[i].position.SetBaseUnits();
		(*objects)[i].velocity.SetBaseUnits();
		(*objects)[i].mass.SetBaseUnits();
	}

	return originalUnits;
}

void Physics::ConvertObjectsToUnits(std::vector<PhysObject>* objects, std::vector<std::map<std::string, int> > units) {

	if (objects->size() != units.size())
		return;

	for (int i = 0; i < objects->size(); i++) {
		(*objects)[i].position.ConvertToUnits(units[i]["position"]);
		(*objects)[i].velocity.ConvertToUnits(units[i]["velocity"]);
		(*objects)[i].mass.ConvertToUnits(units[i]["mass"]);
	}
}

void Physics::updatePaths(bool firstFrame) {
	std::vector<PhysObject> currentObjects = getCurrentObjects();
	if (firstFrame) {
		paths = {};
		for (int i = 0; i < currentObjects.size(); i++) {
			std::vector<float> offsets = GetFocusOffsets(currentObjects);

			paths.push_back({
				currentObjects[i].position.GetBaseValue(0) - offsets[0],
				currentObjects[i].position.GetBaseValue(1) - offsets[1],
				currentObjects[i].position.GetBaseValue(2) - offsets[2],
			});
		}

	}
	else {
		for (int i = 0; i < currentObjects.size(); i++) {
			std::vector<float> offsets = GetFocusOffsets(currentObjects);

			paths[i].push_back(currentObjects[i].position.GetBaseValue(0) - offsets[0]);
			paths[i].push_back(currentObjects[i].position.GetBaseValue(1) - offsets[1]);
			paths[i].push_back(currentObjects[i].position.GetBaseValue(2) - offsets[2]);
		}
	}
}

//source: http://physics.ucsc.edu/~peter/242/leapfrog.pdf
void Physics::velocityVerlet(float dt, std::vector<PhysObject> * currentObjects) {
	std::vector<std::map<std::string, int> > originalUnits = Physics::ConvertObjectsToBaseUnits(currentObjects);
	std::vector<std::vector<float>> accelerations = getAccelerations(currentObjects);

	for (int i = 0; i < currentObjects->size(); i++) {
		for (int j = 0; j < 3; j++) {
			//get velocities of objects at + 1/2 timestep.
			(*currentObjects)[i].velocity.value[j] += .5 * dt * accelerations[i][j];
			//get position at +1 timestep, using velocity at half timestep
			(*currentObjects)[i].position.value[j] += dt * (*currentObjects)[i].velocity.value[j];
		}
	}

	accelerations = getAccelerations(currentObjects);
	for (int i = 0; i < currentObjects->size(); i++) {
		for (int j = 0; j < 3; j++) {
			(*currentObjects)[i].velocity.value[j] += .5 * dt * accelerations[i][j];
		}
	}

	Physics::ConvertObjectsToUnits(currentObjects, originalUnits);
}

std::vector<std::string> Physics::GetObjectNames() {
	std::vector<PhysObject> currentObjects = getCurrentObjects();
	std::vector<std::string> names = { "None" };
	for (int i = 0; i < currentObjects.size(); i++) {
		names.push_back(currentObjects[i].name);
	}
	return names;
}

std::vector<float> Physics::GetFocusOffsets(std::vector <PhysObject> objects) {
	float xOffset = 0, yOffset = 0, zOffset = 0;
	if (objectFocus > 0) {
		//-1 since the first is "no focus"
		xOffset = objects[objectFocus-1].position.GetBaseValue(0);
		yOffset = objects[objectFocus-1].position.GetBaseValue(1);
		zOffset = objects[objectFocus-1].position.GetBaseValue(2);
	}

	return{ xOffset, yOffset, zOffset };
}