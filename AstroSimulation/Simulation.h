#ifndef SIMULATION_H
#define SIMULATION_H

#pragma once
#include "SimulationObject.h"
#include "ObjectSettings.h"
#include "pugixml/pugixml.hpp"
#include <fstream>
#include <iostream>
#include <vector>
#include <string>

#define VELOCITY_VERLET 0
#define RUNGE_KUTTA 1
#define RK_ADAPTIVE_STEPSIZE 2

class Simulation
{
public:
	Simulation();
	~Simulation();

	void step(float dt);
	void velocityVerlet(float dt);
	std::vector<std::vector<float>> getAccelerations(std::vector<SimulationObject> objects = {});
	std::vector<SimulationObject> getCurrentObjects();
	static void FromXml(Simulation* simulation, std::string filename);

	//keep objects and objectSettings as separate objects, because I want 
	//SimulationObject to contain only the fundamental object data, rather than
	//get cluttered up with ui info.
	std::vector<ObjectSettings> objectSettings;
	std::vector<std::vector<SimulationObject>> computedData;

	//used to store previous data when in the middle of computing new set. Needed so that "Cancel" button can reset everything
	std::vector<std::vector<SimulationObject>> temporaryData;
	int temporaryIndex;

	struct timeWithUnits {
		float value;
		int unitIndex;
		const char* unitStrings[5] = { "Years", "Months", "Days", "Hours", "Minutes" };
		const float conversionFactors[5] = { 1.0f, 1.0f / 12.0f, 1.0f / 365.0f, 1.0f / (365.0f * 24.0f), 1.0f / (365.0f * 24.0f * 60.0f) };
		float getConvertedValue() {
			return value * conversionFactors[unitIndex];
		}
	} timestep, totalTime;

	int playbackSpeed;
	int dataIndex;
	float time;

	int selectedAlgorithm;
	const char* algorithms[3] = { "Velocity Verlet", "Runge Kutta 4", "RK45 with Adaptive Stepsize" };

private:
	const float G = 9.94519 * pow(10, 14) * 6.67408 * pow(10, -11) / pow(pow(10, 9), 3);

};

#endif