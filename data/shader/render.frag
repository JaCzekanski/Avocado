#version 150

#define FILTERING 0

in vec3 fragColor;
in vec2 fragTexcoord;
flat in uvec3 fragFlatColor;
flat in uint fragBitcount;
flat in ivec2 fragClut;
flat in ivec2 fragTexpage;
flat in uint fragFlags;
flat in uint fragTextureWindow;

out vec4 outColor;

uniform sampler2D vram;

const uint BIT_NONE = 0u;
const uint BIT_4 = 4U;
const uint BIT_8 = 8U;
const uint BIT_16 = 16U;

const uint SemiTransparency = 1u << 0;
const uint RawTexture = 1u << 1;
const uint Dithering = 1u << 2;
const uint GouroudShading = 1u << 3;

const uint Bby2plusFby2 = 0u;
const uint BplusF = 1u;
const uint BminusF = 2u;
const uint BplusFby4 = 3u;

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
    uvec2 texel = uvec2(uint(texcoord.x) % 256u, uint(texcoord.y) % 256u);

    uvec2 mask = uvec2((fragTextureWindow)&0x1fu, (fragTextureWindow >> 5) & 0x1fu);
    uvec2 offset = uvec2((fragTextureWindow >> 10) & 0x1fu, (fragTextureWindow >> 15) & 0x1fu);

    texel.x = (texel.x & ~(mask.x * 8u)) | ((offset.x & mask.x) * 8u);
    texel.y = (texel.y & ~(mask.y * 8u)) | ((offset.y & mask.y) * 8u);

    return vec2(texel);
}

vec4 doShading(vec3 color, uint flags) {
    vec4 outColor = vec4(color, 1.0);

    // TODO: Dithering

    return outColor;
}

vec4 clut4bit(vec2 coord, ivec2 clut) {
    int texX = fragTexpage.x + int(coord.x / 4);
    int texY = fragTexpage.y + int(coord.y);

    uint index = internalToPsxColor(vramRead(texX, texY));
    uint which = (index >> ((uint(coord.x) & 3u) * 4u)) & 0xfu;

    return vramRead(clut.x + int(which), clut.y);
}

vec4 clut8bit(vec2 coord, ivec2 clut) {
    int texX = fragTexpage.x + int(coord.x / 2.0);
    int texY = fragTexpage.y + int(coord.y);

    uint index = internalToPsxColor(vramRead(texX, texY));
    uint which = (index >> ((uint(coord.x) & 1u) * 8u)) & 0xffu;

    return vramRead(clut.x + int(which), clut.y);
}

vec4 filter4bit(vec2 texcoord, ivec2 fragClut) {
    vec2 texel = calculateTexel(fragTexcoord);
    float xf = fract(fragTexcoord.x);
    float yf = fract(fragTexcoord.y);

    vec4 colors[3 * 3];
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            colors[(y + 1) * 3 + (x + 1)] = clut4bit(ivec2(texel.x + x, texel.y + y), fragClut);
        }
    }

    vec4 color = vec4(0, 0, 0, 0);
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            float distancex = clamp(2.0 - abs((xf * 2 - 1) - float(x)), 0.0, 1.0);
            float distancey = clamp(2.0 - abs((yf * 2 - 1) - float(y)), 0.0, 1.0);
            float distance = sqrt(pow(distancex, 2) * pow(distancey, 2));
            color = mix(color, colors[(y + 1) * 3 + (x + 1)], distance);
        }
    }
    return clamp(color, vec4(0), vec4(1));
}

vec4 read16bit(vec2 coord) { return vramRead(fragTexpage.x + int(coord.x), fragTexpage.y + int(coord.y)); }

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
#if (FILTERING == 1)
        color = filter4bit(fragTexcoord, fragClut);
#else
        color = clut4bit(texel, fragClut);
#endif
    } else if (fragBitcount == BIT_8) {
        color = clut8bit(texel, fragClut);  // vec4(0.0, 1.0, 0.0, 1.0);
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
            brightness = vec4(fragFlatColor.r / 255.f, fragFlatColor.g / 255.f, fragFlatColor.b / 255.f, 1.f);
        }

        color.r = clamp(color.r * brightness.r * 2.f, 0.f, 1.f);
        color.g = clamp(color.g * brightness.g * 2.f, 0.f, 1.f);
        color.b = clamp(color.b * brightness.b * 2.f, 0.f, 1.f);
    }

    // Blending/Transparency
    if (((fragFlags & SemiTransparency) == SemiTransparency)
        && ((fragBitcount != BIT_NONE && color.a != 0.f) || (fragBitcount == BIT_NONE))) {
        uint transparency = (fragFlags & 0x60u) >> 5;
        if (transparency == Bby2plusFby2)
            color.a = 0.5f;
        else if (transparency == BplusF) {
            color.a = 0.5f;
        } else if (transparency == BminusF) {
            color.r = -color.r;
            color.g = -color.g;
            color.b = -color.b;
            color.a = 0.5f;
        } else if (transparency == BplusFby4)
            color.a = 0.25f;
        /*
        auto transparency = (Transparency)((flags & 0x60) >> 5);
        switch (transparency) {
            case Transparency::Bby2plusFby2: c = bg / 2.f + c / 2.f; break;
            case Transparency::BplusF: c = bg + c; break;
            case Transparency::BminusF: c = bg - c; break;
            case Transparency::BplusFby4: c = bg + c / 4.f; break;
        }
        */
        color.a = 0.5f;
    } else {
        color.a = 1.f;
    }

    outColor = color;
}