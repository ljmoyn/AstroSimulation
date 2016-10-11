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
	std::array<float,3> position;
	std::array<float,3> velocity;
private:

};

SimulationObject::SimulationObject(std::string Name, float Mass, std::array<float, 3> Position, std::array<float, 3> Velocity)
{
	name = Name;
	mass = Mass;
	for (int i = 0; i < 3; i++) {
		position[i] = Position[i];
		velocity[i] = Velocity[i];
	}

}

SimulationObject::~SimulationObject()
{
}

float SimulationObject::GetSpeed() 
{
	return sqrtf(pow(velocity[0], 2) + pow(velocity[1], 2) + pow(velocity[2], 2));
}

void SimulationObject::SetPosition(float newPosition, int index) {
	position[index] = newPosition;
}

void SimulationObject::SetVelocity(float newVelocity, int index) {
	velocity[index] = newVelocity;
}

void SimulationObject::SetMass(float newMass) {
	mass = newMass;
}