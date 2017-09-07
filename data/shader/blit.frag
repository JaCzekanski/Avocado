#version 150

in vec2 fragTexcoord;

out vec4 outColor;

uniform sampler2D renderBuffer;

void main()
{
	outColor = vec4(texture(renderBuffer, fragTexcoord).rgb, 1.0);
}