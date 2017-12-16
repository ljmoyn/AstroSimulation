#ifndef PHYSICS_H
#define PHYSICS_H

#pragma once
#include "PhysObject.h"
#include "ObjectSettings.h"
#include "pugixml/pugixml.hpp"
#include "ValueWithUnits.h"

#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <map>

#define VELOCITY_VERLET 0
#define RUNGE_KUTTA 1
#define RK_ADAPTIVE_STEPSIZE 2

class Physics
{
public:
	Physics();
	~Physics();

	void step(float dt);
	void velocityVerlet(float dt, std::vector<PhysObject> * currentObjects);
	std::vector<std::vector<float>> getAccelerations(std::vector<PhysObject> * objects = {});
	std::vector<PhysObject> getCurrentObjects();
	static void FromXml(Physics* physics, std::string filename, std::vector<std::string> textureFolders);
	static void ToXml(Physics physics, std::string filename);

	static std::vector<std::map<std::string, int> > ConvertObjectsToBaseUnits(std::vector<PhysObject>* objects);
	static void ConvertObjectsToUnits(std::vector<PhysObject>* objects, std::vector<std::map<std::string, int> > units);

	std::vector<std::string> GetObjectNames();
	std::vector<float> GetFocusOffsets(std::vector <PhysObject> objects);
	//keep objects and objectSettings as separate objects, because I want 
	//PhysObject to contain only the fundamental object data, rather than
	//get cluttered up with ui info.
	std::vector<ObjectSettings> objectSettings;
	std::vector<std::vector<PhysObject>> computedData;

	void updatePaths(bool firstFrame);
	std::vector<std::vector<float> > paths;

	//used to store previous data when in the middle of computing new set. Needed so that "Cancel" button can reset everything
	std::vector<std::vector<PhysObject>> temporaryData;
	int temporaryIndex;

	ValueWithUnits<UnitType::Time> timestep = ValueWithUnits<UnitType::Time>(0.1f, 2);
	ValueWithUnits<UnitType::Time> totalTime = ValueWithUnits<UnitType::Time>(.15f, 0);

	int playbackSpeed;
	int dataIndex;
	float time;

	int objectFocus;

	int selectedAlgorithm;
	const char* algorithms[3] = { "Velocity Verlet", "Runge Kutta 4", "RK45 with Adaptive Stepsize" };

private:
	static std::vector<std::string> SplitString(std::string str, std::string delimiter);
	const float G = 9.94519 * pow(10, 14) * 6.67408 * pow(10, -11) / pow(pow(10, 9), 3);

};

#endif