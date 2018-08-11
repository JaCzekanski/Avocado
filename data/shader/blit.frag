#version 150

in vec2 fragTexcoord;

out vec4 outColor;

uniform sampler2D renderBuffer;
uniform vec2 displayEnd;

void main()
{
	vec2 pos = vec2(fragTexcoord.x*1024.0, fragTexcoord.y*512.0);
	if (pos.y < displayEnd.y) {
		outColor = vec4(texture(renderBuffer, fragTexcoord).rgb, 1.0);
	} else {
		outColor = vec4(0.0, 0.0, 0.0, 1.0);
	}
}