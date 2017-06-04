#version 450 core
in vec3 vertexColor;
in vec3 texCoord;
flat in int texIndex;

out vec4 color;
uniform samplerCubeArray cubemap;

void main()
{
	color = texture(cubemap, vec4(texCoord, 0));
}