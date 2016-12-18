#include "ObjectSettings.h"

ObjectSettings::ObjectSettings(bool ShowHistory, std::string DisplayType, std::string colorString)
{
	showHistory = ShowHistory;
	SetColorFromString(colorString);
	SetDisplayType(DisplayType);
}

ObjectSettings::~ObjectSettings()
{
}

void ObjectSettings::SetDisplayType(std::string type)
{
	if (type == "Image")
		displayType = DisplayTypes::Image;
	else if (type == "Sphere")
		displayType = DisplayTypes::Sphere;
	else
		displayType = DisplayTypes::Point;
}

//colorString is a comma separated string of numbers
void ObjectSettings::SetColorFromString(std::string colorString)
{
	std::vector<std::string> colorStringVec;
	boost::split(colorStringVec, colorString, boost::is_any_of(","));

	float r = 1.0;
	float g = 1.0;
	float b = 1.0;
	if (colorStringVec.size() == 3)
	{
		boost::trim(colorStringVec[0]);
		r = std::stof(colorStringVec[0]);

		boost::trim(colorStringVec[1]);
		g = std::stof(colorStringVec[1]);

		boost::trim(colorStringVec[2]);
		b = std::stof(colorStringVec[2]);
	}

	color[0] = r;
	color[1] = g;
	color[2] = b;
}