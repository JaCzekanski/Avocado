#version 150

in ivec2 position;
in uvec3 color;
in uvec2 texcoord;
in uint bitcount;
in uvec2 clut;
in uvec2 texpage;
in uint flags;

out vec3 fragColor;
out vec2 fragTexcoord;
flat out uint fragBitcount;
flat out uvec2 fragClut;
flat out uvec2 fragTexpage;
flat out uint fragFlags;

uniform vec2 displayAreaPos;
uniform vec2 displayAreaSize;

void main() {
    vec2 pos = vec2((position.x - displayAreaPos.x) / displayAreaSize.x, (position.y - displayAreaPos.y) / displayAreaSize.y);
    // vec2 pos = vec2(position.x / 1024.f, position.y / 512.f);
    fragColor = vec3(color.r / 255.f, color.g / 255.f, color.b / 255.f);
    fragTexcoord = vec2(texcoord.x, texcoord.y);
    fragBitcount = bitcount;
    fragClut = clut;
    fragTexpage = texpage;
    fragFlags = flags;

    // Change 0-1 space to OpenGL -1 - 1
    gl_Position = vec4(pos.x * 2.f - 1.f, (1.f - pos.y) * 2.f - 1.f, 0.0, 1.0);
}