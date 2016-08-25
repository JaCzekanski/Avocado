#version 330 

in uvec2 position;
in uvec3 color;
in vec2 texcoord;

out vec3 fragColor;
out vec2 fragTexcoord;

void main()
{
	vec2 pos = vec2(position.x / 1024.f, position.y/512.f);
	fragColor = vec3(color.r / 255.f, color.g/255.f, color.b/255.f);
	fragTexcoord = texcoord;
	gl_Position = vec4(pos, 0.0, 1.0);
}