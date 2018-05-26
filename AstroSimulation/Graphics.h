#ifndef GRAPHICS_H
#define GRAPHICS_H
// GLEW
#define GLEW_STATIC
#include <GL/glew.h>
#include <GL/GLU.H>

// GLFW
#include <GLFW/glfw3.h>

// Other Libs
#include <soil/SOIL.h>
// GLM Mathematics
#define GLM_SWIZZLE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "Shader.h"
#include "Physics.h"

#include "Camera.h"
#include "Sphere.h"

class Graphics
{
public:
	Graphics();
	~Graphics() {};

	void setView();
	void drawPoints(Physics * physics);
	void drawSpheres(Physics * physics);
	void drawLines(Physics * physics);
	void drawDebugLine();
	void LoadTextures(std::vector<std::string> textureFolders);

	std::vector<float> debugPoints = {};

	GLFWwindow* window;
	Camera camera;
	Sphere sphere = Sphere();

	GLuint width = 800;
	GLuint height = 800;

	glm::mat4 model;
	glm::mat4 projection;
	glm::mat4 view;
	GLfloat xTranslate;
	GLfloat yTranslate;
	GLfloat zTranslate;

	//name of the object to be followed
	std::string followObject;
private:

	unsigned char* LoadDDS(std::string imagePath, int * format, int * mipmapCount, int * width, int * height);



	//input are coordinates on a sphere
	float GetPixelDiameter(glm::vec4 surfacePoint, glm::vec4 centerPoint);
	void GetWindowCoordinates(glm::vec4 point, float * xWindow, float *yWindow);
	void GetVertexAttributeData(bool drawAsPoint, std::vector<GLfloat> * positions, std::vector<GLfloat> * colors, std::vector<GLint> * textureCoordinates, std::vector<glm::mat4> * objectOrientations, int * count, Physics * physics);

	void CheckGlError(std::string message = "");
	//static void APIENTRY Simulation::glDebugOutput(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam);

	GLuint cubemap;
	GLuint textures;
	GLuint instanceModelsVBO;
	GLuint textureCoordinateVBO;
	GLuint textureIndexVBO;
	GLuint sphereVBO;
	GLuint positionVBO;
	GLuint colorVBO;
	GLuint vertexVBO;
	GLuint VAO;
	GLuint EBO;

	GLuint pointsShaderProgram;
	GLuint objectsShaderProgram;
	GLuint pathsShaderProgram;
};
#endif