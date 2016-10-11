#pragma once
#include <cmath>

class Ellipse
{
public:
	Ellipse(float a, float b = 0, float e = 0);
	~Ellipse();
	float semiminorAxis;
	float semimajorAxis;
	float eccentricity;
private:

};

Ellipse::Ellipse(float a, float b = 0, float e = 0)
{
	semimajorAxis = a;
	if (e == 0 && b != 0) 
	{
		semiminorAxis = b;
		eccentricity = sqrtf(1 - pow(b, 2) / pow(a, 2));
	}
	else if (e != 0) 
	{
		eccentricity = e;
		semiminorAxis = a * sqrtf(1 - pow(e, 2));
	}
	else
	{
		//circle
		eccentricity = 0;
		semiminorAxis = semimajorAxis;
	}
}

Ellipse::~Ellipse()
{
}