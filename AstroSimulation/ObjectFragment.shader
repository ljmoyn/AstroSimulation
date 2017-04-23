#version 330 core
in vec3 vertexColor;
in vec3 TexCoord;

out vec4 color;

uniform sampler2D ourTexture1;

void main()
{
    //color = vec4(vertexColor, 1.0f);
	color = texture(ourTexture1, vec2(.5, .5));
}