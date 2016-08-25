#version 330

in vec3 fragColor;
in vec2 fragTexcoord;

out vec4 outColor;

uniform sampler2D vram;

void main()
{
	vec4 col = vec4(fragColor, 1.0);
	//vec4 test = vec4(fragTexcoord, 1.0, 1.0);
	vec4 tex = vec4(texture(vram, fragTexcoord).rgb, 1.0);
	outColor = mix(col, tex, 0.5);
}