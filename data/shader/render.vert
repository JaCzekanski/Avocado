#version 150

in ivec2 position;
in uvec3 color;
in ivec2 texcoord;
in uint bitcount;
in ivec2 clut;
in ivec2 texpage;
in uint flags;
in uint textureWindow;

out vec3 fragColor;
out vec2 fragTexcoord;
flat out uvec3 fragFlatColor;
flat out uint fragBitcount;
flat out ivec2 fragClut;
flat out ivec2 fragTexpage;
flat out uint fragFlags;
flat out uint fragTextureWindow;

uniform vec2 displayAreaPos;
uniform vec2 displayAreaSize;

void main() {
    vec2 pos = vec2((position.x - displayAreaPos.x) / displayAreaSize.x, (position.y - displayAreaPos.y) / displayAreaSize.y);
    // vec2 pos = vec2(position.x / 1024.f, position.y / 512.f);
    fragColor = vec3(color.r / 255.f, color.g / 255.f, color.b / 255.f);
    fragTexcoord = ivec2(texcoord.x, texcoord.y);
    fragFlatColor = uvec3(color.r, color.g, color.b);
    fragBitcount = bitcount;
    fragClut = clut;
    fragTexpage = texpage;
    fragFlags = flags;
    fragTextureWindow = textureWindow;

    // Change 0-1 space to OpenGL -1 - 1
    gl_Position = vec4(pos.x * 2.f - 1.f, (1.f - pos.y) * 2.f - 1.f, 0.0, 1.0);
}