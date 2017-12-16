#ifndef PHYSOBJECT_H
#define PHYSOBJECT_H

#pragma once
#include <string>
#include <cmath>
#include <array>
#include "ValueWithUnits.h"
class PhysObject
{
public:
	PhysObject(std::string Name, float Mass, float Position[3], float Velocity[3], float Radius, float rotationPeriod, float axialTilt, std::vector<std::string> satellites);
	~PhysObject();
	PhysObject() {};

	float GetSpeed();
	void SetPosition(float newPosition, int index);
	void SetVelocity(float newVelocity, int index);
	void SetMass(float newMass);

	std::string name;
	ValueWithUnits<UnitType::Distance> radius;
	ValueWithUnits<UnitType::Mass> mass;
	ValueWithUnits3<UnitType::Distance> position;
	ValueWithUnits3<UnitType::Velocity> velocity;
	//a.k.a. obliquity
	ValueWithUnits<UnitType::Angle> axialTilt;

	//Name of major object that this is orbiting. Used to organize the list of objects in the ui 
	std::vector<std::string> satellites;

	ValueWithUnits<UnitType::Time> rotationPeriod;
	//amount to rotate object model, based on current timestep and period
	float rotationDegrees;
private:

};

#endif