#pragma once
#include "SimulationObject.h"
#include "ObjectSettings.h"
#include "pugixml/pugixml.hpp"
#include <fstream>
#include <iostream>
#include <vector>
#include <string>




class Simulation
{
public:
	Simulation(char filename[]);
	~Simulation();

	//keep objects and objectSettings as separate objects, because I want 
	//SimulationObject to contain only the fundamental object data, rather than
	//get cluttered up with ui info.
	std::vector<SimulationObject> objects;
	std::vector<ObjectSettings> objectSettings;
	float time;

private:

};

//initialize using xml file
Simulation::Simulation(char filename[])
{
	pugi::xml_document doc;

	pugi::xml_parse_result result = doc.load_file(filename);
	time = doc.select_node("/SavedState/Simulation/Time").node().text().as_float();

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
}

Simulation::~Simulation()
{
}
