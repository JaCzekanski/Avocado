#version 150

in vec2 fragTexcoord;

out vec4 outColor;

uniform sampler2D renderBuffer;
uniform vec2 clipLeftTop;
uniform vec2 clipRightBottom;

void main() {
    vec2 pos = vec2(fragTexcoord.x, fragTexcoord.y);
    
    if (pos.x < clipLeftTop.x || pos.y < clipLeftTop.y || 
        pos.x >= clipRightBottom.x || pos.y >= clipRightBottom.y) {
        outColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    outColor = vec4(texture(renderBuffer, fragTexcoord).rgb, 1.0);
}