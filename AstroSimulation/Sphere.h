#ifndef SPHERE_H
#define SPHERE_H

#include <vector>
#include <algorithm>
#include <map>
#include <glm/glm.hpp>


class Sphere
{
public:
	Sphere();
	~Sphere() {}

	std::vector<int> indices;
	std::vector<float> vertices;
	std::vector<float> textureCoordinates;

private:
	void Subdivide(std::vector<float> vertices, std::vector<int> indices, int n);
	int GetMidpointIndex(int index1, int index2, std::vector<float> midpoint, int * nextIndex, std::map<std::pair<int, int>, int> * midpointDict);
	int GetUpdatedIndex(int index, std::vector<float> vertex, int * nextIndex, std::map<int, int> * indexDict);
	void Inflate();

	//Unused and don't really work, but might be handy in the future
	void InitTextureCoordinates();
	void FixIndexWinding();	
	bool IsCorrectWindingOrder(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3);
};

#endif