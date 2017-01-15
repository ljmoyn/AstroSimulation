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

//http://stackoverflow.com/questions/236129/split-a-string-in-c
void split(const std::string &s, char delim, std::vector<std::string> &elems) {
	std::stringstream ss;
	ss.str(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		elems.push_back(item);
	}
}


std::vector<std::string> split(const std::string &s, char delim) {
	std::vector<std::string> elems;
	split(s, delim, elems);
	return elems;
}

//http://codereview.stackexchange.com/questions/40124/trim-white-space-from-string
void trim(std::string* str)
{
	if (str->empty())
		return;

	std::size_t firstScan = str->find_first_not_of(' ');
	std::size_t first = firstScan == std::string::npos ? str->length() : firstScan;
	std::size_t last = str->find_last_not_of(' ');
	*str = str->substr(first, last - first + 1);
}

//colorString is a string of floats in the format 'r.rr,g.gg,b.bb'
void ObjectSettings::SetColorFromString(std::string colorString)
{
	std::vector<std::string> colorStringVec = split(colorString, ',');

	float r = 1.0;
	float g = 1.0;
	float b = 1.0;
	if (colorStringVec.size() == 3)
	{
		trim(&colorStringVec[0]);
		r = std::stof(colorStringVec[0]);

		trim(&colorStringVec[1]);
		g = std::stof(colorStringVec[1]);

		trim(&colorStringVec[2]);
		b = std::stof(colorStringVec[2]);
	}

	color[0] = r;
	color[1] = g;
	color[2] = b;
}

//sad that I have to do this
std::string ObjectSettings::TypeToString() 
{
	switch (displayType) 
	{
		case DisplayTypes::Point :
			return "Point";
		case DisplayTypes::Image:
			return "Image";
		case DisplayTypes::Sphere:
			return "Sphere";
	}
}