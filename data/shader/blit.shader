uniform sampler2D renderBuffer;
uniform vec2 clipLeftTop;
uniform vec2 clipRightBottom;

SHARED vec2 fragTexcoord;

#ifdef VERTEX_SHADER
in vec2 position;
in vec2 texcoord;

void main() {
    fragTexcoord = texcoord;
    gl_Position = vec4(position.x * 2.0 - 1.0, position.y * 2.0 - 1.0, 0.0, 1.0);
}
#endif


#ifdef FRAGMENT_SHADER
void main() {
    vec2 pos = vec2(fragTexcoord.x, fragTexcoord.y);
    
    if (pos.x < clipLeftTop.x || pos.y < clipLeftTop.y || 
        pos.x >= clipRightBottom.x || pos.y >= clipRightBottom.y) {
        outColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    outColor = vec4(texture(renderBuffer, fragTexcoord).rgb, 1.0);
}
#endif