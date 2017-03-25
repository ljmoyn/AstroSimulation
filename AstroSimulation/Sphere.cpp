#include "Sphere.h"

Sphere::Sphere() {
	std::vector<int> icosahedronIndices = {
		0,4,1,
		0,9,4,
		9,5,4,
		4,5,8,
		4,8,1,
		8,10,1,
		8,3,10,
		5,3,8,
		5,2,3,
		2,7,3,
		7,10,3,
		7,6,10,
		7,11,6,
		11,0,6,
		0,1,6,
		6,1,10,
		9,0,11,
		9,11,2,
		9,2,5,
		7,2,11
	};

	//http://www.opengl.org.ru/docs/pg/0208.html
	//chosen so that points are all 1 from the center
	float X = .525731112119133606;
	float Z = .850650808352039932;
	std::vector<float> icosahedronVertices = {
		-X, 0.0, Z,
		 X, 0.0, Z,
		-X, 0.0, -Z,
		 X, 0.0, -Z,
		 0.0, Z, X,
		 0.0, Z, -X,
		 0.0, -Z, X,
		 0.0, -Z, -X,
		 Z, X, 0.0,
		-Z, X, 0.0,
		 Z, -X, 0.0,
		-Z, -X, 0.0
	};

	Subdivide(icosahedronVertices, icosahedronIndices, 2);
	Inflate();
}
//http://stackoverflow.com/questions/7687148/drawing-sphere-in-opengl-without-using-glusphere
//http://gamedev.stackexchange.com/questions/31308/algorithm-for-creating-spheres
void Sphere::Subdivide(std::vector<float> inputVertices, std::vector<int> inputIndices, int n) {
	vertices = {};
	indices = {};

	// input index --> new index
	std::map<int, int> indexDict;
	// two input indices --> new index of the midpoint between them
	std::map<std::pair<int,int>, int> midpointDict;

	for (int i = 0; i < inputIndices.size(); i += 3) {
		int index1 = inputIndices[i];
		int index2 = inputIndices[i + 1];
		int index3 = inputIndices[i + 2];

		std::vector<float> vertex1 = { inputVertices[3 * index1], inputVertices[3 * index1 + 1], inputVertices[3 * index1 + 2], };
		std::vector<float> vertex2 = { inputVertices[3 * index2], inputVertices[3 * index2 + 1], inputVertices[3 * index2 + 2], };
		std::vector<float> vertex3 = { inputVertices[3 * index3], inputVertices[3 * index3 + 1], inputVertices[3 * index3 + 2], };

		std::vector<float> midpoint12 = {};
		std::vector<float> midpoint13 = {};
		std::vector<float> midpoint23 = {};
		for (int j = 0; j < 3; j++) {
			midpoint12.push_back(vertex2[j] + (vertex1[j] - vertex2[j]) / 2.0f);
			midpoint13.push_back(vertex3[j] + (vertex1[j] - vertex3[j]) / 2.0f);
			midpoint23.push_back(vertex3[j] + (vertex2[j] - vertex3[j]) / 2.0f);
		}

		int nextIndex = vertices.size() / 3;
		int newIndex1, newIndex2, newIndex3, newIndex12, newIndex13, newIndex23;

		newIndex12 = GetMidpointIndex(index1, index2, midpoint12, &nextIndex, &midpointDict);
		newIndex13 = GetMidpointIndex(index1, index3, midpoint13, &nextIndex, &midpointDict);
		newIndex23 = GetMidpointIndex(index2, index3, midpoint23, &nextIndex, &midpointDict);

		newIndex1 = GetUpdatedIndex(index1, vertex1, &nextIndex, &indexDict);
		newIndex2 = GetUpdatedIndex(index2, vertex2, &nextIndex, &indexDict);
		newIndex3 = GetUpdatedIndex(index3, vertex3, &nextIndex, &indexDict);
		
		indices.push_back(newIndex1);
		indices.push_back(newIndex13);
		indices.push_back(newIndex12);

		indices.push_back(newIndex2);
		indices.push_back(newIndex12);
		indices.push_back(newIndex23);

		indices.push_back(newIndex3);
		indices.push_back(newIndex13);
		indices.push_back(newIndex23);

		indices.push_back(newIndex12);
		indices.push_back(newIndex23);
		indices.push_back(newIndex13);
	}

	if(n > 1)
		Subdivide(vertices, indices, n-1);
}

int Sphere::GetMidpointIndex(int index1, int index2, std::vector<float> midpoint, int * nextIndex, std::map<std::pair<int, int>, int>* midpointDict)
{
	int newIndex;

	if (midpointDict->find(std::make_pair(index1, index2)) == midpointDict->end() &&
		midpointDict->find(std::make_pair(index2, index1)) == midpointDict->end()) {
		for (int j = 0; j < 3; j++) {
			vertices.push_back(midpoint[j]);
		}
		newIndex = *nextIndex;
		midpointDict->insert(std::pair<std::pair<int, int>, int>(std::make_pair(index1, index2), *nextIndex));
		(*nextIndex)++;
	}
	else {
		std::map<std::pair<int, int>, int>::iterator iter = midpointDict->find(std::make_pair(index1, index2));
		if (iter != midpointDict->end())
			newIndex = (*midpointDict)[std::make_pair(index1, index2)];
		else
			newIndex = (*midpointDict)[std::make_pair(index2, index1)];
	}

	return newIndex;
}

int Sphere::GetUpdatedIndex(int index, std::vector<float> vertex, int * nextIndex, std::map<int, int>* indexDict)
{
	int newIndex;
	if (indexDict->find(index) == indexDict->end()) {
		for (int j = 0; j < 3; j++) {
			vertices.push_back(vertex[j]);
		}

		newIndex = *nextIndex;
		indexDict->insert(std::pair<int, int>(index, *nextIndex));

		(*nextIndex)++;
	}
	else {
		newIndex = (*indexDict)[index];
	}

	return newIndex;
}

void Sphere::Inflate()
{
	for (int i = 0; i < vertices.size(); i += 3) {
		//assuming this is centered on the origin
		float dx = vertices[i]; 
		float dy = vertices[i+1];
		float dz = vertices[i+2];

		float currentDistance = std::powf(dx*dx + dy*dy + dz*dz, .5);
		//assuming radius = 1
		dx *= 1.0 / currentDistance;
		dy *= 1.0 / currentDistance;
		dz *= 1.0 / currentDistance;

		// = 0 + dx
		vertices[i] = dx;
		vertices[i+1] = dy;
		vertices[i+2] = dz;
	}
}