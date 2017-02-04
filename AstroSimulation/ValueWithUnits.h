#ifndef VALUEWITHUNITS_H
#define VALUEWITHUNITS_H

#include <vector>

enum class UnitType { Time, Position, Velocity, Mass };

struct UnitData {
	//Conversions to base units, which are used in all algorithmic calculations: Years, Kg, Gigameters 
	//Probably causing horrible roundoff errors with all these conversions. I'll worry about it when it becomes a problem.
	const char* timeUnits[5] = { "Years", "Months", "Days", "Hours", "Minutes" };
	const float timeConversions[5] = { 1.0f, 1.0f / 12.0f, 1.0f / 365.0f, 1.0f / (365.0f * 24.0f), 1.0f / (365.0f * 24.0f * 60.0f) };

	const char* massUnits[4] = { "Kg", "Lbs", "Earth Mass", "Solar Mass" };
	const float massConversions[4] = { 
		1.0f, //Kg
		0.45359237f, // convert lbs -----> Kg
		5.972f * powf(10,24), // earth mass
		1.989f * powf(10,30) //solar mass
	};

	const char* positionUnits[8] = { "M", "Km", "GigaMeters", "Mi", "AU", "Light Seconds", "Light Minutes", "Light Years" };
	const float positionConversions[8] = { 
		1.0f / powf(10,9), //convert M -----> Gm
		1.0f / powf(10,6), //Km
		1.0f, // Gm
		1.60934f * powf(10,-6), // Mi
		149.598f, //AU
		0.299792f, //light second
		17.9875f, //light minute
		9.461f * powf(10,6)//light year
	};

	const char* velocityUnits[6] = { "M/s", "Km/s", "Km/Hr", "Mi/Hr", "GM / Year", "c" };
	const float velocityConversions[6] = {
		60.0f * 60.0f * 24.0f * 365.0f / powf(10,9), //convert M / s -----> Gm / yr
		60.0f * 60.0f * 24.0f * 365.0f / powf(10,6), //Km / s
		24.0f * 365.0f / powf(10,6), // Km / hr
		24.0f * 365.0f / (.621371f * powf(10,6)), //Mi/Hr
		1.0f, // Gm / Yr
		2.99792458f * powf(10,8) * 60.0f * 60.0f * 24.0f * 365.0f / powf(10,9), //c
	};
};


template <UnitType type>
class ValueWithUnits
{
public:
	ValueWithUnits() {}
	ValueWithUnits(float val, int uIndex) : value(val), unitIndex(uIndex){}
	~ValueWithUnits() {}

	float GetBaseValue() 
	{
		switch (type)
		{
		case UnitType::Position:
			return value * unitData.positionConversions[unitIndex];
		case UnitType::Velocity:
			return value * unitData.velocityConversions[unitIndex];
		case UnitType::Mass:
			return value * unitData.massConversions[unitIndex];
		case UnitType::Time:
			return value * unitData.timeConversions[unitIndex];
		}
	}

	void SetBaseUnits()
	{
		value = GetBaseValue();

		switch (type)
		{
		case UnitType::Position:
			unitIndex = 2;
			break;
		case UnitType::Velocity:
			unitIndex = 4;
			break;
		case UnitType::Mass:
			unitIndex = 0;
			break;
		case UnitType::Time:
			unitIndex = 0;
			break;
		}
	}

	void ConvertToUnits(int i) 
	{
		SetBaseUnits();
		switch (type)
		{
		case UnitType::Position:
			value /= unitData.positionConversions[i];
			break;
		case UnitType::Velocity:
			value /= unitData.velocityConversions[i];
			break;
		case UnitType::Mass:
			value /= unitData.massConversions[i];
			break;
		case UnitType::Time:
			value /= unitData.timeConversions[i];
			break;
		}

		unitIndex = i;
	}

	float value;
	int unitIndex;
	UnitData unitData;

	ValueWithUnits& operator=(const ValueWithUnits& rhs) {
		ValueWithUnits<type> lhs;
		lhs.value = rhs.value;
		lhs.unitIndex = rhs.unitIndex;
		return lhs;
	};
};

template <UnitType type>
class ValueWithUnits3
{
public:
	ValueWithUnits3() {}
	ValueWithUnits3(float val[3], int uIndex) 
	{
		unitIndex = uIndex;
		for (int i = 0; i < 3; i++)
			value[i] = val[i];
	}
	~ValueWithUnits3() {}
	void GetBaseValue(float (&result)[3])
	{
		for (int i = 0; i < 3; i++) {
			switch (type)
			{
			case UnitType::Position:
				result[i] = value[i] * unitData.positionConversions[unitIndex];
				break;
			case UnitType::Velocity:
				result[i] = value[i] * unitData.velocityConversions[unitIndex];
				break;
			case UnitType::Mass:
				result[i] = value[i] * unitData.massConversions[unitIndex];
				break;
			case UnitType::Time:
				result[i] = value[i] * unitData.timeConversions[unitIndex];
				break;
			}
		}
	}

	float GetBaseValue(int i)
	{
		switch (type)
		{
		case UnitType::Position:
			return value[i] * unitData.positionConversions[unitIndex];
		case UnitType::Velocity:
			return value[i] * unitData.velocityConversions[unitIndex];
		case UnitType::Mass:
			return value[i] * unitData.massConversions[unitIndex];
		case UnitType::Time:
			return value[i] * unitData.timeConversions[unitIndex];
		}
	}

	void SetBaseUnits()
	{
		GetBaseValue(value);

		switch (type)
		{
		case UnitType::Position:
			unitIndex = 2;
			break;
		case UnitType::Velocity:
			unitIndex = 4;
			break;
		case UnitType::Mass:
			unitIndex = 0;
			break;
		case UnitType::Time:
			unitIndex = 2;
			break;
		}
	}

	void ConvertToUnits(int i)
	{
		SetBaseUnits();
		for (int j = 0; j < 3; j++) {
			switch (type)
			{
			case UnitType::Position:
				value[j] /= unitData.positionConversions[i];
				break;
			case UnitType::Velocity:
				value[j] /= unitData.velocityConversions[i];
				break;
			case UnitType::Mass:
				value[j] /= unitData.massConversions[i];
				break;
			case UnitType::Time:
				value[j] /= unitData.timeConversions[i];
				break;
			}
		}

		unitIndex = i;
	}

	float value[3];
	int unitIndex;
	UnitData unitData;

	ValueWithUnits3& operator=(const ValueWithUnits3& rhs) {
		ValueWithUnits3<type> lhs;
		for(int i = 0; i < 3; i++)
			lhs.value[i] = rhs.value[i];
		lhs.unitIndex = rhs.unitIndex;
		return lhs;
	};
};

#endif