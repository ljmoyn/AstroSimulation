#version 330 core
in vec3 vertexColor;
in vec2 texCoord;
flat in int texIndex;

out vec4 color;
uniform sampler2DArray textures;

void main()
{
	color = texture(textures, vec3(texCoord, texIndex));
}