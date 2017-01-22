#ifndef SIMULATIONOBJECT_H
#define SIMULATIONOBJECT_H

#pragma once
#include <string>
#include <cmath>
#include <array>
#include "ValueWithUnits.h"
class SimulationObject
{
public:
	SimulationObject(std::string Name, float Mass, float Position[3], float Velocity[3]);
	~SimulationObject();
	SimulationObject() {};

	float GetSpeed();
	void SetPosition(float newPosition, int index);
	void SetVelocity(float newVelocity, int index);
	void SetMass(float newMass);

	std::string name;
	ValueWithUnits<UnitType::Mass> mass;
	ValueWithUnits3<UnitType::Position> position;
	ValueWithUnits3<UnitType::Velocity> velocity;

private:

};

#endif