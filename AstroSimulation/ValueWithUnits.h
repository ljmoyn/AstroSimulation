#ifndef VALUEWITHUNITS_H
#define VALUEWITHUNITS_H

#include <vector>

enum class UnitType { Time, Position, Velocity, Mass };


template <UnitType type>
class ValueWithUnits
{
public:
	ValueWithUnits(float val, int uIndex) : value(val), unitIndex(uIndex){}
	~ValueWithUnits() {}
	float GetConvertedValue() 
	{
		switch (type)
		{
		case UnitType::Position:
		case UnitType::Velocity:
		case UnitType::Mass:

		case UnitType::Time:
			return value * timeConversions[unitIndex];
		}
	}
	const char* GetUnits()
	{
		switch (type)
		{
		case UnitType::Position:
		case UnitType::Velocity:
		case UnitType::Mass:
		case UnitType::Time:
			
			return timeUnits;
		}
	}
	float value;
	int unitIndex;
	//Conversions to base units, which are used in all algorithmic calculations: Years, Kg, Gigameters 
	//Probably causing horrible roundoff errors with all these conversions. I'll worry about it when it becomes a problem.
	const char* timeUnits[5] = { "Years", "Months", "Days", "Hours", "Minutes" };
	const float timeConversions[5] = { 1.0f, 1.0f / 12.0f, 1.0f / 365.0f, 1.0f / (365.0f * 24.0f), 1.0f / (365.0f * 24.0f * 60.0f) };

	const char* massUnits[4] = { "Kg", "Lbs", "Earth Mass", "Solar Mass"};
	const float massConversions[4] = { 1.0f,
		2.20462, //lbs
		1.0f / (5.972f * powf(10,24)), // earth mass
		1.0f / (1.989f * powf(10,30)) //solar mass
	};

	const char* positionUnits[8] = { "GigaMeters", "M", "Km", "Mi", "AU", "Light Seconds", "Light Minutes", "Light Years"};
	const float positionConversions[8] = { 1.0f,
		1.0f / powf(10,9), //M
		1.0f / powf(10,6), //Km
		1.60934f * powf(10,-6), // Mi
		149.598f, //AU
		0.299792f, //light second
		17.9875f, //light minute
		9.461f * powf(10,6)//light year
	};

	const char* velocityUnits[6] = { "GM / Year", "M/s", "Km/s", "Km/Hr", "Mi/Hr", "c" };
	const float velocityConversions[6] = { 1.0f,
		31.7098f, //M / s
		0.0317098f, //Km / s
		114.155f, // Km / hr
		70.9326284499f, //Mi/Hr
		9460528.4f, //c
	};
};

#endif