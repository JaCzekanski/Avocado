#version 150 

in vec2 position;
in vec2 texcoord;

out vec2 fragTexcoord;

void main()
{
	fragTexcoord = texcoord;
	gl_Position = vec4(position.x * 2.0 - 1.0, -(position.y * 2.0 - 1.0), 0.0, 1.0);
}