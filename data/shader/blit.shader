uniform sampler2D renderBuffer;
uniform vec2 iResolution;
uniform vec2 displayHorizontal;
uniform vec2 displayVertical;
uniform bool displayEnabled;

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
    vec2 pos = gl_FragCoord.xy / iResolution;
    pos.y = 1. - pos.y;
    
    if (!displayEnabled || 
        pos.x < displayHorizontal.x || pos.x >= displayHorizontal.y ||
        pos.y < displayVertical.x   || pos.y >= displayVertical.y) {
        outColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    outColor = vec4(texture(renderBuffer, fragTexcoord).rgb, 1.0);
}
#endif