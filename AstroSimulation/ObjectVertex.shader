#version 450 core
layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 inputColor;
layout (location = 2) in vec3 position;
layout (location = 3) in int textureIndex;

out vec3 vertexColor;
flat out int texIndex;
out vec3 texCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(vertex + position, 1.0f);
	vertexColor = inputColor;
	texCoord = vertex;
	texIndex = textureIndex;
}