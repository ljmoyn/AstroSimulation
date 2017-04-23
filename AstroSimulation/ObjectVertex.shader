#version 330 core
layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 inputColor;
layout (location = 2) in vec3 position;

out vec3 vertexColor;
out vec3 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(vertex + position, 1.0f);
	vertexColor = inputColor;
	TexCoord = vertex;
}