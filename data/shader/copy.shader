uniform sampler2D vram;

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
    outColor = vec4(texture(vram, fragTexcoord).rgb, 1.0);
}
#endif