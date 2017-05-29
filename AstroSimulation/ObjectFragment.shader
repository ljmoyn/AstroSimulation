#version 330 core
in vec3 vertexColor;
in vec3 texCoord;
flat in int texIndex;

out vec4 color;
uniform samplerCube cubemap;

void main()
{
	color = texture(cubemap, texCoord);
}