#include "PhysObject.h"

PhysObject::PhysObject(std::string Name, float Mass, float Position[3], float Velocity[3], float Radius)
	: name(Name), mass(Mass, 0), position(Position, 2), velocity(Velocity, 4), radius(Radius, 2){}

PhysObject::~PhysObject()
{
}

float PhysObject::GetSpeed()
{
	return sqrtf(pow(velocity.value[0], 2) + pow(velocity.value[1], 2) + pow(velocity.value[2], 2));
}

void PhysObject::SetPosition(float newPosition, int index) {
	position.value[index] = newPosition;
}

void PhysObject::SetVelocity(float newVelocity, int index) {
	velocity.value[index] = newVelocity;
}

void PhysObject::SetMass(float newMass) {
	mass.value = newMass;
}