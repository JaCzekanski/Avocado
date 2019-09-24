uniform vec2 displayAreaPos;
uniform vec2 displayAreaSize;

uniform sampler2D vram;

const uint BIT_NONE = 0u;
const uint BIT_4 = 4u;
const uint BIT_8 = 8u;
const uint BIT_16 = 16u;

const uint SemiTransparency = 1u << 0u;
const uint RawTexture = 1u << 1u;
const uint Dithering = 1u << 2u;
const uint GouroudShading = 1u << 3u;

const uint Bby2plusFby2 = 0u;
const uint BplusF = 1u;
const uint BminusF = 2u;
const uint BplusFby4 = 3u;

const float W = 1024.f;
const float H = 512.f;

SHARED vec3 fragColor;
SHARED vec2 fragTexcoord;
flat SHARED uvec3 fragFlatColor;
flat SHARED uint fragBitcount;
flat SHARED ivec2 fragClut;
flat SHARED ivec2 fragTexpage;
flat SHARED uint fragFlags;
flat SHARED uint fragTextureWindow;

#ifdef VERTEX_SHADER
in ivec2 position;
in uvec3 color;
in ivec2 texcoord;
in uint bitcount;
in ivec2 clut;
in ivec2 texpage;
in uint flags;
in uint textureWindow;

void main() {
    vec2 pos = vec2((float(position.x) - displayAreaPos.x) / displayAreaSize.x, (float(position.y) - displayAreaPos.y) / displayAreaSize.y);
    // vec2 pos = vec2(position.x / 1024.f, position.y / 512.f);
    fragColor = vec3(float(color.r) / 255.f, float(color.g) / 255.f, float(color.b) / 255.f);
    fragTexcoord = vec2(texcoord.x, texcoord.y);
    fragFlatColor = uvec3(color.r, color.g, color.b);
    fragBitcount = bitcount;
    fragClut = clut;
    fragTexpage = texpage;
    fragFlags = flags;
    fragTextureWindow = textureWindow;

    // Change 0-1 space to OpenGL -1 - 1
    gl_Position = vec4(pos.x * 2.f - 1.f, (1.f - pos.y) * 2.f - 1.f, 0.0, 1.0);
}
#endif


#ifdef FRAGMENT_SHADER
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
    int texX = fragTexpage.x + int(coord.x / 4.0);
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

vec4 read16bit(vec2 coord) { return vramRead(fragTexpage.x + int(coord.x), fragTexpage.y + int(coord.y)); }

void main() {
    uint x = uint(gl_FragCoord.x);
    uint y = uint(gl_FragCoord.y);

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
            brightness = vec4(float(fragFlatColor.r) / 255.f, float(fragFlatColor.g) / 255.f, float(fragFlatColor.b) / 255.f, 1.f);
        }

        color.r = clamp(color.r * brightness.r * 2.f, 0.f, 1.f);
        color.g = clamp(color.g * brightness.g * 2.f, 0.f, 1.f);
        color.b = clamp(color.b * brightness.b * 2.f, 0.f, 1.f);
    }

    outColor = color;
}
#endif