#version 450 core
in vec3 vertexColor;
in vec3 texCoord;
flat in int texIndex;

out vec4 color;
uniform samplerCubeArray cubemap;

void main()
{
	if (texIndex > -1)
		color = texture(cubemap, vec4(texCoord, texIndex));
	else
		color = vec4(vertexColor, 1.0);
}