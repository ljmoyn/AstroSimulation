#ifndef SIMULATIONOBJECT_H
#define SIMULATIONOBJECT_H

#pragma once
#include <string>
#include <cmath>
#include <array>

class SimulationObject
{
public:
	SimulationObject(std::string Name, float Mass, std::array<float, 3> Position, std::array<float, 3> Velocity);
	~SimulationObject();

	float GetSpeed();
	void SetPosition(float newPosition, int index);
	void SetVelocity(float newVelocity, int index);
	void SetMass(float newMass);

	std::string name;
	float mass;
	float position[3];
	float velocity[3];
private:

};

#endif