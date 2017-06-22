#version 450 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 inputColor;

out vec3 vertexColor;

uniform mat4 view;
uniform mat4 projection;

void main()
{
	gl_Position = projection * view * vec4(position, 1.0f);
	vertexColor = inputColor;

}