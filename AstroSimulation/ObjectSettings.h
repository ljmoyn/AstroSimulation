#ifndef OBJECTSETTINGS_H
#define OBJECTSETTINGS_H

#pragma once
#include <algorithm>
#include <array>
#include <vector>
#include <sstream>

class ObjectSettings
{
public:
	ObjectSettings(bool showHistory, std::string displayType, std::string colorString);
	~ObjectSettings();

	void SetDisplayType(std::string type);
	void SetColorFromString(std::string colorString);

	bool showHistory;
	enum class DisplayTypes { Point, Image, Sphere};
	DisplayTypes displayType;
	std::array<float, 3> color;

private:

};

#endif