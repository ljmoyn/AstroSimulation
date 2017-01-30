#include "SimulationObject.h"

SimulationObject::SimulationObject(std::string Name, float Mass, float Position[3], float Velocity[3])
	: name(Name), mass(Mass, 0), position(Position, 2), velocity(Velocity, 4){}

SimulationObject::~SimulationObject()
{
}

float SimulationObject::GetSpeed()
{
	return sqrtf(pow(velocity.value[0], 2) + pow(velocity.value[1], 2) + pow(velocity.value[2], 2));
}

void SimulationObject::SetPosition(float newPosition, int index) {
	position.value[index] = newPosition;
}

void SimulationObject::SetVelocity(float newVelocity, int index) {
	velocity.value[index] = newVelocity;
}

void SimulationObject::SetMass(float newMass) {
	mass.value = newMass;
}