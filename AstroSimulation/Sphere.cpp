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

	Subdivide(icosahedronVertices, icosahedronIndices, 3);
	Inflate();
	//InitTextureCoordinates();

	FixIndexWinding();
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

//Unused and doesn't really work, but might be handy in the future
void Sphere::InitTextureCoordinates()
{
	textureCoordinates = {};

	float pi = 3.14159265;
	float maxLongitude = 0;
	float minLongitude = 1;

	std::vector<float> longitudes = {};
	for (int i = 0; i < vertices.size(); i += 3)
	{
		float radius = std::sqrtf(std::powf(vertices[i], 2) + std::powf(vertices[i + 1], 2) + std::powf(vertices[i + 2], 2));

		//scaled to be from 0 - 1
		float longitude = (std::atan2(vertices[i], vertices[i + 2]) + pi) / (2 * pi);
		float latitude = std::acos(vertices[i + 1] / radius) / pi;

		if (longitude > maxLongitude)
			maxLongitude = longitude;
		if (longitude < minLongitude)
			minLongitude = longitude;

		longitudes.push_back(longitude);
		textureCoordinates.push_back(longitude);
		textureCoordinates.push_back(latitude);
	}

	//for debugging...
	std::sort(longitudes.begin(), longitudes.end());

	//https://stackoverflow.com/questions/9511499/seam-issue-when-mapping-a-texture-to-a-sphere-in-opengl
	//https://acko.net/blog/making-worlds-1-of-spheres-and-cubes/
	//http://wiki.alioth.net/index.php/Planettool

	//note: because of the way I initialize my icosahedron, the max longitude is always going to be exactly 1 (aka 2*pi radians).
	//if you do it differently, might need to add an extra step here to shift all the texture coordinates so that the max is at exactly 1, or the min is at exactly 0
	std::map<int, float> maxLongitudeIndices = {};
	std::map<int, float> minLongitudeIndices = {};
	for (int i = 0; i < textureCoordinates.size(); i += 2) {
		if (textureCoordinates[i] == maxLongitude) {
			maxLongitudeIndices.insert(std::pair<int, float>(i / 2, vertices[i / 2]));
		}
		else if (textureCoordinates[i] == minLongitude) {
			minLongitudeIndices.insert(std::pair<int, float>(i / 2, vertices[i / 2]));
		}
	}

	int newIndex = vertices.size() / 3;
	for (int i = 0; i < indices.size(); i += 3) {

		//if this triangle has a vertex that is at the max texture coordinate
		bool textureEnd = false;
		int textureEndIndex;

		//if this triangle has a vertex that is at the min texture coordinate
		bool textureBegin = false;

		for (int j = 0; j < 3; j++)
		{
			if (maxLongitudeIndices.find(indices[i + j]) != maxLongitudeIndices.end())
			{
				textureEnd = true;
				textureEndIndex = j;
			}
			else if (minLongitudeIndices.find(indices[i + j]) != minLongitudeIndices.end())
			{
				textureBegin = true;
			}
		}

		//if this triangle spans across the 'edge' of the  texture, from end to beginning, need to duplicate the vertex at the end of the texture, except with a tex coord of 0
		//replace the old vertex in this triangle with the duplicate. 
		if (textureBegin && textureEnd) {
			//copy vertex coordinates
			vertices.push_back(vertices[3 * indices[i + textureEndIndex]]);
			vertices.push_back(vertices[3 * indices[i + textureEndIndex] + 1]);
			vertices.push_back(vertices[3 * indices[i + textureEndIndex] + 2]);

			textureCoordinates.push_back(0.0);

			//keep same longitude as original vertex
			textureCoordinates.push_back(textureCoordinates[2 * indices[i + textureEndIndex] + 1]);

			//order matters. This comes after pushing to the new data to vectors
			indices[i + textureEndIndex] = newIndex;

			newIndex++;
		}
	}

}

bool Sphere::IsCorrectWindingOrder(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3)
{
	glm::vec3 crossVec1 = p2 - p1;
	glm::vec3 crossVec2 = p3 - p1;
	glm::vec3 normal = glm::normalize(glm::cross(crossVec1, crossVec2));

	//center of sphere is at origin, so this is the vector from center of the triangle, to center of the sphere 
	glm::vec3 centerOfTriangle = glm::normalize(glm::vec3(
		(p1[0] + p2[0] + p3[0]) / 3.0f,
		(p1[1] + p2[1] + p3[1]) / 3.0f,
		(p1[2] + p2[2] + p3[2]) / 3.0f
	));

	//The two vectors are either going to be the same, or negatives of each other.
	//The winding is correct if the normal is opposite the vector towards the center, 
	//since that means the 'front' of the triangle is on the outside of the sphere
	bool windingIsCorrect = false;
	for (int i = 0; i < 3; i++) {
		if (normal[i] == 0 || centerOfTriangle[i] == 0)
			continue;

		//NOTE: Ideally, I could just check if they are equal. But even when pointing in the same direction, they are slightly off. Probably related to float precision. 
		//Could probably clean up the way I calculate them, but it doesn't seem to matter.
		windingIsCorrect = (normal[i] > 0 && centerOfTriangle[i] < 0) || (normal[i] < 0 && centerOfTriangle[i] > 0);
		break;
	}

	return windingIsCorrect;
}

//doesn't really work, but might be handy in the future
void Sphere::FixIndexWinding()
{
	//need to make sure each triangle has a consistent winding order
	for (int i = 0; i < indices.size(); i += 3) {
		int index1 = indices[i];
		int index2 = indices[i+1];
		int index3 = indices[i+2];

		glm::vec3 vertex1 = glm::vec3(vertices[3 * index1], vertices[3 * index1 + 1], vertices[3 * index1 + 2]);
		glm::vec3 vertex2 = glm::vec3(vertices[3 * index2], vertices[3 * index2 + 1], vertices[3 * index2 + 2]);
		glm::vec3 vertex3 = glm::vec3(vertices[3 * index3], vertices[3 * index3 + 1], vertices[3 * index3 + 2]);

		//switch vertex order if necessary
		if (!IsCorrectWindingOrder(vertex1, vertex2, vertex3)) {
			indices[i] = index3;
			indices[i + 2] = index1;
		}
	}
}