#version 150

in vec3 fragColor;
in vec2 fragTexcoord;
flat in uint fragBitcount;
flat in uvec2 fragClut;
flat in uvec2 fragTexpage;
flat in uint fragFlags;

out vec4 outColor;

uniform sampler2D vram;

// uniform uvec2 drawingAreaTopLeft;
// uniform uvec2 drawingAreaBottomRight;

const float W = 1024.f;
const float H = 512.f;

uint internalToPsxColor(vec4 c) {
    uint a = uint(floor(c.a + 0.5));
    uint r = uint(floor(c.r * 31.0 + 0.5));
    uint g = uint(floor(c.g * 31.0 + 0.5));
    uint b = uint(floor(c.b * 31.0 + 0.5));
    return (a << 15) | (b << 10) | (g << 5) | r;
}

vec4 vramRead(int x, int y) { return texelFetch(vram, ivec2(x, y), 0); }

vec4 readClut(vec2 coord, uvec2 clut) {
    int texX = int(coord.x / 4.0) & 0xff;
    int texY = int(coord.y) & 0xff;

    texX += int(fragTexpage.x);
    texY += int(fragTexpage.y);

    vec4 firstStage = vramRead(texX, texY);
    uint index = internalToPsxColor(firstStage);

    uint part = uint(mod(coord.x, 8.0));
    uint which = index >> (part * (fragBitcount / 2u));
    which = which & (fragBitcount - 1u);

    return vramRead(int(clut.x + which), int(clut.y));
}
vec4 clut4bit(vec2 coord, uvec2 clut) {
    int texX = int(coord.x / 4.0) & 0xff;
    int texY = int(coord.y) & 0xff;

    texX += int(fragTexpage.x);
    texY += int(fragTexpage.y);

    vec4 firstStage = vramRead(texX, texY);
    uint index = internalToPsxColor(firstStage);

    uint part = uint(coord.x) & 3u;
    uint which = index >> (part * 4u);
    which = which & 15u;

    return vramRead(int(clut.x + which), int(clut.y));
}

vec4 clut8bit(vec2 coord, uvec2 clut) {
    int texX = int(coord.x / 2.0) & 0xff;
    int texY = int(coord.y) & 0xff;

    texX += int(fragTexpage.x);
    texY += int(fragTexpage.y);

    vec4 firstStage = vramRead(texX, texY);
    uint index = internalToPsxColor(firstStage);

    uint part = uint(coord.x) & 1u;
    uint which = index >> (part * 8u);
    which = which & 0xffu;

    return vramRead(int(clut.x + which), int(clut.y));
}

vec4 read16bit(vec2 coord) { return vramRead(int(fragTexpage.x + coord.x), int(fragTexpage.y + coord.y)); }

void main() {
    uint x = uint(gl_FragCoord.x);
    uint y = uint(gl_FragCoord.y);

    // if (x < drawingAreaTopLeft.x || x > drawingAreaBottomRight.x ||
    // 	y < drawingAreaTopLeft.y || y > drawingAreaBottomRight.y) discard;

    vec4 color;
    if (fragBitcount == 4U)
        color = clut4bit(fragTexcoord, fragClut);
    else if (fragBitcount == 8U)
        color = clut8bit(fragTexcoord, fragClut);
    else if (fragBitcount == 16U)
        color = read16bit(fragTexcoord);
    else
        color = vec4(fragColor, 0.0);

    // Transparency
    if (((fragFlags & 1u) == 1u || fragBitcount > 0u) && internalToPsxColor(color) == 0x0000u) discard;

    // If textured and if not raw texture, add brightness
    if (fragBitcount > 0u && (fragFlags & 2u) != 2u) {
        if (fragColor.r != 0) color.r = clamp(color.r + (fragColor.r - 0.5f) * 0.5f, 0.0, 1.0);
        if (fragColor.g != 0) color.g = clamp(color.g + (fragColor.g - 0.5f) * 0.5f, 0.0, 1.0);
        if (fragColor.b != 0) color.b = clamp(color.b + (fragColor.b - 0.5f) * 0.5f, 0.0, 1.0);
    }

    outColor = color;
}