#version 150

in vec3 fragColor;
in vec2 fragTexcoord;
flat in uvec3 fragFlatColor;
flat in uint fragBitcount;
flat in uvec2 fragClut;
flat in uvec2 fragTexpage;
flat in uint fragFlags;

out vec4 outColor;

uniform sampler2D vram;

// uniform uvec2 drawingAreaTopLeft;
// uniform uvec2 drawingAreaBottomRight;

const uint BIT_NONE = 0u;
const uint BIT_4 = 4U;
const uint BIT_8 = 8U;
const uint BIT_16 = 16U;

const uint SemiTransparency = 1u << 0;
const uint RawTexture = 1u << 1;
const uint Dithering = 1u << 2;
const uint GouroudShading = 1u << 3;

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

vec2 calculateTexel(vec2 texcoord) {
    vec2 texel = vec2(mod(texcoord.x, 256.f), mod(texcoord.y, 256.f));

    // TODO: Masking

    return texel;
}

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

vec4 doShading(vec3 color, uint flags) {
    vec4 outColor = vec4(color, 0.0);

    // TODO: Dithering

    return outColor;
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

    vec2 texel = calculateTexel(fragTexcoord);

    vec4 color;
    if (fragBitcount == BIT_NONE) {
        color = doShading(fragColor, fragFlags);
    } else if (fragBitcount == BIT_4) {
        color = clut4bit(texel, fragClut);
    } else if (fragBitcount == BIT_8) {
        color = clut8bit(texel, fragClut);
    } else if (fragBitcount == BIT_16) {
        color = read16bit(texel);
    }

    // Transparency
    if ((fragBitcount != BIT_NONE || ((fragFlags & SemiTransparency) == SemiTransparency)) && internalToPsxColor(color) == 0x0000u) discard;

    // If textured and if not raw texture, add brightness
    if (fragBitcount != BIT_NONE && !((fragFlags & RawTexture) == RawTexture)) {
        vec4 brightness;

        if ((fragFlags & GouroudShading) == GouroudShading) {
            brightness = doShading(fragColor, 0u);
        } else {  // Flat shading
            brightness = vec4(fragFlatColor.r / 255.f, fragFlatColor.g / 255.f, fragFlatColor.b / 255.f, 0.f);
        }

        color.r = clamp(color.r * brightness.r * 2.f, 0.f, 1.f);
        color.g = clamp(color.g * brightness.g * 2.f, 0.f, 1.f);
        color.b = clamp(color.b * brightness.b * 2.f, 0.f, 1.f);
    }

    // Blending/Transparency
    if (((fragFlags & SemiTransparency) == SemiTransparency) && ((fragBitcount != BIT_NONE) || (fragBitcount == BIT_NONE))) {
        color.a = 0.5f;
    }

    outColor = color;
}